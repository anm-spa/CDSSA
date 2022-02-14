//
//  PromoteSmartMem2Reg.hpp
//  SmartSSA
//
//  Created by Abu Naser Masud on 2021-09-29.
//

#ifndef PromoteSmartMem2Reg_h
#define PromoteSmartMem2Reg_h

namespace llvm {

template <typename T> class ArrayRef;
class AllocaInst;
class DominatorTree;
class AssumptionCache;

/// Return true if this alloca is legal for promotion.
///
/// This is true if there are only loads, stores, and lifetime markers
/// (transitively) using this alloca. This also enforces that there is only
/// ever one layer of bitcasts or GEPs between the alloca and the lifetime
/// markers.
bool isAllocaPromotable(const AllocaInst *AI);

/// Promote the specified list of alloca instructions into scalar
/// registers, inserting PHI nodes as appropriate.
///
/// This function makes use of DominanceFrontier information.  This function
/// does not modify the CFG of the function at all.  All allocas must be from
/// the same function.
///
void PromoteSmartMemToReg(ArrayRef<AllocaInst *> Allocas, Function &F, DominatorTree &DT, AssumptionCache *AC=nullptr, bool liveness=false,std::vector<std::pair<std::set<unsigned>,llvm::BitVector>> *ssadual=nullptr, bool dumpCFG=false, bool startDef=false);
//void CompareSmartMemToReg(ArrayRef<AllocaInst *> Allocas, Function &F, DominatorTree &DT, AssumptionCache *AC, bool liveness,  std::vector<std::set<unsigned>> ssadual,std::vector<std::set<unsigned>> ssam2r);

void PromoteOriginalMemToReg(ArrayRef<AllocaInst *> Allocas, DominatorTree &DT,
                     AssumptionCache *AC = nullptr,bool liveness=false,std::vector<std::pair<std::set<unsigned>,llvm::BitVector>> *ssam2r=nullptr, bool dumpCFG=false, bool startDef=false);

} // End llvm namespace

#endif /* PromoteSmartMem2Reg_hpp */
