 //===- Mem2Reg.cpp - The -mem2reg pass, a wrapper around the Utils lib ----===//
 //
 // Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 // See https://llvm.org/LICENSE.txt for license information.
 // SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 //
 //===----------------------------------------------------------------------===//
 //
 // This pass is a simple pass wrapper around the PromoteMemToReg function call
 // exposed by the Utils library.
 //
 //===----------------------------------------------------------------------===//
  
 #include "llvm/Transforms/Utils/Mem2Reg.h"
 #include "llvm/ADT/Statistic.h"
 #include "llvm/Analysis/AssumptionCache.h"
 #include "llvm/IR/BasicBlock.h"
 #include "llvm/IR/Dominators.h"
 #include "llvm/IR/Function.h"
 #include "llvm/IR/Instructions.h"
 #include "llvm/IR/PassManager.h"
 #include "llvm/InitializePasses.h"
 #include "llvm/Pass.h"
 #include "llvm/Support/Casting.h"
 #include "llvm/Transforms/Utils.h"
 #include "SmartMem2Reg.h"
#include "VerifySSA.h"

 #include <vector>
#include <fstream>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

 using namespace llvm;
  
 #define DEBUG_TYPE "smartssa"
  
 STATISTIC(NumPromoted, "Number of alloca's promoted");

static cl::opt<bool> Mem2RegDefault("m2rlive", cl::desc("Perform original mem2reg operation"));
static cl::opt<bool> Mem2RegUnOp("m2r", cl::desc("Perform mem2reg operation without liveness analysis"));
static cl::opt<bool> Dual("dual", cl::desc("Perform SSA construction based on duality theorem without liveness analysis"));
static cl::opt<bool> Duallive("duallive", cl::desc("Perform SSA construction based on duality theorem with liveness analysis"));
static cl::opt<bool> PHIInfo("print-phi-info", cl::desc("Compare SSA construction based on duality theorem and mem2reg  without liveness analysis"));
static cl::opt<bool> ViewCFG("g", cl::desc("View the CFG. It should be used when func option is enabled."));
static cl::opt<bool> StartAtDef("defstart", cl::desc("Start node contains definitions of all variables"));
  
static cl::opt<std::string> FuncIn("func", cl::desc("Specify function name"), cl::value_desc("analyze the given function"));

 static bool promoteMemoryToRegister(Function &F, DominatorTree &DT,
                                     AssumptionCache &AC) {
   std::vector<AllocaInst *> Allocas;
   BasicBlock &BB = F.getEntryBlock(); // Get the entry node for the function
   bool Changed = false;
   std::vector<std::pair<std::set<unsigned>,llvm::BitVector>> ssadual, ssam2r;
   while (true) {
       //std::vector<std::set<unsigned>> dualssa, m2rssa;
     Allocas.clear();
  
     // Find allocas that are safe to promote, by looking at all instructions in
     // the entry node
     for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
       if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) // Is it an alloca?
         if (isAllocaPromotable(AI))
           Allocas.push_back(AI);

     if (Allocas.empty())
       break;
     bool dumpCFG=false;
     std::ofstream FuncInput(FuncIn.c_str());
     if(FuncInput.ios_base::good() && ViewCFG)
         dumpCFG=true;
     if(Mem2RegDefault) PromoteOriginalMemToReg(Allocas, DT, &AC,true,&ssam2r,dumpCFG,StartAtDef);
     else if(Mem2RegUnOp) PromoteOriginalMemToReg(Allocas, DT, &AC,false,&ssam2r,dumpCFG,StartAtDef);
     else if(Dual) PromoteSmartMemToReg(Allocas, F,DT, &AC,false,&ssadual,dumpCFG,StartAtDef);
     else if(Duallive) PromoteSmartMemToReg(Allocas, F,DT, &AC,true,&ssadual,dumpCFG,StartAtDef);
     else PromoteSmartMemToReg(Allocas, F,DT, &AC,true,nullptr,dumpCFG,StartAtDef);
     NumPromoted += Allocas.size();
     Changed = true;
   }
     unsigned id = 0;
     for (auto &BB : F)
         BB.setName(Twine(std::to_string(id++)));
     
   if(PHIInfo && (Dual || Duallive))
         VerifyPhiNodes verifyP(F,ssadual);
   else if (PHIInfo && (Mem2RegDefault || Mem2RegUnOp))
         VerifyPhiNodes verifyP(F,ssam2r);
     
  //   F.viewCFG();
   return Changed;
 }
  
 PreservedAnalyses PromotePass::run(Function &F, FunctionAnalysisManager &AM) {
   auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
   auto &AC = AM.getResult<AssumptionAnalysis>(F);

   if (!promoteMemoryToRegister(F, DT, AC))
     return PreservedAnalyses::all();
     
   PreservedAnalyses PA;
   PA.preserveSet<CFGAnalyses>();

   return PA;
 }
  
 namespace {
  
 struct PromoteLegacyPass : public FunctionPass {
   // Pass identification, replacement for typeid
   static char ID;
  
   PromoteLegacyPass() : FunctionPass(ID) {
     initializePromoteLegacyPassPass(*PassRegistry::getPassRegistry());
   }
  
   // runOnFunction - To run this pass, first we calculate the alloca
   // instructions that are safe for promotion, then we promote each one.
   bool runOnFunction(Function &F) override {
       
    //outs()<<"analyzing "<<F.getName().str()<<"\n";
    std::ofstream FuncInput(FuncIn.c_str());
    if(FuncInput.ios_base::good())
        if(F.getName().str()!=FuncIn.c_str())
            return false;
     if (skipFunction(F))
       return false;
     DominatorTree &DT=getAnalysis<DominatorTreeWrapperPass>().getDomTree();
     //DominatorTree DT=DominatorTree();
     //if(Mem2RegDefault || Mem2RegUnOp)
     //    DT=std::move(getAnalysis<DominatorTreeWrapperPass>().getDomTree());
     
     AssumptionCache &AC =
         getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
     return promoteMemoryToRegister(F, DT, AC);
   }
  
   void getAnalysisUsage(AnalysisUsage &AU) const override {
     AU.addRequired<AssumptionCacheTracker>();
     AU.addRequired<DominatorTreeWrapperPass>();

     AU.setPreservesCFG();
   }
 };
  
 } // end anonymous namespace
  
 char PromoteLegacyPass::ID = 0;


static RegisterPass<PromoteLegacyPass> X("smartssa", "Smart SSA Pass",
                             true,
                             false);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new PromoteLegacyPass()); });


/*
 INITIALIZE_PASS_BEGIN(PromoteLegacyPass, "smartssa", "Promote Memory to "
                                                     "Register in Smart way",
                       false, false)
 INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
 INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
 INITIALIZE_PASS_END(PromoteLegacyPass, "smartssa", "Promote Memory to Register in Smart way",false, false)
  
 // createPromoteMemoryToRegister - Provide an entry point to create this pass.
 FunctionPass *llvm::createPromoteMemoryToRegisterPass() {
   return new PromoteLegacyPass();
 }
 */
