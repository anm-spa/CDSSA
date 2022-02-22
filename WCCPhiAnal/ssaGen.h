//
//  ssaGen.h
//  LLVM
//
//  Created by Abu Naser Masud on 2020-04-26.
//

#ifndef ssaGen_h
#define ssaGen_h

#include "cdAnalysis.h"

// TODO: Code to be removed. Used in previous analysis
class SSAGenEff
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
    llvm::BitVector DefVect, DefVectExt;
    
public:
    SSAGenEff(ASTContext &C):context(C){}
    SSAGenEff(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, unsigned dl, std::set<unsigned> vDefs);
    std::set<unsigned> getAllBlocksDefiningVar(unsigned v, CFGInfo *cfgInfo);
    void printGraph();
    void buildCDgraphNet(std::set<unsigned> vDefs);
    void buildextendedCFG();
    std::set<unsigned> checkReachability(llvm::BitVector wccPre);
    void rankGraph();
    void computeWCC();
    void computeWCCExt();
};

SSAGenEff::SSAGenEff(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C, unsigned dl, std::set<unsigned> vDefs):context(C)
{
    cfg=std::move(CFG);
    for (auto *Block : *cfg){
        CFGIdMap[Block->getBlockID()]=Block;
    }
    
    DefVect=llvm::BitVector(cfg->size());
    for(auto v:vDefs)
        DefVect[v]=true;
    
    buildextendedCFG();
    DefVectExt=llvm::BitVector(cfg->size()+nodeSplitted);
    for(auto v:vDefs)
    {
        if(splitMapRev.find(v)!=splitMapRev.end())
        {
            defs.insert(splitMapRev[v]);
            DefVectExt[splitMapRev[v]]=true;
        }
        else {
            defs.insert(v);
            DefVectExt[v]=true;
        }
    }
    rankGraph();
}

#endif /* ssaGen_h */
