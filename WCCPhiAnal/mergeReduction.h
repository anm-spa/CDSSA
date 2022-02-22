//
//  mergeReduction.h
//  LLVM
//
//  Created by Abu Naser Masud on 2019-06-18.
//

#ifndef mergeReduction_h
#define mergeReduction_h

#include "clang/Analysis/CFG.h"
#include "dataFlowAnalysis.h"

#include <algorithm>
#include <vector>
#include <iterator>
#include <queue>

using namespace llvm;
using namespace clang;

class merge_reduction
{
    unsigned debug_level;
    unsigned maxNode;
    typedef std::map<unsigned,unsigned> EachVarDef;
    std::map<unsigned, std::set<unsigned>> graph;
    std::map<unsigned, llvm::BitVector> jNodeIsVarDefs;
    std::map<unsigned, EachVarDef> jNodeVarDefs;
    
public:
    merge_reduction(std::unique_ptr<CFG> &cfg, std::unique_ptr<CFGAnalysis> &cfg_anal, unsigned dl=0)
    {
        maxNode=cfg->size();
        debug_level=dl;
        CFGBlock &entry=cfg->getEntry();
        visitReachableBlocks(&entry,cfg_anal);
    }
    
    void visitReachableBlocks(clang::CFGBlock *B,std::unique_ptr<CFGAnalysis> &cfg_anal)
    {
        std::queue<std::pair<clang::CFGBlock*, clang::CFGBlock*>> wList;
        llvm::BitVector visited(maxNode);
        wList.push(std::pair<clang::CFGBlock*, clang::CFGBlock*>(B,B));
        EachVarDef varDefMap;
        llvm::errs()<<"var size: "<<cfg_anal->numberOfVars()<<"\n";
        llvm::BitVector nodeIsDefs(cfg_anal->numberOfVars());
        jNodeIsVarDefs[B->getBlockID()]=nodeIsDefs;
        jNodeVarDefs[B->getBlockID()]=varDefMap;
        while (!wList.empty()) {
            std::pair<clang::CFGBlock*, clang::CFGBlock*> bl=wList.front();
            wList.pop();
            CFGBlock *parent=bl.second;
            if(bl.first->pred_size()>1)
            {
                for(auto v:cfg_anal->getDefsOfBasicBlocks(bl.first->getBlockID()))
                {
                    varDefMap[v]=bl.first->getBlockID();
                    nodeIsDefs[v]=true;
                }
                graph[bl.second->getBlockID()].insert(bl.first->getBlockID());
                jNodeIsVarDefs[bl.second->getBlockID()]=nodeIsDefs;
                jNodeVarDefs[bl.second->getBlockID()]=varDefMap;
                nodeIsDefs.reset();
                varDefMap.clear();                
                parent=bl.first;
            }
            else{
                for(auto v:cfg_anal->getDefsOfBasicBlocks(bl.first->getBlockID()))
                {
                    varDefMap[v]=bl.first->getBlockID();
                    nodeIsDefs[v]=true;
                }
            }
            if(!visited[bl.first->getBlockID()])
            for (CFGBlock::const_succ_iterator I = bl.first->succ_begin(),
                 F = bl.first->succ_end(); I != F; ++I)
            {
                if(!(*I)) continue;
                wList.push(std::pair<clang::CFGBlock*,clang::CFGBlock*>(*I,parent));
            }
            visited[bl.first->getBlockID()]=true;
        }
    }
};

#endif /* mergeReduction_h */
