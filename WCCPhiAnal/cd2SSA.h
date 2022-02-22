//
//  cd2SSA.h
//  LLVM
//
//  Created by Abu Naser Masud on 2020-09-23.
//

#ifndef cd2SSA_h
#define cd2SSA_h

#include <algorithm>
#include <ctime>
#include <chrono>
#include "ssaTime.h"
#include <iostream>
#include <fstream>
#include "clang/Analysis/CFG.h"

using namespace std::chrono;
using namespace llvm;
using namespace clang;
class CDSSA
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
    std::map<unsigned,std::set<unsigned>> varToPhiNodes;
    unsigned nextRank;
    std::set<unsigned> domTreeLeaves;
    typedef void* range;
    llvm::iterator_range<BumpVector<clang::CFGBlock::AdjacentBlock>::iterator> (CFGBlock::*adj)();
    llvm::iterator_range<BumpVector<clang::CFGBlock::AdjacentBlock>::iterator> (CFGBlock::*adjBar)();
    bool computeSSA;
    // used for debugging
    unsigned curr_var;
public:
    CDSSA(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C);
    CDSSA(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C,bool computeSSA);
    void printGraph();
    std::set<unsigned> checkReachability(llvm::BitVector wccPre,std::set<unsigned> Defs);
    std::set<unsigned> overapproxWX(std::set<unsigned> Defs);
    std::set<unsigned> overapproxWXFP(std::set<unsigned> Defs);
    std::set<unsigned> overapproxWXFPOptimized(std::set<unsigned> Defs);
    std::set<unsigned> computeIncrementalWD(std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre,std::set<unsigned> AllX);
    std::map<unsigned,std::set<unsigned> >  getPhiNodes(std::map<unsigned,std::set<unsigned> > varDefs);
    std::set<unsigned> getWCC(std::set<unsigned> vDefs);
    std::set<std::pair<unsigned,unsigned> > findAllReachableEdges(std::set<unsigned> Vp);
    std::set<unsigned> filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma);
    void printGraph(std::map<unsigned, edgeType> graph);
    bool existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    bool existInitWitnessEff(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res);
    std::set<unsigned> excludeImpreciseResults(std::set<unsigned> wcc,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre);
};


CDSSA::CDSSA(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C):context(C)
{
    cfg=std::move(CFG);
    for (auto *Block : *cfg){
        CFGIdMap[Block->getBlockID()]=Block;
    }
    DefVect=llvm::BitVector(cfg->size());
    adjBar=&CFGBlock::preds;
    adj=&CFGBlock::succs;
}

CDSSA::CDSSA(std::unique_ptr<clang::CFG> &&CFG, ASTContext &C,bool compSSA):context(C)
{
    cfg=std::move(CFG);
    for (auto *Block : *cfg){
        CFGIdMap[Block->getBlockID()]=Block;
    }
    DefVect=llvm::BitVector(cfg->size());
    computeSSA=compSSA;
    if(computeSSA){
        adjBar=&CFGBlock::preds;
        adj=&CFGBlock::succs;
    }
    else{
        adj=&CFGBlock::preds;
        adjBar=&CFGBlock::succs;
    }
}


std::map<unsigned,std::set<unsigned> >  CDSSA::getPhiNodes(std::map<unsigned,std::set<unsigned> > varDefs)
{
    std::map<unsigned,std::set<unsigned> > Res;
    if(!computeSSA) return Res;

    for (auto it=varDefs.begin(); it!=varDefs.end();++it) {
        for(auto n:it->second)
           DefVect[n]=true;
        curr_var=it->first;
        std::set<unsigned> ssa=overapproxWXFPOptimized(it->second);
        Res[it->first]=ssa;
        for(auto n:it->second)
            DefVect[n]=false;
    }
    return Res;
}


std::set<unsigned>  CDSSA::getWCC(std::set<unsigned>  vDefs)
{
    std::set<unsigned> wcc;
    if(computeSSA) return wcc;
    llvm::BitVector WgNodes(cfg->size());
    for(auto v:vDefs)
        DefVect[v]=true;
    std::set<unsigned> Wg=overapproxWXFPOptimized(vDefs);
    for(auto n:Wg)
        WgNodes[n]=1;
    wcc=checkReachability(WgNodes,vDefs);
    return wcc;
}

std::set<unsigned> CDSSA::checkReachability(llvm::BitVector wccPre, std::set<unsigned> Defs)
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


std::set<unsigned>  CDSSA::overapproxWXFP(std::set<unsigned> Defs)
{
    std::queue<unsigned> Q;
    std::map<unsigned,std::set<std::pair<unsigned,unsigned> >> Pre;
    unsigned UNINITVAL=cfg->size()+10;
    unsigned Infty=5*cfg->size();
    std::set<unsigned> WX;
    llvm::BitVector V(cfg->size());
   
    for(auto *B:*cfg)
    {
        if(!B) continue;
        Pre[B->getBlockID()]=std::set<std::pair<unsigned,unsigned>>();
    }
    
    for(auto n:Defs)
    {
        Pre[n].insert({n,0});
        Q.push(n);
        V[n]=true;
    }
    
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        CFGBlock *B=CFGIdMap[currNode];
       
        for(auto Badjfront:(B->*adj)())
        {
            if(!Badjfront) continue;
            unsigned n=Badjfront->getBlockID();
            std::set<std::pair<unsigned,unsigned>> S;
            std::set<unsigned> Sx;
           
            for(auto Badjback:(Badjfront->*adjBar)()){
                if(!Badjback) continue;
                unsigned m=Badjback->getBlockID();
                if(!Pre[m].empty()){
                   S.insert(*Pre[m].begin());
                   Sx.insert((*Pre[m].begin()).first);
                }
            }
            std::set<std::pair<unsigned,unsigned>> Y;
            if(Defs.find(n)!=Defs.end())
               Y.insert({n,0});
            else if(Sx.size()>1)
               Y.insert({n,0});
            else if(Sx.size()==1){
                unsigned min=UNINITVAL;
                for(auto x:S)
                {
                    if(x.first==*Sx.begin() && min>x.second)
                        min=x.second;
                }
                Y.insert({*Sx.begin(),min+1});
            }
            else Y=std::set<std::pair<unsigned,unsigned>>();
            if(Sx.size()>1)
            {
                WX.insert(n);
            }
            if(Y.empty()) continue;
            else if(Pre[n].empty() && !Y.empty()) {
                Pre[n]=Y;
                if(!V[n]){
                   Q.push(n);
                   V[n]=true;
                }
            }
            else{
                   std::pair<unsigned,unsigned> x=*Y.begin(), y=*Pre[n].begin();
                   bool F1=(y.first==n);
                   bool F2=(y.first!=n), F3=(x.first!=n), F4=(y.second <= x.second);
                   if(!(F1 || ( F2 && F3 && F4))){
                      Pre[n]=Y;
                      if(!V[n]){
                       Q.push(n);
                       V[n]=true;
                      }
                    }
              }
       }
    }
    std::map<unsigned,unsigned > Prem;
    for(auto *B:*cfg)
    {
        if(!B) continue;
        unsigned n=B->getBlockID();
        if((Pre[n]).size()==0) Prem[n]=UNINITVAL;
        else Prem[n]=(*Pre[n].begin()).first;
       
    }
    std::set<unsigned> Wg=excludeImpreciseResults(WX,Defs,Prem);
    return Wg;
}

std::set<unsigned>  CDSSA::overapproxWXFPOptimized(std::set<unsigned> Defs)
{
    std::queue<unsigned> Q;
    std::map<unsigned,unsigned> Pre;
    
    unsigned UNINITVAL=cfg->size()+10;
    std::set<unsigned> WX;
    llvm::BitVector V(cfg->size());
    llvm::BitVector DEFN(cfg->size());
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
        DEFN[n]=n;
    }
    
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        CFGBlock *B=CFGIdMap[currNode];
       
        for(auto Badjfront:(B->*adj)())
        {
            if(!Badjfront) continue;
            unsigned n=Badjfront->getBlockID();
            std::set<unsigned> Sx;
            for(auto Badjback:(Badjfront->*adjBar)()){
                if(!Badjback) continue;
                unsigned m=Badjback->getBlockID();
                if(Pre[m]!=UNINITVAL){
                   Sx.insert(Pre[m]);
                }
            }
            
            if(Sx.size()>1)
            {
                WX.insert(n);
                if(!V[n] && Pre[n]!=n) //&& elemValue[n]!=n
                {
                    Q.push(n);
                    V[n]=true;
                }
                Pre[n]=n;
            }
            else{
                unsigned oldVal=Pre[n];
                if(DEFN[n] || Pre[n]==n) continue;
                else Pre[n]=*Sx.begin();
                if(!V[n] && oldVal!=Pre[n]){
                    Q.push(n);
                    V[n]=true;
                }
            }
       }
    }
    std::map<unsigned,unsigned > Prem;
    for(auto *B:*cfg)
    {
        if(!B) continue;
        unsigned n=B->getBlockID();
        if(Pre[n]==UNINITVAL) Prem[n]=UNINITVAL;
        else Prem[n]=Pre[n];
       
    }
    std::set<unsigned> Wg=excludeImpreciseResults(WX,Defs,Pre);
    return Wg;
}
std::set<unsigned>  CDSSA::overapproxWX(std::set<unsigned> Defs)
{
    std::queue<unsigned> Q;
    std::map<unsigned,unsigned > Pre;
    unsigned UNINITVAL=cfg->size()+10;
    std::set<unsigned> WX;
    llvm::BitVector V(cfg->size());
   
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
    
        for(auto Badjfront:(B->*adj)())
        {
            if(!Badjfront) continue;
            unsigned n=Badjfront->getBlockID();
            
            std::set<unsigned> S;
            for(auto Badjback:(Badjfront->*adjBar)()){
                if(!Badjback) continue;
                unsigned m=Badjback->getBlockID();
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
                WX.insert(n);
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
    std::set<unsigned> Wg=excludeImpreciseResults(WX,Defs,Pre);
    return Wg;
}
 
std::set<unsigned> CDSSA::excludeImpreciseResults(std::set<unsigned> WX,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre)
{
    std::map<unsigned, unsigned> Acc;
    std::map<unsigned,bool> isFinal;
    std::map<unsigned, edgeType> solnGraph;
    unsigned UNINITVAL=cfg->size()+10;
    llvm::BitVector V1(cfg->size());
    llvm::BitVector wccNode(cfg->size());
    std::queue<unsigned> Q;
    std::set<unsigned> Wg;
    
    for(auto n:WX)
    {
        isFinal[n]=false;
        wccNode[n]=false;
        
        CFGBlock *B=CFGIdMap[n];
        for(auto Badjback:(B->*adjBar)()){
            if(!Badjback) continue;
            unsigned m=Badjback->getBlockID();
            if(Pre[m]!=UNINITVAL)
            {
                solnGraph[n].second.insert(Pre[m]);
                solnGraph[Pre[m]].first.insert(n);
            }
        }
        Acc[n]=UNINITVAL;
    }
    
    for(auto n:Defs){
        Acc[n]=n;
       // isFinal[n]=true;
        wccNode[n]=false;
        for(auto m:solnGraph[n].first){
            if(!V1[m]) {
                Q.push(m);
                V1[m]=true;
            }
        }
    }
    
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V1[n]=false;
        std::set<unsigned> S;
        unsigned solvedNode;
        std::set<unsigned> unsolvedNode;
        for(auto m:solnGraph[n].second){
            if(Acc[m]!=UNINITVAL) {S.insert(Acc[m]);solvedNode=Acc[m];}
            else unsolvedNode.insert(m);
        }
        
        if(S.size()>1)
        {
            Acc[n]=n;
            isFinal[n]=true;
            if(!DefVect[n])
                wccNode[n]=true;
            Wg.insert(n);
        }
        else
        {
            if(existInitWitness(solnGraph,unsolvedNode,solvedNode,wccNode))
            {
                Acc[n]=n;
                isFinal[n]=true;
                Wg.insert(n);
                if(!DefVect[n])
                    wccNode[n]=true;
            }
            else
            {
                isFinal[n]=true;
                Acc[n]=Acc[solvedNode];
                Pre[n]=Acc[solvedNode];
                
            }
        }
        for(auto m:solnGraph[n].first){
            if(!V1[m] && !isFinal[m]) {
                Q.push(m);
                V1[m]=true;
            }
        }
    }

    return Wg;
}
    

bool CDSSA::existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res){
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
         for(auto m:solnGraph[n].second){
             if(!V[m]) W.push(m);
         }
     }
    return false;
}




std::set<std::pair<unsigned,unsigned> > CDSSA::findAllReachableEdges(std::set<unsigned> Vp)
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

std::set<unsigned> CDSSA::filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma)
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





void CDSSA::printGraph()
{
    CFGBlock* src=&(cfg->getEntry());
    std::queue<CFGBlock*> W;
    W.push(src);
    llvm::BitVector V(cfg->size());
    V[src->getBlockID()]=true;
    std::ofstream fout ("/tmp/temp.dot");
    fout<< "digraph G {" << endl;
    map<unsigned,string> nodes;
    for(auto *B:*cfg)
    {
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
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
                fout << x->getBlockID() <<" -> " << (*ib)->getBlockID() <<endl;
            if(!V[(*ib)->getBlockID() ]) {V[(*ib)->getBlockID() ]=true; W.push(*ib);}
        }
    }
    fout << "}" << endl;
   system("open /tmp/temp.dot");
}

void CDSSA::printGraph(std::map<unsigned, edgeType> graph)
{
    unsigned src=(cfg->getEntry()).getBlockID();
    std::queue<unsigned> W;
    W.push(src);
    llvm::BitVector V(cfg->size());
    V[src]=true;
    std::ofstream fout ("/tmp/soln.dot");
    fout<< "digraph G {" << endl;
    map<unsigned,string> nodes;
    std::map<unsigned, edgeType>::iterator itb=graph.begin();
    while(itb!=graph.end())
    {
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
#endif 
