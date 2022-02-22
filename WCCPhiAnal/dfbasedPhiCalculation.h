//
//  dfbasedPhiCalculation.h
//  LLVM
//
//  Created by Abu Naser Masud on 2022-02-18.
//

#ifndef dfBasedPhiCalculation_h
#define dfBasedPhiCalculation_h

#include "clang/Analysis/AnalysisDeclContext.h"
#include "config.h"

class dfBasedPhiImpl
{
private:
    ASTContext &context;
    std::map<unsigned, DomTreeNodeBase< CFGBlock>*> idToBlock;
    void calculate_DF(std::map<unsigned, std::set<unsigned>> &DF,CFGDomTree &domTree,DomTreeNode *node);

public:
    dfBasedPhiImpl(ASTContext &C,std::map<unsigned, DomTreeNodeBase< CFGBlock>*> *i2b): context(C), idToBlock (*i2b){}
    void computePhifunctions(FunctionDecl *, CFGAnalysis *,std::map<unsigned,std::set<unsigned> > &);
    void computePhifunctions2(FunctionDecl *, CFGAnalysis *);
    
};


void dfBasedPhiImpl::computePhifunctions(FunctionDecl *f, CFGAnalysis *cfgAnal,std::map<unsigned,std::set<unsigned> > &phiNodes)
{
    // Alternative method based on dominator tree
    const Decl* D=static_cast<Decl *>(f);
    AnalysisDeclContextManager  *analDeclCtxMgr=new AnalysisDeclContextManager(context);
    if(AnalysisDeclContext  *analDeclCtx=analDeclCtxMgr->getContext(D)){
    CFGDomTree domTree;
       
    Stmt *funcBody = f->getBody();
    std::unique_ptr<CFG> cfg = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
    domTree.buildDominatorTree(cfg.get());
    for (CFG::const_iterator I = domTree.getCFG()->begin(),E = domTree.getCFG()->end(); I != E; ++I)
    {
        if(!*I) continue;
        if(!domTree.getBase().getNode(*I)) continue;
        idToBlock[domTree.getBase().getNode(*I)->getBlock()->getBlockID()]=domTree.getBase().getNode(*I);
    }
    std::map<unsigned, std::set<unsigned>> DF;
    
    for (auto *block : *(analDeclCtx->getCFG())){
        DF[block->getBlockID()]=std::set<unsigned>();
    }
    calculate_DF(DF,domTree,domTree.getRootNode());
    
    if(debugLevel==DEBUGMAX)
      for (auto *block : *(analDeclCtx->getCFG()))
      {
          llvm::errs()<<"DF of "<<block->getBlockID()<<": ";
         for(auto i=DF[block->getBlockID()].begin(), e=DF[block->getBlockID()].end();
            i!=e;i++)
             llvm::errs()<<*i<<" ";
        llvm::errs()<<"\n";
      }
    
    std::map<unsigned, std::map< unsigned,unsigned>> DF_number;
    
    unsigned iterCount=0;
    std::map<unsigned,unsigned> hasAlready, work;
    for (auto *block : *(analDeclCtx->getCFG())){
        hasAlready[block->getBlockID()]=0;
        work[block->getBlockID()]=0;
    }
    for (auto *block : *(analDeclCtx->getCFG())){
        if(block->pred_size()>1)
        {
            for(auto v:cfgAnal->getVars())
                DF_number[block->getBlockID()][v]=0;
        }
    }
    
    std::queue<unsigned> W;
    for(auto v:cfgAnal->getVars())
    {
        iterCount = iterCount+1;
        std::set<CFGBlock *> bList=cfgAnal->getDefsOfVar(v);
        for(auto *block: bList)
        {
            work[block->getBlockID()]=iterCount;
            W.push(block->getBlockID());
        }
        W.push(analDeclCtx->getCFG()->getEntry().getBlockID());
        while(!W.empty())
        {
            unsigned x=W.front();
            W.pop();
            for(auto y:DF[x])
            {
                DF_number[y][v]++;
                if(hasAlready[y]<iterCount)
                {
                    if(debugLevel>1)
                       llvm::errs()<<y << " needs phi for "<<cfgAnal->getVarName(v)<<"\n";
                    phiNodes[v].insert(y);    // vars to phi nodes
                    hasAlready[y]=iterCount;
                    if(work[y]<iterCount)
                    {
                        work[y]=iterCount;
                        W.push(y);
                    }
                }
            }
        }
    }
    }
}



void dfBasedPhiImpl::calculate_DF(std::map<unsigned, std::set<unsigned>> &DF,CFGDomTree &domTree,DomTreeNode *node)
{
    for(auto z:node->getChildren())
        calculate_DF(DF,domTree,z);
    DF[node->getBlock()->getBlockID()]=std::set<unsigned>();
    CFGBlock *B =node->getBlock();
    for(auto i=B->succ_begin(), e=B->succ_end();i!=e;i++)
    {
        if(!domTree.getBase().getNode(*i)) continue;
        CFGBlock *b=domTree.getBase().getNode(*i)->getIDom()->getBlock();
        if(b->getBlockID()!=node->getBlock()->getBlockID())
            DF[node->getBlock()->getBlockID()].insert((*i)->getBlockID());
    }
    
    for(auto z:node->getChildren())
    {
        for(auto y:DF[z->getBlock()->getBlockID()])
        {
            DomTreeNodeBase< CFGBlock>* Y=idToBlock[y];
            if(!Y) continue;
            CFGBlock *idom=Y->getIDom()->getBlock();
            if(idom->getBlockID()!=node->getBlock()->getBlockID()){
                DF[node->getBlock()->getBlockID()].insert(Y->getBlock()->getBlockID());
            }
            
        }
    }
}

void dfBasedPhiImpl::computePhifunctions2(FunctionDecl *f, CFGAnalysis *cfgAnal)
{
    // Alternative method based on dominator tree
    const Decl* D=static_cast<Decl *>(f);
    AnalysisDeclContextManager  *analDeclCtxMgr=new AnalysisDeclContextManager(context);
    AnalysisDeclContext  analDeclCtx(analDeclCtxMgr,D);
    CFGDomTree domTree;
    Stmt *funcBody = f->getBody();
    std::unique_ptr<CFG> cfg = CFG::buildCFG(f, funcBody, &context, CFG::BuildOptions());
    domTree.buildDominatorTree(cfg.get());
   
    std::map<unsigned, std::set<unsigned>> DF,DF2;
    std::map<CFGBlock *, std::set<CFGBlock *>> DF1;
    
    std::map<unsigned, std::map< unsigned,unsigned>> DF_number;
    for (auto *block: *(analDeclCtx.getCFG())){
        DF[block->getBlockID()]=std::set<unsigned>();
        DF2[block->getBlockID()]=std::set<unsigned>();
        DF1[block]=std::set<CFGBlock *>();
    }
    for (auto *block : *(analDeclCtx.getCFG())){
        if(block->pred_size()>=2)
        {
            for (CFGBlock::const_pred_iterator I = block->pred_begin(),
                 F = block->pred_end(); I != F; ++I)
            {
                CFGBlock *runner=(*I);
                CFGBlock *b;
                if(!domTree.getBase().getNode(block)) continue;
                while((b=domTree.getBase().getNode(block)->getIDom()->getBlock()) && b->getBlockID()!=runner->getBlockID())
                {
                    DF[runner->getBlockID()].insert(block->getBlockID());
                    runner=domTree.getBase().getNode(runner)->getIDom()->getBlock();
                }
            }
        }
    }
    
    /*   Cytron et al.'s Algorithm
     for each X in a bottom-up traversal of the dominator tree do
       DF(X) = empty
       for each Y in Successors(X)
         if (idom(Y) != X) then DF(X) += Y
       for each Z in DomChildren(X)
        for each Y in DF(Z)
          if (idom(Y) != X then DF(X) += Y  */
    
    DomTreeNode *root=domTree.getRootNode();
    std::queue<DomTreeNode *> dq;
    dq.push(root);
    while(!dq.empty())
    {
        DomTreeNode *current=dq.front();
        dq.pop();
        CFGBlock *curr=current->getBlock();
        for (CFGBlock::const_succ_iterator I = curr->succ_begin(),
             F = curr->succ_end(); I != F; ++I)
        {
            if(!domTree.getBase().getNode(*I)) continue;
            CFGBlock *b=domTree.getBase().getNode(*I)->getIDom()->getBlock();
            if(b->getBlockID()!=curr->getBlockID()){
                DF1[curr].insert((*I));
                DF2[curr->getBlockID()].insert((*I)->getBlockID());
            }
        }
        for(auto z:current->getChildren())
        {
            for(auto y:DF1[z->getBlock()])
            {
                if(!domTree.getBase().getNode(y)) continue;
                CFGBlock *idom=domTree.getBase().getNode(y)->getIDom()->getBlock();
                if(idom->getBlockID()!=curr->getBlockID()){
                    DF1[curr].insert(y);
                    DF2[curr->getBlockID()].insert(y->getBlockID());
                }
                
            }
            dq.push(z);
        }
        
    }
    
    unsigned iterCount=0;
    std::map<unsigned,unsigned> hasAlready, work;
    for (auto *block : *(analDeclCtx.getCFG())){
        hasAlready[block->getBlockID()]=0;
        work[block->getBlockID()]=0;
        
    }
    
    for (auto *block : *(analDeclCtx.getCFG())){
        if(block->pred_size()>1)
        {
            for(auto v:cfgAnal->getVars())
                DF_number[block->getBlockID()][v]=0;
        }
    }
    
    std::queue<unsigned> W;
    for(auto v:cfgAnal->getVars())
    {
        iterCount = iterCount+1;
        std::set<CFGBlock *> bList=cfgAnal->getDefsOfVar(v);
        for(auto *block: bList)
        {
            work[block->getBlockID()]=iterCount;
            W.push(block->getBlockID());
        }
        while(!W.empty())
        {
            unsigned x=W.front();
            W.pop();
            for(auto y:DF2[x])
            {
                if(hasAlready[y]<iterCount)
                {
                    //llvm::errs()<<"\n"<<y<<" requires phi for "<<cfgAnal->getVarName(v);
                    DF_number[y][v]++;
                    hasAlready[y]=iterCount;
                    if(work[y]<iterCount)
                    {
                        work[y]=iterCount;
                        W.push(y);
                    }
                }
            }
        }
    }
    for (auto *block : *(analDeclCtx.getCFG())){
        if(block->pred_size()>1)
        {
            for(auto v:cfgAnal->getVars())
                if(DF_number[block->getBlockID()][v]>1)
                     llvm::errs()<<"\n"<<block->getBlockID()<<" requires phi for "<<cfgAnal->getVarName(v);
        }
    }
    
}


#endif /* dfBasedPhiCalculation_h */
