//
//  VerifySSA.hpp
//  SmartSSA
//
//  Created by Abu Naser Masud on 2021-10-12.
//

#ifndef VerifySSA_h
#define VerifySSA_h

#include "llvm/IR/Function.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/CFG.h"

#include <iostream>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <algorithm>
#include <sstream>



using namespace llvm;

class VerifyPhiNodes
{
public:
    typedef std::pair<std::set<unsigned>,std::set<unsigned>> PhiPair;
    typedef std::vector<PhiPair> PhiPerVarT;

    VerifyPhiNodes(){}
    VerifyPhiNodes(Function &F, std::vector<std::pair<std::set<unsigned>,llvm::BitVector>> &PhiInfo);
    bool compare_results();
private:
    unsigned isACorrectPhiNode(unsigned n, llvm::BitVector DefVect);
    unsigned getBlockID(llvm::BasicBlock* B){ return BB2ID[B];}

    std::map<llvm::BasicBlock*,unsigned> BB2ID;
    std::map<unsigned,llvm::BasicBlock *> ID2BB;
    llvm::Function *F;
    unsigned size;
    
};



#endif /* VerifySSA_h */

