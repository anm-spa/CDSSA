//
//  CDSSA.hpp
//  SmartSSA
//
//  Created by Abu Naser Masud on 2021-09-30.
//

#ifndef CDSSA_h
#define CDSSA_h

#include <algorithm>
#include <ctime>
#include <chrono>
//#include "ssaTime.h"
#include <iostream>
#include <fstream>
//#include "clang/Analysis/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/DenseMap.h"
#include <set>
#include <map>
#include <queue>

//using namespace std::chrono;
using namespace llvm;
//using namespace clang;


struct AdjBlockIter
{
    bool direction;
    AdjBlockIter(){}
    AdjBlockIter(bool dir)
   {
       direction=dir;
   }

    llvm::succ_range adj(llvm::BasicBlock *B)
    {
        return llvm::successors(B);
    }
    
    llvm::pred_range adjRev(llvm::BasicBlock *B)
     {
        return llvm::predecessors(B);
     }
};


class CDSSA
{
private:

    typedef std::pair<std::set<unsigned>,std::set<unsigned>> edgeType;
    llvm::BitVector DefVect,UseVect, LiveVect;
    bool computeSSA;
    llvm::Function *F;
    DenseMap<BasicBlock *, unsigned> BBNumbers;
    struct AdjBlockIter ABIter;
    unsigned size;
    std::map<unsigned, llvm::BasicBlock*> BBIds;
    std::set<unsigned> defs,uses,finalssa;
    bool liveness;
    
    unsigned getBlockID(llvm::BasicBlock* B){ return BBNumbers[B];}
    std::set<unsigned> overapproxWXFP();
    std::set<unsigned> overapproxWXFPOptimized();
    std::set<unsigned> checkReachability(llvm::BitVector wccPre,std::set<unsigned> Defs);
    std::set<unsigned> getReachableNodes(llvm::BitVector reachableNodes,std::set<unsigned> FromNodes);
    void backwardReachability(llvm::BitVector &vect, std::set<unsigned> &initNodes);

    std::set<unsigned> computeIncrementalWD(std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre,std::set<unsigned> AllX);
    bool existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    bool existInitWitnessEff(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    std::set<unsigned> excludeImpreciseResults(std::set<unsigned> wcc,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre,std::set<unsigned> *reqDefs=nullptr);
    std::set<std::pair<unsigned,unsigned> > findAllReachableEdges(std::set<unsigned> Vp);
    std::set<unsigned> filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma);
    std::set<unsigned>  overapproxWXFPHeu();
    
    
public:
    CDSSA(llvm::Function &Fs,DenseMap<BasicBlock *, unsigned> BBs,SmallVector<BasicBlock *, 32> Defs,  SmallPtrSet<BasicBlock *, 32> LiveNodes, bool computeSSA,bool liveanal);
    CDSSA(llvm::Function &Fs,DenseMap<BasicBlock *, unsigned> BBs,SmallVector<BasicBlock *, 32> Defs, SmallVector<BasicBlock *, 32> Uses, SmallPtrSet<BasicBlock *, 32> LiveNodes, bool computeSSA,bool liveanal);
    void getPhiNodes(SmallVector<BasicBlock *, 32> &PhiBlocks);
    std::set<unsigned> getWCC();
    std::set<unsigned> getSSA();
    void printGraph(std::map<unsigned, edgeType> graph);
    void printGraph();
    
};

#endif /* CDSSA_h */

