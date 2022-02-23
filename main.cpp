/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Sema/Sema.h"
#include "clang/Basic/LangOptions.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Analysis/Analyses/Dominators.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <ctime>
#include <sstream>
#include <string>
#include "analyse_cfg.h"
#include "cdg.h"
#include <algorithm>
#include <chrono>
#include "commandOptions.h"
#include "ssaTime.h"
#include "cd2SSA.h"
#include "cdAnalysis.h"
#include "mergeReduction.h"
#include "storeResults.h"
#include "config.h"
#include "dfbasedPhiCalculation.h"

using namespace std;
using namespace std::chrono;
using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace AnalysisStatesConf;
//using namespace Conf;


class CFGVisitor : public RecursiveASTVisitor<CFGVisitor> {
public:
    CFGVisitor(ASTContext &C,unsigned dl,unsigned alg) : context(C) {_dl=dl;_algt=alg;}
    
    void TraverseCFG(std::unique_ptr<CFG> &cfg)
    {
    }
   
    void computeCDG(FunctionDecl *f, bool printOption)
    {
       //llvm::errs()<<"\n CDG of Func: "<<f->getNameInfo().getAsString()<<"\n";
       Stmt *funcBody = f->getBody();
       std::unique_ptr<CFG> sourceCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
       new CDG(std::move(sourceCFG),printOption);
    }
    
    // Compute phi functions using the method of the SCAM paper
    void computePhiRDBasedAnalysis(FunctionDecl *f)
    {
        // Analysis Based on the paper published in SCAM
        std::set<std::string> params;
        for(unsigned long i=0;i<f->param_size();i++)
            params.insert(f->getParamDecl(i)->getNameAsString());
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> sourceCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        unsigned size=sourceCFG->size();
        CFGAnalysis *cfgAnal=new CFGAnalysis(std::move(sourceCFG),context,params);
        std::map<unsigned,std::set<unsigned> > varDefs=cfgAnal->getNodesVarDefs();
        std::set<unsigned> vars=cfgAnal->getVars();
        ExecTime<std::chrono::microseconds> timer;
        cfgAnal->dataflow_analysis_fixpoint();
        cfgAnal->RD_analysis();
        unsigned long etime=timer.stop();
        std::map<unsigned,std::set<unsigned> > phis;
        std::map<unsigned,std::string> varnames;
        for(auto v:vars)
            varnames[v]=cfgAnal->getVarName(v);
        cfgAnal->exportPhiResults(phis);
        analInfo.storeInfo(f->getNameInfo().getAsString(),varDefs,vars,varnames,phis,etime,size,PhiRD);
    }
    
    //Cytron's Method
    void computePhiDFBasedAnalysis(FunctionDecl *f)
    {
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> cfg = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        unsigned size=cfg->size();
        std::set<std::string> pram;
        for(unsigned long i=0;i<f->param_size();i++)
            pram.insert(f->getParamDecl(i)->getNameAsString());
        
        CFGAnalysis *cfganal=new CFGAnalysis(std::move(cfg),context,pram);
        std::map<unsigned,std::set<unsigned> > varDefs=cfganal->getNodesVarDefs();
        std::set<unsigned> vars=cfganal->getVars();

        ExecTime<std::chrono::microseconds> timer;
        dfBasedPhiImpl phiCalc=dfBasedPhiImpl(context,&idToBlock);
        std::map<unsigned,std::set<unsigned> > phis;
        phiCalc.computePhifunctions(f,cfganal,phis);
        unsigned long etime=timer.stop();
        std::map<unsigned,std::string> varnames;
        for(auto v:vars)
            varnames[v]=cfganal->getVarName(v);
        analInfo.storeInfo(f->getNameInfo().getAsString(),varDefs,vars,varnames,phis,etime,size,PhiDF);
    }
  
    void computePhiDualityBasedAnalysis(FunctionDecl *f)
    {
        std::set<std::string> pram;
        for(unsigned long i=0;i<f->param_size();i++)
            pram.insert(f->getParamDecl(i)->getNameAsString());
        
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> cfg = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        unsigned size=cfg->size();
        CFGAnalysis *cfgAnal=new CFGAnalysis(std::move(cfg),context,pram);
        std::map<unsigned,std::set<unsigned> > varDefs=cfgAnal->getNodesVarDefs();
        std::set<unsigned> vars=cfgAnal->getVars();
        std::map<unsigned,std::string> varnames;
        for(auto v:vars)
            varnames[v]=cfgAnal->getVarName(v);
        
        ExecTime<std::chrono::microseconds> timer;
        CDSSA *ssa=new CDSSA(std::move(cfgAnal->getCFG()),context,true);
        unsigned long etime=timer.stop();
        auto phis=ssa->getPhiNodes(varDefs);
        analInfo.storeInfo(f->getNameInfo().getAsString(),varDefs,vars,varnames,phis,etime,size,PhiDual);
    }
    
    void ComputeAndComparePhiFunction(FunctionDecl *f)
    {
        std::map<unsigned,std::set<unsigned> > phiNodesRD,phiNodesDF,phiNodesSCAM, phiNodesCD;
        computePhiDFBasedAnalysis(f);
        computePhiRDBasedAnalysis(f);
        computePhiDualityBasedAnalysis(f);
    }
    
    void ComputeAndCompareCDComputation(FunctionDecl *f)
    {
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> sCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        std::set<unsigned> Np=getSubsetOfCFGNodes(sCFG->size());
        computeCDDanicic(f,&Np);
        computeCDSAS(f,&Np);
        computeCDDual(f,&Np);
    }
    
    std::set<unsigned> getSubsetOfCFGNodes(unsigned size)
    {
        std::srand (std::time (0));
        std::set<unsigned> vDefs;
        unsigned n=rand()%size+2;
        for(unsigned i=0;i<n;i++) {
            vDefs.insert(rand()%size);
        }
        return vDefs;
    }
    
    
    std::set<unsigned> getSubsetOfCFGNodesfromConsole(unsigned size)
    {
        std::set<unsigned> Np;
        outs()<<"\nRequires a set of CFG basic block id numbers (range from 0 to "<<size-1<<"):";
        std::string line;
        std::getline(std::cin, line);
        std::istringstream os(line);
        unsigned i;
        while(os >> i)
            if(i>=0 && i< size)
                Np.insert(i);
        return Np;
    }
    
    
    
    void computeCDDanicic(FunctionDecl *f,std::set<unsigned> *Npp=NULL)
    {
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> sCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        unsigned size=sCFG->size();
        std::set<unsigned> Np;
        if(!Npp){
            if(UserNP) Np=getSubsetOfCFGNodesfromConsole(sCFG->size());
            else Np=getSubsetOfCFGNodes(sCFG->size());
        }
        else Np=*Npp;
        ExecTime<std::chrono::microseconds> timer;
        CDAnalysisDanicic *cDanicic=new CDAnalysisDanicic(std::move(sCFG),context,Np);
        std::set<unsigned> wcc=cDanicic->computeWCCDanicic(Np);
        wcc.insert(Np.begin(),Np.end());
        unsigned long etime=timer.stop();
        cdAnalInfo.storeWCCInfo(f->getNameInfo().getAsString(),Np,wcc,etime,size,0);
    }
    
    void computeCDSAS(FunctionDecl *f,std::set<unsigned> *Npp=NULL)
    {
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> sCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        std::set<unsigned> Np;
        if(!Npp){
            if(UserNP) Np=getSubsetOfCFGNodesfromConsole(sCFG->size());
            else Np=getSubsetOfCFGNodes(sCFG->size());
        }
        else Np=*Npp;
        unsigned size=sCFG->size();
        ExecTime<std::chrono::microseconds> timer;
        CDAnalysis *cdanal=new CDAnalysis(std::move(sCFG),context,Np);
        std::set<unsigned> wcc=cdanal->computeWCCExt(Np);
        wcc.insert(Np.begin(),Np.end());
        unsigned long etime=timer.stop();
        cdAnalInfo.storeWCCInfo(f->getNameInfo().getAsString(),Np,wcc,etime,size,1);
    }
    
    void computeCDDual(FunctionDecl *f,std::set<unsigned> *Npp=NULL)
    {
        Stmt *funcBody = f->getBody();
        std::unique_ptr<CFG> sCFG = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
        std::set<unsigned> Np;
        if(!Npp){
            if(UserNP) Np=getSubsetOfCFGNodesfromConsole(sCFG->size());
            else Np=getSubsetOfCFGNodes(sCFG->size());
        }
        else Np=*Npp;
        unsigned size=sCFG->size();
        ExecTime<std::chrono::microseconds> timer;
        CDSSA *cd=new CDSSA(std::move(sCFG),context,false);
        std::set<unsigned> wcc=cd->getWCC(Np);
        wcc.insert(Np.begin(),Np.end());
        unsigned long etime=timer.stop();
        cdAnalInfo.storeWCCInfo(f->getNameInfo().getAsString(),Np,wcc,etime,size,2);
    }
    
    
    bool VisitFunctionDecl(FunctionDecl *f) {
        clang::SourceManager &sm(context.getSourceManager());
        if(!sm.isInMainFile(sm.getExpansionLoc(f->getBeginLoc())))
            return true;
        if(ProcName!="none" && f->getNameInfo().getAsString()!=ProcName) return true;
        
        switch(AlgType)
        {
            case CDDanicic: computeCDDanicic(f,NULL);
                            break;
            case CDSAS: computeCDSAS(f,NULL);
                        break;
            case CDDual: computeCDDual(f,NULL);
                         break;
            case PhiDF: computePhiDFBasedAnalysis(f);
                        break;
            case PhiRD: computePhiRDBasedAnalysis(f);
                        break;
            case PhiDual: computePhiDualityBasedAnalysis(f);
                          break;
            case CompCD: ComputeAndCompareCDComputation(f);
                         break;
            case CompPhi:ComputeAndComparePhiFunction(f);
                         break;
        }
        
        if(ProcName!="none" && DumpCFG)
        {
            Stmt *funcBody = f->getBody();
            std::unique_ptr<CFG> cfg = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
            //cfg->dump(LangOptions(),true);
            cfg->viewCFG(LangOptions());
        }
        
        if(ProcName!="none" && DumpCDG)
            computeCDG(f,true);
        
        return true;
    }

    

private:
    ASTContext &context;
    std::map<unsigned, DomTreeNodeBase< CFGBlock>*> idToBlock;
    unsigned _dl;
    unsigned _algt;
};


class CFGAstConsumer : public ASTConsumer {
public:
    CFGAstConsumer(ASTContext &C,unsigned dl,unsigned alg) : Visitor(C,dl,alg) {}
    
    bool HandleTopLevelDecl(DeclGroupRef DR) override {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
            Visitor.TraverseDecl(*b);
        }
        return true;
    }
    
private:
    CFGVisitor Visitor;
};


class CFGFrontendAction : public ASTFrontendAction {
    unsigned _dl;
    unsigned _algt;
 public:
    CFGFrontendAction(unsigned dl,unsigned alg) {_dl=dl;_algt=alg;}
    
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
        return make_unique<CFGAstConsumer>(CI.getASTContext(),_dl,_algt);
    }
};

class CFGFrontendActionFactory : public clang::tooling::FrontendActionFactory {
    unsigned _dl;
    unsigned _algt;
public:
    CFGFrontendActionFactory (unsigned dl)
    : _dl (dl)  { }
    
    virtual std::unique_ptr<clang::FrontendAction> create () {
        return std::make_unique<CFGFrontendAction> (_dl,_algt);
    }
};


int main(int argc,  const char **argv) {
    CommonOptionsParser op(argc, argv, MainCat);
    std::string Path,source_file_name;
    SplitFilename(argv[1],Path,source_file_name);
    
    // Set the debug level. Different internal informations are printed based on the choosen debug level. 0 - minimum debug information, 3 - maximum debug information
    switch(DL)
    {
        case O1: debugLevel=1;
                 break;
        case O2: debugLevel=2;
                 break;
        case O3: debugLevel=3;
                 break;
        default: debugLevel=0;
    }
    
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    CFGFrontendActionFactory cfgFact(debugLevel);
    Tool.run(&cfgFact);
    if(AlgType==PhiDF|| AlgType==PhiRD|| AlgType==PhiDual|| AlgType==CompPhi)
    {
      analInfo.setFileName(source_file_name);
      currentFile=source_file_name;
      analInfo.printAnalysisInfo(AlgType);
    }
    if(AlgType==CDDanicic|| AlgType==CDSAS|| AlgType==CDDual || AlgType==CompCD)
    {
      cdAnalInfo.setFileName(source_file_name);
      cdAnalInfo.printAnalysisInfo(AlgType);
    }
    return 1;
}
