//
//  storeResults.h
//  LLVM
//
//  Created by Abu Naser Masud on 2022-02-17.
//

#ifndef storeResults_h
#define storeResults_h


#include "analyse_cfg.h"
#include "config.h"
#include "commandOptions.h"


class CFGAnalysis;

class AnalysisInfoType{
private:
    struct PhiStats
    {
        std::map<unsigned, std::set<unsigned>> phis;
        std::map<unsigned, std::set<unsigned>> defs;
        std::map<unsigned,std::string> varnames;
        std::set<unsigned> vars;
        unsigned long etime, cfgsize;
    };
    std::string FileName;
    std::map<std::string, struct PhiStats> PhiNodesDual, PhiNodesDF, PhiNodesRD;
    std::set<std::string> funcs;
public:
    AnalysisInfoType(){}
    void setFileName(std::string f) {FileName=f;}
    void storeInfo(std::string f,std::map<unsigned,std::set<unsigned> > vardefs,std::set<unsigned> vars, std::map<unsigned,std::string> vnames,std::map<unsigned, std::set<unsigned>> phis,unsigned long etime,unsigned size,unsigned alg)
    {
        PhiStats phistats;
        phistats.phis=phis;
        phistats.defs=vardefs;
        phistats.etime=etime;
        phistats.cfgsize=size;
        phistats.varnames=vnames;
        phistats.vars=vars;
        funcs.insert(f);
        if(alg==PhiDF)
            PhiNodesDF[f]=phistats;
        if(alg==PhiRD)
            PhiNodesRD[f]=phistats;
        if(alg==PhiDual)
            PhiNodesDual[f]=phistats;
    }
    
    bool comparePhiLists(std::string f)
    {
        bool nodiff=true;
        for(auto phirec:PhiNodesDual[f].phis)
        {
            if(PhiNodesRD[f].phis[phirec.first]!=phirec.second)
            {
                nodiff=false;
                break;
            }
        }
        for(auto phirec:PhiNodesRD[f].phis)
        {
            if(PhiNodesDual[f].phis[phirec.first]!=phirec.second)
            {
                nodiff=false;
                break;
            }
        }
        
        for(auto phirec:PhiNodesDual[f].phis)
        {
            if(PhiNodesDF[f].phis[phirec.first]!=phirec.second)
            {
                nodiff=false;
                break;
            }
        }
        for(auto phirec:PhiNodesDF[f].phis)
        {
            if(PhiNodesDual[f].phis[phirec.first]!=phirec.second)
            {
                nodiff=false;
                break;
            }
        }
        return nodiff;
    }
    
    void printPhiInfo(unsigned alg, std::string title="")
    {
        std::map<std::string, struct PhiStats> phiInfo;
        if(alg==PhiDF) phiInfo=PhiNodesDF;
        else if(alg==PhiRD) phiInfo=PhiNodesRD;
        else if(alg==PhiDual) phiInfo=PhiNodesDual;
        
        outs()<<"--------------------\n";
        outs()<<"Phi statistics ("<<title<<")\n";
        outs()<<"--------------------\n";
        for(auto rec:phiInfo)
        {
            outs()<<"Function: "<<rec.first<<"\n";
            for(auto vnum:rec.second.vars)
            {
                std::string v=rec.second.varnames[vnum];
                outs()<<"Variable: "<<v<<"\n";
                outs()<<"Def nodes of "<<v<<": ";
                auto defs=rec.second.defs[vnum];
                for(auto n:defs)
                    outs()<<n<<" ";
                outs()<<"\n";
                auto phis=rec.second.phis[vnum];
                outs()<<"Phi nodes for "<<v<<": ";
                for(auto n:phis)
                    outs()<<n<<" ";
                outs()<<"\n";
                
            }
            outs()<<"\nCFG size : "<<rec.second.cfgsize;
            outs()<<"\nExecution time : "<<rec.second.etime<<" ms\n";
            outs()<<"--------------------\n";
        }
        
    }
    
    void printAnalysisInfo(unsigned alg)
    {
        if(alg==PhiDF || (alg==PhiRD) || alg==PhiDual) printPhiInfo(alg);
        else printStatistics();
    }
    
    unsigned long numOfphis(std::string f, unsigned alg)
    {
        std::map<unsigned, std::set<unsigned>> phiInfo;
        if(alg==PhiDF) phiInfo=PhiNodesDF[f].phis;
        else if(alg==PhiRD) phiInfo=PhiNodesRD[f].phis;
        else if(alg==PhiDual) phiInfo=PhiNodesDual[f].phis;
        unsigned long nphi=0;
        for(auto rec: phiInfo)
            nphi+=rec.second.size();
        return nphi;
    }
    
    void printStatistics()
    {
        
        printPhiInfo(PhiDF, "Dominance Frontier-based method");
        printPhiInfo(PhiRD,"RD based Method [SCAM Paper]");
        printPhiInfo(PhiDual,"Duality based method");
        
        unsigned long N=0,tRD=0,tDF=0, tDual=0,nPhiDF=0,nPhiRD=0,nPhiDual=0;
        bool Res=true;
        std::string resstr;
        for(auto f:funcs)
        {
            bool isEqRes=comparePhiLists(f);
            N+=PhiNodesDual[f].cfgsize;
            tDual=tDual+PhiNodesDual[f].etime;
            tDF=tDF+PhiNodesDF[f].etime;
            tRD=tRD+PhiNodesRD[f].etime;
            nPhiDF+=numOfphis(f,PhiDF);
            nPhiRD+=numOfphis(f,PhiRD);
            nPhiDual+=numOfphis(f,PhiDual);

            Res= Res & isEqRes;
        }
        if(!Res)
            resstr="Result Differs!!!";
        else  resstr="Result Equal!!!";

        double nDF=nPhiDF,nRD=nPhiRD,nDual=nPhiDual;
        double perincDFDual=(double)(((nDF-nDual)/nDual)*100);
        double perincDFRD=(double)(((nDF-nRD)/nRD)*100);

        double tdFP=(double)(tDF/tDual);
        double tdScam=(double)(tRD/tDF);
        std::cout<<FileName<<", nCFG: "<<N<<"\n";
        std::cout<<"time (Duality Based): "<<tDual<<"\ntime (RD-based method): "<<tRD<<"\ntime (Dominance frontier-based method): "<<tDF<<"\ntimeDF/timeDual: "<<std::setprecision(4)<<tdFP<<"timeRD/timeDF: "<<std::setprecision(4)<<tdScam<<"\n#PhiNodesDual: "<<nPhiDual<<"\n#PhiNodesRD: "<<nPhiRD<<"\n#PhiNodesDF: "<<nPhiDF<<"\nPercentage of (DF-Dual)/Dual: "<<std::setprecision(4)<<perincDFDual<<"\nPercentage of (DF-RD)/RD: "<<std::setprecision(4)<<perincDFRD<<"\n"<<resstr<<"\n";
    }
    
};




class CDAnalysisInfoType{
private:
    struct WCCStats
    {
        std::set<unsigned> Np;
        std::set<unsigned> WCC;
        unsigned long etime, cfgsize;
    };
    std::string FileName;
    std::map<std::string, struct WCCStats> wccNodesDual, wccNodesSAS, wccNodesDanicic;
    std::set<std::string> funcs;
    bool compareWCCLists(std::string f);
    
public:

    CDAnalysisInfoType()
    {
    }
    void setFileName(std::string f) {FileName=f;}
    void storeWCCInfo(std::string,std::set<unsigned>, std::set<unsigned>, unsigned long, unsigned long, unsigned);
    void printStatistics();
    void printWCCInfo(unsigned alg, std::string title="");
    void printAnalysisInfo(unsigned alg)
    {
        if(alg==CDDanicic || (alg==CDSAS) || alg==CDDual) printWCCInfo(alg);
        else printStatistics();
    }

};

bool CDAnalysisInfoType::compareWCCLists(std::string f)
{
    bool nodiff=true;
    for(auto n:wccNodesDual[f].WCC)
      if(wccNodesDanicic[f].WCC.find(n)==wccNodesDanicic[f].WCC.end())
      {
        nodiff=false;
        break;
       }
    for(auto n:wccNodesDanicic[f].WCC)
      if(wccNodesDual[f].WCC.find(n)==wccNodesDual[f].WCC.end())
      {
        nodiff=false;
        break;
      }
    return nodiff;
}

void CDAnalysisInfoType::storeWCCInfo(std::string fname,std::set<unsigned> Np, std::set<unsigned> WCC, unsigned long etime, unsigned long cfgSize, unsigned alg)
{
    WCCStats wstats;
    wstats.Np=Np;
    wstats.WCC=WCC;
    wstats.etime=etime;
    wstats.cfgsize=cfgSize;
    
   if(alg==CDDanicic)
       wccNodesDanicic[fname]=wstats;
   else if(alg==CDSAS)
       wccNodesSAS[fname]=wstats;
   else if(alg==CDDual)
       wccNodesDual[fname]=wstats;
    funcs.insert(fname);
}

void CDAnalysisInfoType::printWCCInfo(unsigned alg, std::string title)
{
    std::map<std::string, struct WCCStats> wcc;
    if(alg==CDDanicic) wcc=wccNodesDanicic;
    else if(alg==CDSAS) wcc=wccNodesSAS;
    else if(alg==CDDual) wcc=wccNodesDual;
    outs()<<"--------------------\n";
    outs()<<"WCC statistics "<<title<<"\n";
    outs()<<"--------------------\n";
    for(auto rec:wcc)
    {
        outs()<<"Function: "<<rec.first<<"\n";
        outs()<<"Np: ";
        for(auto n:rec.second.Np)
            outs()<<n<<" ";
        outs()<<"\n";
        outs()<<"WCC: ";
        for(auto n:rec.second.WCC)
            outs()<<n<<" ";
        outs()<<"\nCFG size : "<<rec.second.cfgsize;
        outs()<<"\nExecution time : "<<rec.second.etime<<" ms\n";
        outs()<<"--------------------\n";
    }
    
}

void CDAnalysisInfoType::printStatistics()
{
    printWCCInfo(CDDanicic, "Danicic's method");
    printWCCInfo(CDSAS,"RD based analysis");
    printWCCInfo(CDDual,"Duality based method");
    
    bool Res=true;
    unsigned long N=0,tDanicic=0,tSAS=0,tDual=0;
    for(auto f:funcs)
    {
        bool isEqRes=compareWCCLists(f);
        N+=wccNodesDual[f].cfgsize;
        tSAS=tSAS+wccNodesSAS[f].etime;
        tDual=tDual+wccNodesDual[f].etime;
        tDanicic=tDanicic+wccNodesDanicic[f].etime;
        Res= Res & isEqRes;
        if(!isEqRes && debugLevel==DEBUGMAX)
        {
            llvm::errs()<<" ***Result Differs***\n";
            
            llvm::errs()<<"WCC (Duality-based method): ";
            for(auto n:wccNodesDual[f].WCC) llvm::errs()<<n<<" ";
            llvm::errs()<<"\n";
            
            llvm::errs()<<"WCC (SAS method): ";
            for(auto n:wccNodesSAS[f].WCC) llvm::errs()<<n<<" ";
            llvm::errs()<<"\n";
            
            llvm::errs()<<"Results of Danicic's method: ";
            for(auto n:wccNodesDanicic[f].WCC) llvm::errs()<<n<<" ";
            llvm::errs()<<"\n";
            
            llvm::errs()<<" Np Nodes: {";
            for(auto n:wccNodesDual[f].Np) llvm::errs()<<n<<", ";
            llvm::errs()<<"}\n";
        }
    }
    double dSAS=tSAS, dDual=tDual, dDanicic=tDanicic;
    double tdFP=(double)(dSAS/dDual);
    double tdDanicic=(double)(dDanicic/dDual);
    std::cout << std::setprecision(2) << std::fixed;
    if(Res)
       std::cout<<FileName<<" nCFG: "<<N<<" time (Duality-based): "<<tDual<<" time (SAS method): "<<tSAS<<" time (Danicic's method): "<<tDanicic<<" tSAS/tDual: "<<tdFP<<" tDanicic/tDual: "<<tdDanicic<<" OK \n";
    else
        std::cout<<FileName<<" nCFG: "<<N<<" time (Duality-based): "<<tDual<<" time (SAS method): "<<tSAS<<" time (Danicic's method): "<<tDanicic<<" tSAS/tDual: "<<tdFP<<" tDanicic/tDual: "<<tdDanicic<<" Not OK \n";
}

namespace AnalysisStatesConf
{
    //static unsigned debugLevel;
    static std::string currentFile;
    AnalysisInfoType analInfo;
    CDAnalysisInfoType cdAnalInfo;
}



#endif /* storeResults_h */
