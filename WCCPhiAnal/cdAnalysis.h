//
//  cdAnalysis.h
//  LLVM
//
//  Created by Abu Naser Masud on 2020-04-17.
//

#ifndef cdAnalysis_h
#define cdAnalysis_h

#include <algorithm>
#include "dataFlowAnalysis.h"
#include <ctime>
#include <chrono>
#include "ssaTime.h"
#include <iostream>
#include <fstream>

using namespace std::chrono;
using namespace llvm;
using namespace clang;

class CFGInfo{
public:
    CFGInfo(ASTContext &C):context(C){varInfo=VarInfoT();}
    void WalkThroughBasicBlockStmts(CFGBlock *B);
    void GetVarsFromStmt(const Stmt *S,VarInfoInBlock &varBlock);
    bool WalkThroughExpr(const Expr * expr, std::set<unsigned> &Refs, std::set<unsigned> &globals);
    bool WalkThroughArraySubscriptExprDef(const ArraySubscriptExpr *ae, std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs, std::set<unsigned> &globalRefs);
    bool WalkThroughMemberExprDef(const MemberExpr *me, std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs, std::set<unsigned> &globalRefs);
    void VisitBinaryOp(const BinaryOperator *B,std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs,std::set<unsigned> &globalRefs);
    std::string findVar(const Expr *E);
    bool isGlobalVar(const Expr *E);
    const Expr *stripCasts(const Expr *Ex);
    std::set<unsigned> getVars();
    std::string getVarName(unsigned v);
    bool isVarDefindinBlock(unsigned v,unsigned b)
    {
        return BlockToVars[b].isDef(v);
    }
    
    void setStartNodesDefiningAllVars(unsigned startid)
    {
        BlockToVars[startid].BlockDefs.insert(vars.begin(),vars.end());
    }
    
private:
    clang::ASTContext &context;
    std::set<unsigned> vars;
    std::map<unsigned,VarInfoInBlock> BlockToVars;
    std::set<unsigned> gDefs, gRefs;
    VarInfoT varInfo;
   
};




void CFGInfo::WalkThroughBasicBlockStmts(CFGBlock *B)
{
    
    // llvm::errs()<<"Block "<<B->getBlockID()<<"\n";
    VarInfoInBlock VarsB;
    for(auto &Bi : *B) {
        std::set<unsigned> Def,Ref;
        switch (Bi.getKind()) {
            case CFGElement::Statement: {
                CFGStmt* S = reinterpret_cast<CFGStmt*>(&Bi);
                GetVarsFromStmt(S->getStmt(),VarsB);
                break;
            }
            default:
                break;
        }
    }
    vars.insert(VarsB.BlockDefs.begin(),VarsB.BlockDefs.end());
    vars.insert(VarsB.BlockRefs.begin(),VarsB.BlockRefs.end());
    BlockToVars[B->getBlockID()]=VarsB;
}


void CFGInfo::GetVarsFromStmt(const Stmt *S,VarInfoInBlock &varBlock)
{
    std::set<unsigned> Defs, Refs,globalDefs,globalRefs;
    if (const clang::BinaryOperator *bop=dyn_cast<clang::BinaryOperator>(S))
        VisitBinaryOp(bop,Defs,Refs,globalDefs,globalRefs);
    else if (const clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(S)){
        if(uop->isIncrementDecrementOp())
        {
            const clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
            std::set<unsigned> temp;
            WalkThroughExpr(subexp,temp,globalDefs);
            //Probably it can be made efficient
            Defs.insert(temp.begin(),temp.end());
            Refs.insert(temp.begin(),temp.end());
            globalRefs.insert(globalDefs.begin(),globalDefs.end());
            
        }
    }
    if (const CallExpr *call = dyn_cast<CallExpr>(S)) {
        for(unsigned i=0;i<call->getNumArgs();i++)
        {
            const Expr *expr = call->getArg(i);
            std::string s("scanf");
            const FunctionDecl * fd=call->getDirectCallee();
            if((fd) && (fd->getNameAsString()==s)){
                if(const clang::UnaryOperator *U=dyn_cast<clang::UnaryOperator>(expr))
                {
                    const clang::Expr *subexp=U->getSubExpr()->IgnoreImplicit();
                    WalkThroughExpr(subexp,Defs,globalDefs);
                }
            }
            else
            {
                WalkThroughExpr(call->getArg(i),Refs,globalRefs);
            }
        }
    }
    if (const DeclStmt *DS = dyn_cast<DeclStmt>(S)) {
        for (const auto *DI : DS->decls()) {
            const auto *V = dyn_cast<VarDecl>(DI);
            if (!V) continue;
            if (const Expr *E = V->getInit())
            {
                WalkThroughExpr(E,Refs,globalRefs);
                unsigned numVar=varInfo.mapsto(V->getNameAsString());
                Defs.insert(numVar);
            }
            
        }
    }
    
    if(const ReturnStmt *rtst = dyn_cast<ReturnStmt>(S)) {
        if(const Expr *retexpr=rtst->getRetValue()){
            //const Expr *retexpr=rtst->getRetValue()->IgnoreImplicit();
            WalkThroughExpr(retexpr,Refs,globalRefs);
        }
    }
    if (const IfStmt *ifstmt = dyn_cast<IfStmt>(S)) {
        const Expr *ifexp=ifstmt->getCond();
        std::set<std::string> Ref;
        if(ifexp)
            WalkThroughExpr(ifexp->IgnoreImplicit(),Refs,globalRefs);
    }
    varBlock.insertVarInfo(Defs,Refs);
    gDefs.insert(globalDefs.begin(),globalDefs.end());
    gRefs.insert(globalRefs.begin(),globalRefs.end());
}

bool CFGInfo::WalkThroughExpr(const Expr * expr, std::set<unsigned> &Refs, std::set<unsigned> &globals)
{
    if(!expr) return false;
    const Expr *exp=stripCasts(expr);
    if(exp->isIntegerConstantExpr(context,NULL)) return 1;
    if(const clang::DeclRefExpr *dexpr=dyn_cast<clang::DeclRefExpr>(exp))
    {
        if(const auto DV=dyn_cast<VarDecl>(dexpr->getDecl()))
        {
            unsigned numVar=varInfo.mapsto(DV->getNameAsString());
            Refs.insert(numVar);
            if(DV->hasLinkage()) globals.insert(numVar);
            return 1;
        }
    }
    
    /*if(clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
     {
     clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
     traverse_subExpr(subexp);
     return true;
     }*/
    
    if(const BinaryOperator *bop=dyn_cast<BinaryOperator>(exp))
    {
        const Expr *lhs=bop->getLHS();
        const Expr *rhs=bop->getRHS();
        WalkThroughExpr(lhs->IgnoreImplicit(),Refs,globals);
        WalkThroughExpr(rhs->IgnoreImplicit(),Refs,globals);
        return 1;
    }
    return 0;
}

bool CFGInfo::WalkThroughArraySubscriptExprDef(const ArraySubscriptExpr *ae, std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs, std::set<unsigned> &globalRefs)
{
    const Expr *Base=ae->getBase();
    const Expr *Id=ae->getIdx();
    std::string var=findVar(Base);
    unsigned numVar=varInfo.mapsto(var);
    Defs.insert(numVar);
    if(isGlobalVar(Base))
        globalDefs.insert(numVar);
    
    if (const clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(Id)){
        if(uop->isIncrementDecrementOp())
        {
            const clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
            std::set<unsigned> temp;
            WalkThroughExpr(subexp,temp,globalDefs);
            //Probably it can be made efficient
            Defs.insert(temp.begin(),temp.end());
            Refs.insert(temp.begin(),temp.end());
            globalRefs.insert(globalDefs.begin(),globalDefs.end());
            
        }
    }
    else WalkThroughExpr(Id,Refs,globalRefs);
    
    return true;
}

bool CFGInfo::WalkThroughMemberExprDef(const MemberExpr *me, std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs, std::set<unsigned> &globalRefs)
{
    //const MemberExpr *me=dyn_cast<MemberExpr>(stripCasts(Me));
    const Expr *Base=me->getBase();
    const ValueDecl *Vd=me->getMemberDecl();
    
    if (const clang::VarDecl *VarDecl=dyn_cast<clang::VarDecl>(Vd)){
        std::string varB=findVar(Base);
        std::string varF=Vd->getNameAsString();
        std::string var=varB+"@"+varF;
        unsigned numVar=varInfo.mapsto(var);
        Defs.insert(numVar);
        if(isGlobalVar(Base))
            globalDefs.insert(numVar);
        return true;
    }
    return false;
}

void CFGInfo::VisitBinaryOp(const BinaryOperator *B,std::set<unsigned> &Defs, std::set<unsigned> &Refs, std::set<unsigned> &globalDefs,std::set<unsigned> &globalRefs) {
    
    switch(B->getOpcode()){
        case BO_MulAssign:
        case BO_DivAssign:
        case BO_RemAssign:
        case BO_AddAssign:
        case BO_SubAssign:
        case BO_ShlAssign:
        case BO_ShrAssign:
        case BO_AndAssign:
        case BO_OrAssign:
        case BO_XorAssign: {
            std::set<unsigned> T;
            if(ArraySubscriptExpr *ae=dyn_cast<ArraySubscriptExpr>(B->getLHS()))
            {
                WalkThroughArraySubscriptExprDef(ae, T, Refs, globalDefs, globalRefs);
                Defs.insert(T.begin(),T.end());
                Refs.insert(T.begin(),T.end());
            }
            else if(MemberExpr *me=dyn_cast<MemberExpr>(B->getLHS())){
                WalkThroughMemberExprDef(me, T, Refs, globalDefs, globalRefs);
                Defs.insert(T.begin(),T.end());
                Refs.insert(T.begin(),T.end());
            }
            else if(const clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(B->getLHS()))
            {
                const clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
                std::set<unsigned> temp;
                WalkThroughExpr(subexp,temp,globalDefs);
                Defs.insert(temp.begin(),temp.end());
                Refs.insert(temp.begin(),temp.end());
                if(uop->isIncrementDecrementOp())
                {
                    globalRefs.insert(globalDefs.begin(),globalDefs.end());
                }
            }
            else{
                std::string var=findVar(B->getLHS());
                unsigned numVar=varInfo.mapsto(var);
                Defs.insert(numVar);
                Refs.insert(numVar);
                if(isGlobalVar(B->getLHS()))
                    globalDefs.insert(numVar);
            }
            WalkThroughExpr(B->getRHS(),Refs,globalRefs);
            break;
        }
        case BO_Assign:    {
            if(ArraySubscriptExpr *ae=dyn_cast<ArraySubscriptExpr>(B->getLHS()))
                WalkThroughArraySubscriptExprDef(ae, Defs, Refs, globalDefs, globalRefs);
            else if(MemberExpr *me=dyn_cast<MemberExpr>(B->getLHS()))
                WalkThroughMemberExprDef(me, Defs, Refs, globalDefs, globalRefs);
            else if(const clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(B->getLHS()))
            {
                const clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
                std::set<unsigned> temp;
                WalkThroughExpr(subexp,temp,globalDefs);
                Defs.insert(temp.begin(),temp.end());
                if(uop->isIncrementDecrementOp())
                {
                    Refs.insert(temp.begin(),temp.end());
                    globalRefs.insert(globalDefs.begin(),globalDefs.end());
                }
            }
            else{
                std::string var=findVar(B->getLHS());
                unsigned numVar=varInfo.mapsto(var);
                Defs.insert(numVar);
                if(isGlobalVar(B->getLHS()))
                    globalDefs.insert(numVar);
            }
            WalkThroughExpr(B->getRHS(),Refs,globalRefs);
            break;
        }
        default:{
            WalkThroughExpr(B->getLHS(),Refs,globalRefs);
            WalkThroughExpr(B->getRHS(),Refs,globalRefs);
            break;
        }
    }
}

std::string CFGInfo::findVar(const Expr *E) {
    E=stripCasts(E);
    if (const auto *DRE =dyn_cast<DeclRefExpr>(stripCasts(E)))
        return DRE->getDecl()->getNameAsString ();
    else
    {
        //llvm::errs()<<"Not a Valid Variable\n";
        //E->dump();
        //E->getExprLoc().dump(context.getSourceManager());
        return "not_a_valid_variable";}
}
bool CFGInfo::isGlobalVar(const Expr *E) {
    if (const auto *DRE =dyn_cast<DeclRefExpr>(stripCasts(E)))
        if(DRE->getDecl()->hasLinkage()) return true;
    return false;
}

const Expr *CFGInfo::stripCasts(const Expr *Ex) {
    while (Ex) {
        Ex=Ex->IgnoreImplicit();
        Ex = Ex->IgnoreParenNoopCasts(context);
        if (const auto *CE = dyn_cast<CastExpr>(Ex)) {
            if (CE->getCastKind() == CK_LValueBitCast) {
                Ex = CE->getSubExpr();
                continue;
            }
        }
        break;
    }
    return Ex;
}

std::set<unsigned> CFGInfo::getVars()
{
    return vars;
}

std::string CFGInfo::getVarName(unsigned v)
{
    return varInfo.mapsfrom(v);
}




class CDAnalysis
{
private:
    std::unique_ptr<clang::CFG> cfg;
    clang::ASTContext &context;
    typedef std::pair<std::set<unsigned>,std::set<unsigned>> edgeType;
    std::map<unsigned, edgeType> graphCDNet;
    std::map<unsigned,unsigned> graphRank;
    unsigned nodeSplitted;
    std::map<unsigned,unsigned> splitMap,splitMapRev;
    std::map<unsigned, CFGBlock*> CFGIdMap;
    std::set<unsigned> defs;
    llvm::BitVector DefVect, DefVectExt,LeaveVect;
    CFGInfo *cfgInfo;
    std::map<unsigned,std::set<unsigned>> varToPhiNodes;
    unsigned nextRank;
    std::set<unsigned> domTreeLeaves;
    
public:
    CDAnalysis(ASTContext &C):context(C){}
    CDAnalysis(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C);
    CDAnalysis(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, std::set<unsigned> vDefs);
    std::set<unsigned> getAllBlocksDefiningVar(unsigned v, CFGInfo *cfgInfo);
    void printGraph();
    void buildCDgraphNet(std::set<unsigned> vDefs);
    void buildextendedCFG(){}
    std::set<unsigned> checkReachability(llvm::BitVector wccPre,std::set<unsigned> Defs);
    void rankGraph(CFGDomTree *domTree);
    void rankGraph();
    std::set<unsigned> computeWCC(std::set<unsigned> Defs);
    std::set<unsigned> computeWCCExt(std::set<unsigned> Defs);
    std::set<unsigned>  computeIncrementalWD(std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre,std::set<unsigned> AllX);
    std::set<unsigned>  computeWD(std::set<unsigned> Defs);
    std::set<unsigned> computeGamma(std::set<unsigned> Np, llvm::BitVector &V);
    std::set<unsigned> computeSCC(std::set<unsigned> Np);
    std::set<unsigned> computeSCCNonIncremental(std::set<unsigned> Np);
    std::set<unsigned> computeTheta(std::set<unsigned> Vp, unsigned u);
    std::set<std::pair<unsigned,unsigned> > findAllReachableEdges(std::set<unsigned> Vp);
    std::set<unsigned> filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma);
    std::set<unsigned> computeSSAExt();
    std::set<unsigned> computeSSAExtension();
    void computePhiNodes();
    void genVarDefs(std::map<unsigned,std::set<unsigned>> &varDefs);
    void getPhiNodeInfo(std::map<unsigned,std::set<unsigned> > &phiNodesGR);
    void rankSuccessors(DomTreeNode *node);
    void buildDomTree(CFGDomTree *DT);
    void buildTree(DomTreeNode *rootNode);
    void addCFGEdgesToDomTree();
    void transferCFG();
    void printGraph(std::map<unsigned, edgeType> graph);
    bool existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    bool existInitWitnessEff(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    llvm::BitVector excludeImpreciseResults(std::set<unsigned> wcc,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre);
};

CDAnalysis::CDAnalysis(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C):context(C)
{
    cfg=std::move(CFG);
    cfgInfo=new CFGInfo(C);
    for (auto *Block : *cfg){
        cfgInfo->WalkThroughBasicBlockStmts(Block);
        CFGIdMap[Block->getBlockID()]=Block;
    }
    nodeSplitted=0;
    cfgInfo->setStartNodesDefiningAllVars((cfg->getEntry()).getBlockID());
}

void CDAnalysis::genVarDefs(std::map<unsigned,std::set<unsigned>> &varDefs)
{
    
    for(auto *b:*cfg)
    {
        for(auto v:cfgInfo->getVars())
        {
           if(cfgInfo->isVarDefindinBlock(v,(*b).getBlockID()))
            varDefs[v].insert((*b).getBlockID());
        }
    }
}

void CDAnalysis::getPhiNodeInfo(std::map<unsigned,std::set<unsigned> > &phiNodesGR)
{
    for(auto v:cfgInfo->getVars())
    {
        for(auto n: varToPhiNodes[v])
            phiNodesGR[n].insert(v);
    }
}

void CDAnalysis::computePhiNodes()
{
    std::map<unsigned,std::set<unsigned>> varDefs;
    genVarDefs(varDefs);
    DefVect=llvm::BitVector(cfg->size());
    DefVectExt=llvm::BitVector(cfg->size()+nodeSplitted);
    unsigned v=8;
    {
        llvm::errs()<<"Analysing Var: "<<cfgInfo->getVarName(v)<<" ("<<v<<")\n";
        DefVect.reset();
        DefVectExt.reset();
        for(auto n:varDefs[v])
            DefVect[n]=true;
        
        buildextendedCFG();
        defs.clear();
        for(auto z:varDefs[v])
        {
            if(splitMapRev.find(z)!=splitMapRev.end())
            {
                defs.insert(splitMapRev[z]);
                //defs.insert(v);
                DefVectExt[splitMapRev[z]]=true;
            }
            else {
                defs.insert(z);
                DefVectExt[z]=true;
            }
        }
        
        rankGraph();
        printGraph();
        varToPhiNodes[v]=computeSSAExtension();
    }
}


CDAnalysis::CDAnalysis(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, std::set<unsigned> vDefs):context(C)
{
    cfg=std::move(CFG);
    for (auto *Block : *cfg){
        CFGIdMap[Block->getBlockID()]=Block;
    }
    DefVect=llvm::BitVector(cfg->size());
    for(auto v:vDefs)
        DefVect[v]=true;
   // transferCFG();
}

void  CDAnalysis::addCFGEdgesToDomTree()
{
    std::map<unsigned, edgeType>::iterator it=graphCDNet.begin();
    while(it!=graphCDNet.end())
    {
        unsigned n=it->first;
        CFGBlock* B=CFGIdMap[n];
         if(!B) continue;
        if(LeaveVect[n] || ((it->second).first.size()<2 && B->succ_size()>1))
        {
            CFGBlock::succ_iterator ib=B->succ_begin();
            CFGBlock::succ_iterator ie=B->succ_end();
            for(;ib!=ie;ib++){
                if(!(*ib)) continue;
                graphCDNet[n].first.insert((*ib)->getBlockID());
                graphCDNet[(*ib)->getBlockID()].second.insert(n);
            }
        }
        it++;
    }
}


void CDAnalysis::transferCFG()
{
    for(auto *B:*cfg)
    {
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            graphCDNet[B->getBlockID()].first.insert((*ib)->getBlockID());
            graphCDNet[(*ib)->getBlockID()].second.insert(B->getBlockID());
        }
    }
}

void CDAnalysis::buildTree(DomTreeNode *node)
{
    if(!node) return;
    unsigned n=(node->getBlock())->getBlockID();
   // llvm::errs()<<"Node "<<i<<"has rank "<<nextRank-1<<"\n";
    unsigned numChild=0;
    auto it=node->begin();
    while(it!=node->end())
    {
        graphCDNet[n].first.insert(((*it)->getBlock())->getBlockID());
        graphCDNet[((*it)->getBlock())->getBlockID()].second.insert(n);
        
        buildTree(*it);
        it++;
        numChild++;
    }
    if(!numChild){
        domTreeLeaves.insert(n);
        LeaveVect[n]=true;
    }
}
void CDAnalysis::buildDomTree(CFGDomTree *DT)
{
     DomTreeNode *rootNode=DT->getRootNode();
     buildTree(rootNode);
}



void CDAnalysis::rankGraph()
{
    CFGBlock *eB=&(cfg->getEntry());
    std::queue<unsigned> W;
  //  std::stack<unsigned> W;
    W.push(eB->getBlockID());
    llvm::BitVector V(cfg->size()+nodeSplitted);
    nextRank=0;
    while(!W.empty())
    {
        unsigned n=W.front();
        //unsigned n=W.top();
        W.pop();
        if(!V[n])
        {
            graphRank[n]=nextRank++;
            for(auto m:graphCDNet[n].first)
            {
                W.push(m);
            }
            V[n]=true;
        }
    }
}

void CDAnalysis::rankSuccessors(DomTreeNode *node)
{
    if(!node) return;
    unsigned i;
    //if(node->getBlock())
        i=(node->getBlock())->getBlockID();
    graphRank[i]=nextRank++;
    llvm::errs()<<"Node "<<i<<"has rank "<<nextRank-1<<"\n";
    auto it=node->begin();
    while(it!=node->end())
    {
        rankSuccessors(*it);
        it++;
    }
}

void CDAnalysis::rankGraph(CFGDomTree *DT)
{
    DomTreeNode *rootNode=DT->getRootNode();
    rankSuccessors(rootNode);
}



void CDAnalysis::buildCDgraphNet(std::set<unsigned> vDefs)
{
    CFGBlock *eB=&(cfg->getEntry());
    
    std::queue<std::pair<CFGBlock *,CFGBlock *>> W;
    W.push(std::pair<CFGBlock *,CFGBlock *>(eB,eB));
    llvm::BitVector V(cfg->size());
    llvm::BitVector Nd(cfg->size());
  //  V[eB->getBlockID()]=true;
    graphCDNet[eB->getBlockID()]=edgeType(std::set<unsigned>(),std::set<unsigned>());
    Nd[eB->getBlockID()]=true;
    while(!W.empty())
    {
        std::pair<CFGBlock *,CFGBlock *> pr=W.front();
        W.pop();
        CFGBlock * toFlow=pr.second;
        //llvm::errs()<<"Visiting: "<<pr.first->getBlockID()<<"---"<<pr.second->getBlockID()<<"\n";
        if(V[pr.first->getBlockID()]) continue;
        V[pr.first->getBlockID()]=true;
        if((pr.first->succ_size()>1) && (vDefs.find(pr.first->getBlockID())!=vDefs.end()))
        {
            nodeSplitted++;
            unsigned nn=cfg->size()+nodeSplitted;
            Nd.resize(nn+1);
            V.resize(nn+1);
            graphCDNet[pr.second->getBlockID()].first.insert(nn);
            if(!Nd[pr.first->getBlockID()]){ graphCDNet[pr.first->getBlockID()]=edgeType(std::set<unsigned>(),std::set<unsigned>());
                Nd[pr.first->getBlockID()]=true;
            }
            if(!Nd[nn]){
                graphCDNet[nn]=edgeType(std::set<unsigned>(),std::set<unsigned>());
                Nd[nn]=true;
            }
            graphCDNet[pr.first->getBlockID()].second.insert(nn);
            graphCDNet[nn].first.insert(pr.first->getBlockID());
            graphCDNet[nn].second.insert(pr.second->getBlockID());
            
            if(splitMap.find(nn)==splitMap.end())
                splitMap[nn]=pr.first->getBlockID();
            toFlow=pr.first;
        }
        else if((pr.first->succ_size()>1) || (vDefs.find(pr.first->getBlockID())!=vDefs.end()))
        {
            graphCDNet[pr.second->getBlockID()].first.insert(pr.first->getBlockID());
            if(!Nd[pr.first->getBlockID()]){ graphCDNet[pr.first->getBlockID()]=edgeType(std::set<unsigned>(),std::set<unsigned>());
                Nd[pr.first->getBlockID()]=true;
            }
            graphCDNet[pr.first->getBlockID()].second.insert(pr.second->getBlockID());
            toFlow=pr.first;
            
        }
        if(pr.first){
            CFGBlock::succ_iterator ib=pr.first->succ_begin();
            CFGBlock::succ_iterator ie=pr.first->succ_end();
            for(;ib!=ie;ib++){
                if(*ib)
                    //if(!V[(*ib)->getBlockID()] || pr.first->succ_size() < 2) {
                        W.push(std::pair<CFGBlock *,CFGBlock *>(*ib,toFlow));
                        //V[(*ib)->getBlockID()]=true;
                    }
            }
    }
    nodeSplitted++;
}


std::set<unsigned> CDAnalysis::checkReachability(llvm::BitVector wccPre, std::set<unsigned> Defs)
{
    std::queue<unsigned> Q;
    for(auto n:Defs){
        Q.push(n);
    }
    
    std::set<unsigned> wcc;
    llvm::BitVector V(cfg->size());
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V[n]=true;
       // llvm::errs()<<"Visiting node "<<n->getBlockID()<<"\n";
        if(wccPre[n]) wcc.insert(n);
        
        CFGBlock* B=CFGIdMap[n];
        if(!B) continue;
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            if(!V[(*ib)->getBlockID()])
                Q.push((*ib)->getBlockID());
        
        }
    }
    return wcc;
}

std::set<unsigned> CDAnalysis::getAllBlocksDefiningVar(unsigned v,CFGInfo *cfgInfo)
{
    std::set<unsigned> Defs;
    for(auto *b:*cfg)
    {
        if(cfgInfo->isVarDefindinBlock(v,(*b).getBlockID()))
            Defs.insert((*b).getBlockID());
    }
    return Defs;
}


std::set<unsigned>  CDAnalysis::computeWCCExt(std::set<unsigned> Defs)
{
    //ExecTime<std::chrono::microseconds> timer1;
   
    std::queue<unsigned> Q;
    std::map<unsigned,unsigned > Pre;
    unsigned UNINITVAL=cfg->size()+10;
    std::set<unsigned> wcc;
    llvm::BitVector V(cfg->size());
   // llvm::BitVector wccNode(cfg->size());
 
   
    for(auto *B:*cfg)
    {
        if(!B) continue;
        Pre[B->getBlockID()]=UNINITVAL;
    }
    
    for(auto n:Defs)
    {
        Pre[n]=n;
        Q.push(n);
        V[n]=true;
    }
    
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        
        CFGBlock *B=CFGIdMap[currNode];
        CFGBlock::pred_iterator pb=B->pred_begin();
        CFGBlock::pred_iterator pe=B->pred_end();
       
        for(;pb!=pe;pb++){
            if(!(*pb)) continue;
            unsigned n=(*pb)->getBlockID();
          //  llvm::errs()<<"\nVisiting edge: "<<currNode<< " <- "<<n<<"\n";
            
            std::set<unsigned> S;
            CFGBlock::succ_iterator ib=(*pb)->succ_begin();
            CFGBlock::succ_iterator ie=(*pb)->succ_end();
            for(;ib!=ie;ib++){
                if(!(*ib)) continue;
                unsigned m=(*ib)->getBlockID();
            //    llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
                if(Pre[m]==UNINITVAL)
                    continue;
                else S.insert(Pre[m]);
            }
            
            if(S.size()>1)
            {
                
                if(!V[n] && Pre[n]!=n) //&& elemValue[n]!=n
                {
                    Q.push(n);
                    V[n]=true;
                }
                Pre[n]=n;
                wcc.insert(n);
             //   wccNode[n]=true;
                
            }
            else if(S.size()<2)
            {
                unsigned oldVal=Pre[n];
                if(DefVect[n] || Pre[n]==n) continue;
                else Pre[n]=*S.begin();
                if(!V[n] && oldVal!=Pre[n]){
                    Q.push(n);
                    V[n]=true;
                }
            }
        }
    }
    
  /*  for(auto *B:*cfg){
        unsigned m=B->getBlockID();
         llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        CFGBlock *B=CFGIdMap[n];
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    llvm::BitVector wccNode=excludeImpreciseResults(wcc,Defs,Pre);
    std::set<unsigned> wccFinal=checkReachability(wccNode,Defs);
    
    
  /*  for(auto *B:*cfg){
        unsigned m=B->getBlockID();
        llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        CFGBlock *B=CFGIdMap[n];
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    return wccFinal;
}

std::set<unsigned>  CDAnalysis::computeWD(std::set<unsigned> Defs)
{
    //ExecTime<std::chrono::microseconds> timer1;
   
    std::queue<unsigned> Q;
    std::map<unsigned,unsigned > Pre;
    unsigned UNINITVAL=cfg->size()+10;
    std::set<unsigned> wcc;
    llvm::BitVector V(cfg->size());
   // llvm::BitVector wccNode(cfg->size());
 
   
    for(auto *B:*cfg)
    {
        if(!B) continue;
        Pre[B->getBlockID()]=UNINITVAL;
    }
    
    for(auto n:Defs)
    {
        Pre[n]=n;
        Q.push(n);
        V[n]=true;
    }
    
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        
        CFGBlock *B=CFGIdMap[currNode];
        CFGBlock::pred_iterator pb=B->pred_begin();
        CFGBlock::pred_iterator pe=B->pred_end();
       
        for(;pb!=pe;pb++){
            if(!(*pb)) continue;
            unsigned n=(*pb)->getBlockID();
          //  llvm::errs()<<"\nVisiting edge: "<<currNode<< " <- "<<n<<"\n";
            
            std::set<unsigned> S;
            CFGBlock::succ_iterator ib=(*pb)->succ_begin();
            CFGBlock::succ_iterator ie=(*pb)->succ_end();
            for(;ib!=ie;ib++){
                if(!(*ib)) continue;
                unsigned m=(*ib)->getBlockID();
            //    llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
                if(Pre[m]==UNINITVAL)
                    continue;
                else S.insert(Pre[m]);
            }
            
            if(S.size()>1)
            {
                
                if(!V[n] && Pre[n]!=n) //&& elemValue[n]!=n
                {
                    Q.push(n);
                    V[n]=true;
                }
                Pre[n]=n;
                wcc.insert(n);
             //   wccNode[n]=true;
                
            }
            else if(S.size()<2)
            {
                unsigned oldVal=Pre[n];
                if(DefVect[n] || Pre[n]==n) continue;
                else Pre[n]=*S.begin();
                if(!V[n] && oldVal!=Pre[n]){
                    Q.push(n);
                    V[n]=true;
                }
            }
        }
    }
    
  /*  for(auto *B:*cfg){
        unsigned m=B->getBlockID();
         llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        CFGBlock *B=CFGIdMap[n];
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    llvm::BitVector wccNode=excludeImpreciseResults(wcc,Defs,Pre);
    std::set<unsigned> wccFinal=checkReachability(wccNode,Defs);
    
    
  /*  for(auto *B:*cfg){
        unsigned m=B->getBlockID();
        llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        CFGBlock *B=CFGIdMap[n];
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    return wccFinal;
}



std::set<unsigned>  CDAnalysis::computeIncrementalWD(std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre, std::set<unsigned> AllX)
{
   
    std::queue<unsigned> Q;
    
    unsigned UNINITVAL=cfg->size()+10;
    std::set<unsigned> wcc;
    llvm::BitVector V(cfg->size());
    // llvm::BitVector wccNode(cfg->size());
    
    
    
    for(auto n:Defs)
    {
        Pre[n]=n;
        Q.push(n);
        V[n]=true;
    }
    
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        
        CFGBlock *B=CFGIdMap[currNode];
        CFGBlock::pred_iterator pb=B->pred_begin();
        CFGBlock::pred_iterator pe=B->pred_end();
        
        for(;pb!=pe;pb++){
            if(!(*pb)) continue;
            unsigned n=(*pb)->getBlockID();
           // llvm::errs()<<"Visiting edge: "<<currNode<< " <- "<<n<<"\n";
            
            std::set<unsigned> S;
            CFGBlock::succ_iterator ib=(*pb)->succ_begin();
            CFGBlock::succ_iterator ie=(*pb)->succ_end();
            for(;ib!=ie;ib++){
                if(!(*ib)) continue;
                unsigned m=(*ib)->getBlockID();
                if(Pre[m]==UNINITVAL)
                    continue;
                else S.insert(Pre[m]);
            }
            
            if(S.size()>1)
            {
                
                if(!V[n] && Pre[n]!=n) //&& elemValue[n]!=n
                {
                    Q.push(n);
                    V[n]=true;
                }
                Pre[n]=n;
                wcc.insert(n);
                //   wccNode[n]=true;
                
            }
            else if(S.size()<2)
            {
                unsigned oldVal=Pre[n];
                if(Pre[n]==n) continue;
                else Pre[n]=*S.begin();
                if(!V[n] && oldVal!=Pre[n]){
                    Q.push(n);
                    V[n]=true;
                }
            }
        }
    }
    
    llvm::BitVector wccNode=excludeImpreciseResults(wcc,Defs,Pre);
    std::set<unsigned> wccFinal=checkReachability(wccNode,AllX);
    /*
     llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
     for(auto n:wccFinal)
     {
     if(DefVect[n]) continue;
     std::set<unsigned> S;
     
     CFGBlock *B=CFGIdMap[n];
     CFGBlock::succ_iterator ib=B->succ_begin();
     CFGBlock::succ_iterator ie=B->succ_end();
     for(;ib!=ie;ib++){
     if(!(*ib)) continue;
     unsigned m=(*ib)->getBlockID();
     if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
     }
     llvm::errs()<<n<<" (";
     for(auto x:S) llvm::errs()<<x<<", ";
     llvm::errs()<<")\n";
     }*/
    
    return wccFinal;
}



llvm::BitVector CDAnalysis::excludeImpreciseResults(std::set<unsigned> wcc,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre)
{
    std::map<unsigned, unsigned> Acc;
    std::map<unsigned,bool> isFinal;
    std::map<unsigned, edgeType> solnGraph;
    unsigned UNINITVAL=cfg->size()+10;
    llvm::BitVector V1(cfg->size());
    llvm::BitVector wccNode(cfg->size());
    std::queue<unsigned> Q;
    unsigned counter=0;
    
    for(auto n:wcc)
    {
        isFinal[n]=false;
        wccNode[n]=false;
        
        CFGBlock *B=CFGIdMap[n];
        CFGBlock::succ_iterator ib=B->succ_begin();
        CFGBlock::succ_iterator ie=B->succ_end();
        //for(auto m:graphCDNet[n].first)
        for(;ib!=ie;ib++)
            // for(auto m:graphCDNet[n].first)
        {
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL)
            {
                solnGraph[n].first.insert(Pre[m]);
                solnGraph[Pre[m]].second.insert(n);
            }
        }
        Acc[n]=UNINITVAL;
    }
    
    for(auto n:Defs){
        Acc[n]=n;
        isFinal[n]=true;
        wccNode[n]=false;
        for(auto m:solnGraph[n].second){
            if(!V1[m]) {
                Q.push(m);
                V1[m]=true;
            }
        }
    }
    
   // printGraph(solnGraph);
    
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V1[n]=false;
        std::set<unsigned> S;
        unsigned solvedNode;
        std::set<unsigned> unsolvedNode;
        for(auto m:solnGraph[n].first){
            if(Acc[m]!=UNINITVAL) {S.insert(Acc[m]);solvedNode=Acc[m];}
            else unsolvedNode.insert(m);
        }
        
        if(S.size()>1)
        {
            Acc[n]=n;
            isFinal[n]=true;
            if(!DefVect[n])
                wccNode[n]=true;
            //llvm::errs()<<n<<" is included\n";
        }
        else  // if(existUninit)
        {
            if(existInitWitness(solnGraph,unsolvedNode,solvedNode,wccNode))
            {
                Acc[n]=n;
                isFinal[n]=true;
                if(!DefVect[n])
                    wccNode[n]=true;
                // llvm::errs()<<n<<" is included\n";
            }
            else
            {
                isFinal[n]=true;
                Acc[n]=Acc[solvedNode];
                Pre[n]=Acc[solvedNode];   // ******************Added for SCC computation
                // llvm::errs()<<n<<" is excluded\n";
            }
        }
        for(auto m:solnGraph[n].second){
            if(!V1[m] && !isFinal[m]) {
                Q.push(m);
                V1[m]=true;
            }
        }
    }

    return wccNode;
}
    

bool CDAnalysis::existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res){
     llvm::BitVector V(cfg->size());
     std::stack<unsigned> W;
     for(auto n:startNodes)
       W.push(n);
     while(!W.empty())
     {
         unsigned n=W.top();
         W.pop();
         V[n]=true;
         if((DefVect[n] || Res[n]) && n!=diffFrom) {return true;}
         if(DefVect[n] || Res[n]) continue;
         for(auto m:solnGraph[n].first){
             if(!V[m]) W.push(m);
         }
     }
    return false;
}

bool CDAnalysis::existInitWitnessEff(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res){
     llvm::BitVector V(cfg->size());
     std::stack<unsigned> W;
     for(auto n:startNodes)
       W.push(n);
     while(!W.empty())
     {
         unsigned n=W.top();
         W.pop();
         V[n]=true;
         if((DefVect[n] || Res[n]) && n!=diffFrom) {return true;}
         if(DefVect[n] || Res[n]) continue;
         for(auto m:solnGraph[n].first){
             if(!V[m]) W.push(m);
         }
     }
    return false;
}

std::set<unsigned> CDAnalysis::computeTheta(std::set<unsigned> Vp, unsigned u)
{
    CFGBlock *ublock;
    if(CFGIdMap.find(u)!=CFGIdMap.end())
        ublock=CFGIdMap[u];
    else assert("Node not found" && false);
    llvm::BitVector Vect(cfg->size());
    llvm::BitVector V(cfg->size());
    llvm::BitVector inQ(cfg->size());
    for(auto n:Vp)
        Vect[n]=true;
    std::queue<CFGBlock *> Q;
    std::set<unsigned> Res;
    Q.push(ublock);
    inQ[ublock->getBlockID()]=true;
    while(!Q.empty())
    {
        CFGBlock *n=Q.front();
        Q.pop();
        inQ[n->getBlockID()]=false;
        if(Vect[n->getBlockID()])  Res.insert(n->getBlockID());
        V[n->getBlockID()]=true;
        CFGBlock::succ_iterator ib=n->succ_begin();
        CFGBlock::succ_iterator ie=n->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            if(Vect[n->getBlockID()]) continue;
            if(!V[(*ib)->getBlockID()])  {Q.push(*ib);inQ[(*ib)->getBlockID()]=true;}
        }
    }
    return Res;
}

std::set<std::pair<unsigned,unsigned> > CDAnalysis::findAllReachableEdges(std::set<unsigned> Vp)
{
    std::queue<CFGBlock *> Q;
    llvm::BitVector inQ(cfg->size());
    for(auto u:Vp){
        //if(CFGIdMap.find(u)==CFGIdMap.end())
        //  continue;
        CFGBlock* uB= CFGIdMap[u];
        if(!uB) continue;
        CFGBlock::succ_iterator ib=uB->succ_begin();
        CFGBlock::succ_iterator ie=uB->succ_end();
        for(;ib!=ie;ib++){
            if(*ib){
                Q.push(CFGIdMap[(*ib)->getBlockID()]);
                inQ[(*ib)->getBlockID()]=true;
            }
        }
    }
    
    llvm::BitVector V(cfg->size());
    std::set<std::pair<unsigned,unsigned> > Res;
    
    while(!Q.empty())
    {
        CFGBlock *n=Q.front();
        Q.pop();
        inQ[n->getBlockID()]=false;
        V[n->getBlockID()]=true;
        CFGBlock::succ_iterator ib=n->succ_begin();
        CFGBlock::succ_iterator ie=n->succ_end();
        for(;ib!=ie;ib++){
            if(*ib)  {
                Res.insert(std::pair<unsigned,unsigned>(n->getBlockID(),(*ib)->getBlockID()));
                if(!V[(*ib)->getBlockID()]){
                    Q.push(*ib);
                    inQ[(*ib)->getBlockID()]=true;
                }
            }
        }
    }
    return Res;
}

std::set<unsigned> CDAnalysis::filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma)
{
    llvm::BitVector inGamma(cfg->size());
    llvm::BitVector visit(cfg->size());
    for(auto n:Gamma)
        inGamma[n]=true;
    std::set<unsigned> Y(X),GammaR;
    while(!Y.empty())
    {
        unsigned n=*Y.begin();
        Y.erase(Y.begin());
        visit[n]=true;
        if(inGamma[n]) GammaR.insert(n);
        CFGBlock *B=CFGIdMap[n];
        if(!B) continue;
        for(auto pB:B->succs())
        {
            if(!(pB)) continue;
            if(!visit[pB->getBlockID()]) Y.insert(pB->getBlockID());
        }
    }
    return GammaR;
}

std::set<unsigned> CDAnalysis::computeGamma(std::set<unsigned> Np, llvm::BitVector &inX)
{
    std::set<unsigned> X(Np),Gamma;
    while(!X.empty())
    {
        unsigned n=*X.begin();
        X.erase(X.begin());
        inX[n]=true;
        CFGBlock *B=CFGIdMap[n];
        for(auto pB:B->preds()){
            if(!(pB)) continue;
            bool succ=true;
            for(auto sPB:(*pB).succs())
            {
                if(!(sPB)) continue;
                if(!inX[(*sPB).getBlockID()]) {succ=false;break;}
            }
            if(succ && !inX[(*pB).getBlockID()]) X.insert((*pB).getBlockID());
        }
    }
    for(auto *B:*cfg)
    {
        if(!inX[B->getBlockID()]) Gamma.insert(B->getBlockID());
    }
    return Gamma;
}

std::set<unsigned> CDAnalysis::computeSCC(std::set<unsigned> Np)
{
    
    std::set<unsigned> scc,X={};
    llvm::BitVector inX(cfg->size());
    bool changed=true;
    std::set<unsigned> Xp(Np),WD={};
    std::map<unsigned,unsigned > Pre;
    unsigned UNINITVAL=cfg->size()+10;
    
    for(auto *B:*cfg)
    {
        if(!B) continue;
        Pre[B->getBlockID()]=UNINITVAL;
    }
    
    while(changed){
        changed=false;
        std::set<unsigned> GammaPre=computeGamma(Xp,inX);
        X.insert(Xp.begin(),Xp.end());
        std::set<unsigned> WDAux=computeIncrementalWD(Xp,Pre,X);
        WD.insert(WDAux.begin(),WDAux.end());
        Xp={};
        std::set<unsigned> Gamma=filterOutUnreachablenodes(X,GammaPre);
        //std::set<unsigned> WD=computeWCCExt(X);
        for(auto p:WD)
        {
            CFGBlock *B=CFGIdMap[p];
            for(auto rB:B->succs())
            {
                if(!(rB)) continue;
                if(inX[rB->getBlockID()]) {
                    scc.insert(p);
                    Xp.insert(p);
                    changed=true;
                    break;
                }
            }
        }
        for(auto x:Xp)
           WD.erase(x);
        for(auto p:Gamma)
        {
            CFGBlock *B=CFGIdMap[p];
            for(auto rB:B->succs())
            {
                if(!(rB)) continue;
                if(inX[rB->getBlockID()]) {
                    scc.insert(p);
                    Xp.insert(p);
                    changed=true;
                    break;
                }
            }
        }
        for(auto n:Xp) DefVect[n]=true;
    }
    
    /*
    
    std::set<unsigned> X(Np), Y,Gamma;
    bool hasFound;
    llvm::BitVector inX(cfg->size());
    std::set<unsigned> Xp(Np);
    do{
        hasFound=false;
        std::set<std::pair<unsigned,unsigned> > edges=findAllReachableEdges(X);
        for(auto e:edges)  ////for(auto *B:*cfg)
        {
            // if(!isReachable(B->getBlockID(),X)) continue;
            // llvm::errs()<<"Edges "<<e.first<<" -> "<<e.second<<"\n";
           
            Gamma=computeGamma(Xp,inX);
            X.insert(Xp.begin(),Xp.end());
            Xp={};
            std::set<unsigned> S1,S2;
            S1=computeTheta(X,e.second);
            S2=computeTheta(X,e.first);
            if(inX[e.second] && S1.size()==1)
                if(S2.size()>=2 || !inX[e.first]){
                    Xp.insert(e.first);
                    Y.insert(e.first);
                    hasFound=true;
                }
        }
    }while(hasFound);
    
    return Y;
     */
    
    return scc;
}


std::set<unsigned> CDAnalysis::computeSCCNonIncremental(std::set<unsigned> Np)
{
    
    bool changed=true;
    std::set<unsigned> SCC(Np);
    std::map<unsigned,unsigned > Pre;
    unsigned UNINITVAL=cfg->size()+10;

    
    while(true){
        llvm::BitVector inX(cfg->size());
        std::set<unsigned> WD=computeWD(SCC);
        std::set<unsigned> Gamma=computeGamma(SCC,inX);
        std::set<unsigned> X,Y,Z;
        X.insert(WD.begin(),WD.end());
        X.insert(Gamma.begin(),Gamma.end());
        for(auto n:SCC)
            if(X.find(n)!=X.end())
                X.erase(n);
        Y=filterOutUnreachablenodes(SCC,X);
        //std::set<unsigned> WD=computeWCCExt(X);
        for(auto p:Y)
        {
            CFGBlock *B=CFGIdMap[p];
            for(auto rB:B->succs())
            {
                if(!(rB)) continue;
                if(inX[rB->getBlockID()]) {
                    Z.insert(p);
                    break;
                }
            }
        }
        for(auto n:Z) DefVect[n]=true;
        SCC.insert(Z.begin(),Z.end());
        if(Z.empty()) break;
    }
    
    return SCC;
}



// This function is still under development
std::set<unsigned>  CDAnalysis::computeSSAExt()
{
    struct T{
        unsigned node;
        unsigned rank;
    };
    
    auto comp = [](struct T left, struct T right)
    { return (left.rank) > (right.rank); };
    std::priority_queue<struct T,std::vector< struct T>,decltype(comp) > Q(comp);
    
    std::map<unsigned,unsigned > Pre,Post;
    unsigned UNINITVAL=cfg->size()+nodeSplitted+11;
    llvm::BitVector V(cfg->size()+nodeSplitted);
    llvm::BitVector phiNode(cfg->size()+nodeSplitted);
    std::set<unsigned> wccTmp,ssa;
    for(auto it=graphCDNet.begin();it!=graphCDNet.end();it++)
    {
        Pre[(*it).first]=UNINITVAL;
        Post[(*it).first]=UNINITVAL;
    }
    for(auto n:defs)
    {
        Post[n]=n;
        struct T elem={n,graphRank[n]};
        Q.push(elem);
        V[elem.node]=true;
    }
    
    
    while(!Q.empty())
    {
        struct T elem=Q.top();
        Q.pop();
        V[elem.node]=false;
        
        for(auto n:graphCDNet[elem.node].first)    // for all predecessor node
        {
           llvm::errs()<<"Visiting edge: "<<elem.node<< " <- "<<n<<"\n";
            
            if(Pre[n]==UNINITVAL)
            {
                Pre[n]=Post[elem.node];
                if(Post[n]!=n)
                    Post[n]=Pre[n];
                if(!V[n] && Post[n]!=n)
                {
                    Q.push({n,graphRank[n]});
                    V[n]=true;
                }
            }
            
            else
            {
                std::set<unsigned> S;
                for(auto m:graphCDNet[n].second)
                {
                    if(Post[m]!=UNINITVAL && !((Post[m]==n) && !DefVect[n])) S.insert(Post[m]);
                }
                if(S.size()>1 && !phiNode[n])
                {
                    Post[n]=n;
                    if(!V[n]) //&& elemValue[n]!=n
                    {
                        Q.push({n,graphRank[n]});
                        V[n]=true;
                    }
                    ssa.insert(n);
                    phiNode[n]=true;
                    if(n>cfg->size()) wccTmp.insert(n);
                    llvm::errs()<<"===> Node "<<n<<": ";
                    for(auto x:S) llvm::errs()<<x<<" ";
                    llvm::errs()<<"\n";
                }
                else if(S.size()>1 && phiNode[n]) {}
                else if(S.size()<=1 && phiNode[n])  // to be included in wcc
                {
                    phiNode[n]=false;
                   // graphRank[n]=nextRank++;
                    if(!DefVect[n])
                       Post[n]=*S.begin();
                    if(!V[n]) //&& elemValue[n]!=n
                    {
                        Q.push({n,graphRank[n]});
                        V[n]=true;
                    }
                    llvm::errs()<<"===> Node "<<n<<"removed as phi node ";
                    ssa.erase(n);
                }
               
                else
                {
                    unsigned oldVal=Post[n];
                    if(!S.empty()){
                        Pre[n]=*S.begin();
                        if(Post[n]!=n)
                            Post[n]=*S.begin();
                        }
                    if(!V[n] && Post[n]!=n && (Post[n]!=oldVal))  // This condition may introduce error
                     {
                     Q.push({n,graphRank[n]});
                     V[n]=true;
                     }
                }
            }
        }
    }
    
    for(auto n:wccTmp)
    {
        unsigned i=splitMap[n];
        phiNode[i]=true;
    }
  //  std::set<unsigned> wcc=checkReachability(wccPre);
    
    llvm::errs()<<"\n SSA nodes are (in graph ranking based method): \n";
    for(auto n:ssa)
    {
        //if(DefVect[n]) continue;
        std::set<unsigned> S;
        for(auto m:graphCDNet[n].second)
        {
            if(Post[m]!=UNINITVAL) S.insert(Post[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }
    return ssa;
}


std::set<unsigned>  CDAnalysis::computeSSAExtension()
{
    struct T{
        unsigned node;
        unsigned rank;
    };
    
    auto comp = [](struct T left, struct T right)
    { return (left.rank) > (right.rank); };
    std::priority_queue<struct T,std::vector< struct T>,decltype(comp) > Q(comp);
    
    std::map<unsigned,unsigned > Pre,Post,Replaced;
    unsigned UNINITVAL=cfg->size()+nodeSplitted+11;
    llvm::BitVector V(cfg->size()+nodeSplitted);
    llvm::BitVector phiNode(cfg->size()+nodeSplitted);
    std::set<unsigned> wccTmp,ssa;
    for(auto it=graphCDNet.begin();it!=graphCDNet.end();it++)
    {
        Pre[(*it).first]=UNINITVAL;
        Replaced[(*it).first]=UNINITVAL;
        Post[(*it).first]=UNINITVAL;
    }
    for(auto n:defs)
    {
        Post[n]=n;
        struct T elem={n,graphRank[n]};
        Q.push(elem);
        V[elem.node]=true;
    }
    
    
    while(!Q.empty())
    {
        struct T elem=Q.top();
        Q.pop();
        V[elem.node]=false;
        
        for(auto n:graphCDNet[elem.node].first)    // for all predecessor node
        {
            llvm::errs()<<"Visiting edge: "<<elem.node<< " <- "<<n<<"\n";
            
            if(Pre[n]==UNINITVAL)
            {
                Pre[n]=Post[elem.node];
                if(Post[n]!=n)
                    Post[n]=Pre[n];
                if(!V[n] && Post[n]!=n)
                {
                    Q.push({n,graphRank[n]});
                    V[n]=true;
                }
            }
            
            else
            {
                std::set<unsigned> S,R;
                for(auto m:graphCDNet[n].second)
                {
                    if(Post[m]!=UNINITVAL && !((Post[m]==n) && !DefVect[n]))
                    {
                        S.insert(Post[m]);
                        if(Replaced[n]!=UNINITVAL) R.insert(Replaced[n]);
                    }
                }
                for(auto m:R) S.erase(m);
                
                if(S.size()>1 && !phiNode[n])
                {
                    //Replaced[n]=Post[n];
                    Post[n]=n;
                    if(!V[n]) //&& elemValue[n]!=n
                    {
                        Q.push({n,graphRank[n]});
                        V[n]=true;
                    }
                    ssa.insert(n);
                    phiNode[n]=true;
                    if(n>cfg->size()) wccTmp.insert(n);
                    llvm::errs()<<"===> Node "<<n<<": ";
                    for(auto x:S) llvm::errs()<<x<<" ";
                    llvm::errs()<<"\n";
                }
                else if(S.size()>1 && phiNode[n]) {}
                else if(S.size()<=1 && phiNode[n])  // to be included in wcc
                {
                    phiNode[n]=false;
                    // graphRank[n]=nextRank++;
                  //  Replaced[n]=Post[n];
                    if(!DefVect[n])
                        Post[n]=*S.begin();
                    if(!V[n]) //&& elemValue[n]!=n
                    {
                        Q.push({n,graphRank[n]});
                        V[n]=true;
                    }
                    llvm::errs()<<"===> Node "<<n<<"removed as phi node ";
                    ssa.erase(n);
                }
                
                else
                {
                    unsigned oldVal=Post[n];
                    if(!S.empty()){
                        Pre[n]=*S.begin();
                        if(Post[n]!=n)
                            Post[n]=*S.begin();
                    }
                    if(!V[n] && Post[n]!=n && (Post[n]!=oldVal))  // This condition may introduce error
                    {
                       // Q.push({n,graphRank[n]});
                        Q.push({n,0});
                        V[n]=true;
                        Replaced[n]=oldVal;
                    }
                }
            }
        }
    }
    
    for(auto n:wccTmp)
    {
        unsigned i=splitMap[n];
        phiNode[i]=true;
    }
    //  std::set<unsigned> wcc=checkReachability(wccPre);
    
    llvm::errs()<<"\n SSA nodes are (in graph ranking based method): \n";
    for(auto n:ssa)
    {
        //if(DefVect[n]) continue;
        std::set<unsigned> S;
        for(auto m:graphCDNet[n].second)
        {
            if(Post[m]!=UNINITVAL) S.insert(Post[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }
    return ssa;
}


void CDAnalysis::printGraph()
{
    CFGBlock* src=&(cfg->getEntry());
    std::queue<CFGBlock*> W;
    W.push(src);
    llvm::BitVector V(cfg->size());
    V[src->getBlockID()]=true;
    std::ofstream fout ("/tmp/temp.dot");
    //fout << endl
    //<< "-----file_with_graph_G begins here-----" << endl
    fout<< "digraph G {" << endl;
   // fout<<"  node [shape=circle];"<<endl;
    map<unsigned,string> nodes;
   // std::map<unsigned, edgeType>::iterator itb=graphCDNet.begin();
    //while(itb!=graphCDNet.end())
    for(auto *B:*cfg)
    {
        //if(DefVect[itb->first])
          //  fout <<itb->first<< "[fontcolor=blue label ="<<"\"" << itb->first<<"  | rank: "<<graphRank[itb->first]<<"  \""<<"]"<<endl;
        if(DefVect[B->getBlockID()])
            fout <<B->getBlockID()<< "[fontcolor=red label ="<<"\"" << B->getBlockID()<<" \""<<"]"<<endl;
        else
            fout <<B->getBlockID()<< "[label ="<<"\"" << B->getBlockID()<<"  \""<<"]"<<endl;
    }
        
    while(!W.empty())
    {
        CFGBlock* x=W.front();
        W.pop();
        CFGBlock::succ_iterator ib=x->succ_begin();
        CFGBlock::succ_iterator ie=x->succ_end();
        //for(auto m:graphCDNet[n].first)
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
                fout << x->getBlockID() <<" -> " << (*ib)->getBlockID() <<endl;
            if(!V[(*ib)->getBlockID() ]) {V[(*ib)->getBlockID() ]=true; W.push(*ib);}
        }
    }
    fout << "}" << endl;
   system("open /tmp/temp.dot");
}

void CDAnalysis::printGraph(std::map<unsigned, edgeType> graph)
{
    unsigned src=(cfg->getEntry()).getBlockID();
    std::queue<unsigned> W;
    W.push(src);
    llvm::BitVector V(cfg->size());
    V[src]=true;
    std::ofstream fout ("/tmp/soln.dot");
    //fout << endl
    //<< "-----file_with_graph_G begins here-----" << endl
    fout<< "digraph G {" << endl;
    // fout<<"  node [shape=circle];"<<endl;
    map<unsigned,string> nodes;
    std::map<unsigned, edgeType>::iterator itb=graph.begin();
    while(itb!=graph.end())
    {
        //if(DefVect[itb->first])
        //  fout <<itb->first<< "[fontcolor=blue label ="<<"\"" << itb->first<<"  | rank: "<<graphRank[itb->first]<<"  \""<<"]"<<endl;
        if(DefVect[itb->first])
            fout <<itb->first<< "[fontcolor=red label ="<<"\"" << itb->first<<" \""<<"]"<<endl;
        else
            fout <<itb->first<< "[label ="<<"\"" << itb->first<<"  \""<<"]"<<endl;
        itb++;
    }
    
    itb=graph.begin();
    while(itb!=graph.end())
    {
        for(auto n:itb->second.first)
            fout << itb->first <<" -> " << n<<endl;
        itb++;
    }
    fout << "}" << endl;
    system("open /tmp/soln.dot");
}


class CDAnalysisDanicic
{
private:
    std::unique_ptr<clang::CFG> cfg;
    clang::ASTContext &context;
    typedef std::pair<std::set<unsigned>,std::set<unsigned>> edgeType;
    std::map<unsigned, edgeType> graphCDNet;
    std::map<unsigned, CFGBlock*> CFGIdMap;
   
    
public:
    CDAnalysisDanicic(ASTContext &C):context(C){}
    CDAnalysisDanicic(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, std::set<unsigned> vDefs);
    std::set<unsigned> getAllBlocksDefiningVar(unsigned v, CFGInfo *cfgInfo);
    //void printGraph(std::set<unsigned>);
    std::set<unsigned> computeTheta(std::set<unsigned> Vp, unsigned u);
    std::set<std::pair<unsigned,unsigned> > findAllReachableEdges(std::set<unsigned> Vp);
    bool isReachable(unsigned p, std::set<unsigned> X);
    std::set<unsigned> computeWCCDanicic(std::set<unsigned> Vp);
    std::set<unsigned> computeSCCDanicic(std::set<unsigned> Vp);
    std::set<unsigned> computeGamma(std::set<unsigned> Vp, llvm::BitVector &inX);
    std::set<unsigned> computeGammaD(std::set<unsigned> Vp, llvm::BitVector &inX);
};


CDAnalysisDanicic::CDAnalysisDanicic(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, std::set<unsigned> vDefs):context(C)
{
    cfg=std::move(CFG);
  //  CFGInfo *cfgInfo=new CFGInfo(C);
    for (auto *Block : *cfg){
       // cfgInfo->WalkThroughBasicBlockStmts(Block);
        CFGIdMap[Block->getBlockID()]=Block;
    }
    
    // for debugging
    
    /*for(auto v:cfgInfo->getVars())
    {
        llvm::errs()<<"Var "<<v<<" Name: "<<cfgInfo->getVarName(v)<<"\n";
    }
    std::set<unsigned> vDefs=getAllBlocksDefiningVar(0,cfgInfo);
     */
    
    // std::set<unsigned> vDefs={3,13,19,21,31,35};
    
    //for(auto b:vDefs) llvm::errs()<<"Block "<<b<<" ";
    
    //computeWCCDanicic(vDefs);
}

std::set<unsigned> CDAnalysisDanicic::computeTheta(std::set<unsigned> Vp, unsigned u)
{
    CFGBlock *ublock;
    if(CFGIdMap.find(u)!=CFGIdMap.end())
        ublock=CFGIdMap[u];
    else assert("Node not found" && false);
    llvm::BitVector Vect(cfg->size());
    llvm::BitVector V(cfg->size());
    llvm::BitVector inQ(cfg->size());
    for(auto n:Vp)
        Vect[n]=true;
    std::queue<CFGBlock *> Q;
    std::set<unsigned> Res;
    Q.push(ublock);
    inQ[ublock->getBlockID()]=true;
    while(!Q.empty())
    {
        CFGBlock *n=Q.front();
        Q.pop();
        inQ[n->getBlockID()]=false;
        if(Vect[n->getBlockID()])  Res.insert(n->getBlockID());
        V[n->getBlockID()]=true;
        CFGBlock::succ_iterator ib=n->succ_begin();
        CFGBlock::succ_iterator ie=n->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            if(Vect[n->getBlockID()]) continue;
            if(!V[(*ib)->getBlockID()])  {Q.push(*ib);inQ[(*ib)->getBlockID()]=true;}
        }
    }
    return Res;
}

std::set<std::pair<unsigned,unsigned> > CDAnalysisDanicic::findAllReachableEdges(std::set<unsigned> Vp)
{
    std::queue<CFGBlock *> Q;
    llvm::BitVector inQ(cfg->size());
    for(auto u:Vp){
        //if(CFGIdMap.find(u)==CFGIdMap.end())
          //  continue;
        CFGBlock* uB= CFGIdMap[u];
        if(!uB) continue;
        CFGBlock::succ_iterator ib=uB->succ_begin();
        CFGBlock::succ_iterator ie=uB->succ_end();
        for(;ib!=ie;ib++){
            if(*ib){
                Q.push(CFGIdMap[(*ib)->getBlockID()]);
                inQ[(*ib)->getBlockID()]=true;
               }
            }
    }
    
    llvm::BitVector V(cfg->size());
    std::set<std::pair<unsigned,unsigned> > Res;
    
    while(!Q.empty())
    {
        CFGBlock *n=Q.front();
        Q.pop();
        inQ[n->getBlockID()]=false;
        V[n->getBlockID()]=true;
        CFGBlock::succ_iterator ib=n->succ_begin();
        CFGBlock::succ_iterator ie=n->succ_end();
        for(;ib!=ie;ib++){
            if(*ib)  {
                Res.insert(std::pair<unsigned,unsigned>(n->getBlockID(),(*ib)->getBlockID()));
                if(!V[(*ib)->getBlockID()]){
                    Q.push(*ib);
                    inQ[(*ib)->getBlockID()]=true;
                }
            }
        }
    }
    return Res;
}

bool CDAnalysisDanicic::isReachable(unsigned p, std::set<unsigned> X)
{
    std::queue<CFGBlock *> Q;
    llvm::BitVector inQ(cfg->size());
    llvm::BitVector inX(cfg->size());
    llvm::BitVector visit(cfg->size());
    for(auto u:X){
        inX[u]=true;
    }
    
    Q.push(CFGIdMap[p]);
    inQ[p]=true;
    
    while(!Q.empty())
    {
        CFGBlock *n=Q.front();
        Q.pop();
        inQ[n->getBlockID()]=false;
        visit[n->getBlockID()]=true;
        if(inX[n->getBlockID()]) return true;
        CFGBlock::pred_iterator ib=n->pred_begin();
        CFGBlock::pred_iterator ie=n->pred_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            if(!visit[(*ib)->getBlockID()] && !inQ[(*ib)->getBlockID()]){
                Q.push(*ib);
                inQ[(*ib)->getBlockID()]=true;
            }
        }
    }
    return false;
}



std::set<unsigned> CDAnalysisDanicic::computeWCCDanicic(std::set<unsigned> Vp)
{
    std::set<unsigned> X(Vp), Y;
   // std::set<std::pair<unsigned,unsigned> > edges=findAllReachableEdges(Vp);
    bool hasFound;
    do{
        hasFound=false;
        std::set<std::pair<unsigned,unsigned> > edges=findAllReachableEdges(X);
        for(auto e:edges)
        {
           // llvm::errs()<<"Edges "<<e.first<<" -> "<<e.second<<"\n";
            std::set<unsigned> S1,S2;
            S1=computeTheta(X,e.second);
            S2=computeTheta(X,e.first);
            if(S1.size()==1 && S2.size()>=2){
                X.insert(e.first);
                Y.insert(e.first);
                hasFound=true;
            }
        }
    }while(hasFound);
    
  /*  llvm::errs()<<"\n Weak control closure nodes are (Danicic's method): \n";
    for(auto n:Y)
    {
        llvm::errs()<<n<<", ";
    }
    llvm::errs()<<"\n";*/
    return Y;
}

std::set<unsigned> CDAnalysisDanicic::computeGamma(std::set<unsigned> Np, llvm::BitVector &inX)
{
    std::set<unsigned> X=Np;
    std::queue<CFGBlock *> Q;
    llvm::BitVector inQ(cfg->size());
    llvm::BitVector visit(cfg->size());
    
    for(auto u:Np){
        CFGBlock* uB= CFGIdMap[u];
        if(!uB) continue;
        Q.push(uB);
        inQ[u]=true;
        inX[u]=true;
    }
    
    while(!Q.empty())
    {
        CFGBlock* B=Q.front();
        Q.pop();
        inQ[B->getBlockID()]=false;
        visit[B->getBlockID()]=true;
        CFGBlock::pred_iterator pb=B->pred_begin();
        CFGBlock::pred_iterator pe=B->pred_end();
        for(;pb!=pe;pb++)
        {
            if(!(*pb)) continue;
            bool succ=true;
            for(auto nB:(*pb)->succs())
            {
                if(!nB) continue;
                if(!inX[nB->getBlockID()]) {succ=false;break;}
            }
            if(succ){
                inX[(*pb)->getBlockID()]=true;
                if(!inQ[(*pb)->getBlockID()] && !visit[(*pb)->getBlockID()]) Q.push(*pb);
            }
        } //endfor
    } // end while
    std::set<unsigned> Gamma;
    for(auto *B:*cfg)
    {
        if(!inX[B->getBlockID()]) Gamma.insert(B->getBlockID());
    }
    return Gamma;
}

std::set<unsigned> CDAnalysisDanicic::computeGammaD(std::set<unsigned> Np, llvm::BitVector &inX)
{
    
    std::map<unsigned, std::set<unsigned>> G;
     std::set<unsigned> Gamma;
    for(auto *B:*cfg)
    {
        if(!B) continue;
        for(auto sB:B->succs())
        {
            if(!sB) continue;
            G[B->getBlockID()].insert((*sB).getBlockID());
        }
    }
    
    for(auto u:Np){
        inX[u]=true;
    }
    std::set<unsigned> X=Np;
    bool changed=true;
    while(changed){
        changed=false;
        for(auto it:G)
        {
            if(inX[it.first]) continue;
            //if(!it.second.empty())
            std::set<unsigned> temp;
            for(auto x:it.second)
            {
                if(inX[x]) temp.insert(x);
            }
            for(auto x:temp)
               it.second.erase(x);
            
            if(it.second.size()==0)    // if it.second.size()==0, then it.first is a nonpredicate
            {
               // Gamma.insert(it.first);
                //if(!inX[it.first])
                  changed=true;
                  inX[it.first]=true;
            }
        }
    }
    
    /*
    std::queue<CFGBlock *> Q;
    llvm::BitVector inQ(cfg->size());
    llvm::BitVector visit(cfg->size());
    
    for(auto u:Np){
        CFGBlock* uB= CFGIdMap[u];
        if(!uB) continue;
        Q.push(uB);
        inQ[u]=true;
        inX[u]=true;
    }
    
    
    
    while(!Q.empty())
    {
        CFGBlock* B=Q.front();
        Q.pop();
        inQ[B->getBlockID()]=false;
        visit[B->getBlockID()]=true;
        CFGBlock::pred_iterator pb=B->pred_begin();
        CFGBlock::pred_iterator pe=B->pred_end();
        for(;pb!=pe;pb++)
        {
            if(!(*pb)) continue;
            bool succ=true;
            for(auto nB:(*pb)->succs())
            {
                if(!nB) continue;
                if(!inX[nB->getBlockID()]) {succ=false;break;}
            }
            if(succ){
                inX[(*pb)->getBlockID()]=true;
                if(!inQ[(*pb)->getBlockID()] && !visit[(*pb)->getBlockID()]) Q.push(*pb);
            }
        } //endfor
    } // end while
   */
    for(auto *B:*cfg)
    {
        if(!inX[B->getBlockID()]) Gamma.insert(B->getBlockID());
    }
    return Gamma;
}

std::set<unsigned> CDAnalysisDanicic::computeSCCDanicic(std::set<unsigned> Vp)
{
    std::set<unsigned> X(Vp), Y,Gamma;
    bool hasFound;
    do{
        hasFound=false;
        std::set<std::pair<unsigned,unsigned> > edges=findAllReachableEdges(X);
        for(auto e:edges)  ////for(auto *B:*cfg)
        {
           // if(!isReachable(B->getBlockID(),X)) continue;
            // llvm::errs()<<"Edges "<<e.first<<" -> "<<e.second<<"\n";
            llvm::BitVector inX(cfg->size());
            Gamma=computeGammaD(X,inX);
            std::set<unsigned> S1,S2;
            S1=computeTheta(X,e.second);
            S2=computeTheta(X,e.first);
            if(inX[e.second] && S1.size()==1)
            if(S2.size()>=2 || !inX[e.first]){
                X.insert(e.first);
                Y.insert(e.first);
                hasFound=true;
            }
        }
    }while(hasFound);
    
    return Y;
}

std::set<unsigned> CDAnalysisDanicic::getAllBlocksDefiningVar(unsigned v,CFGInfo *cfgInfo)
{
    std::set<unsigned> Defs;
    for(auto *b:*cfg)
    {
        if(cfgInfo->isVarDefindinBlock(v,(*b).getBlockID()))
            Defs.insert((*b).getBlockID());
    }
    return Defs;
}


#endif /* cdAnalysis_h */
