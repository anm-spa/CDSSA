//
//  cdg.h
//  LLVM
//
//  Created by Abu Naser Masud on 2020-05-17.
//

#ifndef cdg_h
#define cdg_h

#include "clang/Analysis/Analyses/Dominators.h"
#include "commandOptions.h"
#include "config.h"

using namespace clang;
using namespace llvm;

class CDG
{
private:
    std::unique_ptr<clang::CFG> cfg;
    std::map<unsigned, std::set<unsigned>> dom;
    std::map<unsigned,CFGBlock*> cfgMap;
    CFGPostDomTree DT;
    
public:
   
    CDG(std::unique_ptr<clang::CFG> &&CFG, bool printOption);
    void computeDominatorRel();
};


CDG::CDG(std::unique_ptr<clang::CFG> &&CFG, bool printOption)
{
    cfg=std::move(CFG);
    DT.buildDominatorTree(cfg.get());
    ControlDependencyCalculator *  cdg=new ControlDependencyCalculator(cfg.get());
    computeDominatorRel();
    if(printOption) cdg->dump();
    if(debugLevel==DEBUGMAX) DT.dump();
}

void CDG::computeDominatorRel()
{
    CFGBlock eB=cfg->getEntry();
    std::set<unsigned> S,*S1=&S;
    for(auto *B:*cfg)
    {
        if(!B) continue;
        if(B->getBlockID()==eB.getBlockID()) continue;
        S.insert(B->getBlockID());
        dom[B->getBlockID()]=*S1;
        cfgMap[B->getBlockID()] = B;
    }
    cfgMap[eB.getBlockID()] =&eB;
    S1=&S;
    bool changed;
    do{
        changed=false;
        for(auto n:S)
        {
            std::set<unsigned> oldDom=dom[n], sCommon;
            CFGBlock *nB=cfgMap[n];
            bool first=true;
            for(auto pB:nB->preds())
            {
                if(!pB) continue;
                if(first) { sCommon=dom[pB->getBlockID()]; first=false;}
                else{
                    std::set<unsigned> intersect;
                    set_intersection(sCommon.begin(),sCommon.end(),dom[pB->getBlockID()].begin(),dom[pB->getBlockID()].end(),
                    std::inserter(intersect,intersect.begin()));
                    sCommon=intersect;
                }
            }
            dom[n]=sCommon;
            dom[n].insert(n);
            if(dom[n]!=oldDom) changed=true;
        }
    }while(changed);
    
  if(debugLevel==DEBUGMAX)
    for(auto *B:*cfg)
    {
        llvm::errs()<<B->getBlockID()<<" dominates: ";
        for(auto n:dom[B->getBlockID()])  llvm::errs()<<n<<" ";
        llvm::errs()<<"\n";
    }
    
}


#endif /* cdg_h */
