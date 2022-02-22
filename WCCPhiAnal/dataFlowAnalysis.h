//
//  dataFlowAnalysisg.h
//  LLVM
//
//  Created by Abu Naser Masud on 2019-02-22.
//

#ifndef dataFlowAnalysis_h
#define dataFlowAnalysis_h
//
//  dataflow_analysis.h
//  LLVM
//
//  Created by Abu Naser Masud on 2019-02-19.
//
#include <algorithm>
#include <vector>
#include <iterator>
#include <queue>
#include "ssaTime.h"

using namespace llvm;
using namespace clang;

typedef std::pair< const CFGBlock *,const CFGBlock *> WElemT;

class VarInfoT{
    std::map<unsigned, std::string> VarMap;
    std::map<std::string,unsigned>  MapVar;
    unsigned id;
public:
    VarInfoT(){id=0;VarMap.clear(); MapVar.clear();}
    unsigned mapsto(std::string var)
    {
        auto it=MapVar.find(var);
        if(it!= MapVar.end())
            return it->second;
        else{
            VarMap[id]=var;
            MapVar[var]=id++;
            return id-1;
        }
    }
    std::string mapsfrom(unsigned varNum)
    {
        auto it=VarMap.find(varNum);
        if(it!= VarMap.end())
            return it->second;
        else{
            return "unknown variable";
        }
    }
};

class VarInfoInBlock
{
public:   // privacy should be changed
    std::vector<std::set<unsigned> > Defs;   //defs of each stmt
    std::vector<std::set<unsigned> > Refs;   // refs of each stmt
    std::set<unsigned> BlockDefs, BlockRefs;
    void insertVarInfo(std::set<unsigned> &D, std::set<unsigned> &R)
    {
        Defs.push_back(D);
        Refs.push_back(R);
        BlockDefs.insert(D.begin(),D.end());
        BlockRefs.insert(R.begin(),R.end());
    }
    bool isDef(unsigned v)
    {
        auto it=BlockDefs.find(v);
        if(it==BlockDefs.end()) return false;
        return true;
    }
};
class WorkListT
{
    std::queue<WElemT> WList;
    std::map<unsigned,std::set<unsigned> > hasEnqueued;
    std::map<unsigned,std::set<unsigned> > visited;
    
public:
    WorkListT(){}
    WorkListT(WorkListT &w){
    }
    void EnqueueOP(const CFGBlock *Src, const CFGBlock *Dst) {
        WList.push(std::make_pair(Src,Dst));
    }
    
    WElemT DequeueOP() {
        if (WList.empty()) return WElemT();
        WElemT elem=WList.front();
        WList.pop();
        return elem;
    }
    
    bool notEmpty()
    {
        return !WList.empty();
    }
};

class DFAnalysis
{
    typedef std::set<unsigned> DataT;
    typedef std::map<unsigned,DataT> VarDataT;
    std::map<unsigned, VarDataT> AEntry, AExit;
    WorkListT workList;
    VarInfoT varInfo;
    std::map<unsigned, CFGBlock*> idToBlock;
    unsigned maxNode;
    unsigned EntryID;
    std::set<unsigned, std::greater<unsigned>> joinNodes;
    std::map<unsigned, std::map<unsigned,std::set<unsigned> > > jMap;
    struct CFGBlockInfo
    {
        std::set<unsigned> preds, succs;
    };
    
    std::map<unsigned, struct CFGBlockInfo> blockMap;
    std::map<unsigned,VarInfoInBlock> blockToVars;
    unsigned debug_level;
    
public:
    std::map<unsigned,unsigned> numEdgeVisit;
    
    unsigned muN(unsigned ID)
    {
        if(ID< maxNode) return ID;
        return ID-maxNode;
    }
    unsigned invMuN(unsigned ID)
    {
        return maxNode+ID;
    }
    
    DFAnalysis(){}
    DFAnalysis(std::unique_ptr<CFG> &cfg,std::set<unsigned> vars, VarInfoT &vInfo, std::map<unsigned,VarInfoInBlock> &block2Vars, unsigned dl=0)
    {
        maxNode=cfg->size();
        for (auto *block : *cfg)
            idToBlock[block->getBlockID()]=block;
    
        CFGBlock &entry=cfg->getEntry();
        EntryID=entry.getBlockID();
        visitReachableBlocks(&entry);   // Refine CFG succs and preds
        
        for(CFGBlock::const_succ_iterator I = entry.succ_begin(),
            E = entry.succ_end(); I != E; ++I) {
            workList.EnqueueOP(&entry,*I);
        }
        varInfo=vInfo;
        debug_level=dl;
        for (auto *B : *cfg) {
            unsigned bID=B->getBlockID();
            if(blockMap[bID].preds.size()>1)
                joinNodes.insert(bID);
        }
        blockToVars=block2Vars;
    }
    void visitReachableBlocks(clang::CFGBlock *B)
    {
        std::set<unsigned> wList;
        llvm::BitVector visited(maxNode);
        wList.insert(B->getBlockID());
        while (!wList.empty()) {
            unsigned bl=*wList.begin();
            wList.erase(bl);
            visited[bl]=true;
            for (CFGBlock::const_succ_iterator I = idToBlock[bl]->succ_begin(),
                 F = idToBlock[bl]->succ_end(); I != F; ++I)
            {
                if(!(*I)) continue;
                blockMap[bl].succs.insert((*I)->getBlockID());
                blockMap[(*I)->getBlockID()].preds.insert(bl);
                if(!visited[(*I)->getBlockID()])  wList.insert((*I)->getBlockID());
            }
        }
    }
    
    // Printing internal steps of the analysis
    void print_nodeInfo(unsigned n,unsigned v)
    {
        if(debug_level<2) return;
        llvm::errs()<<"info("<<n<<", "<<*(AExit[n][v].begin())<<") = ";
        for(auto d: AEntry[n][v])
            llvm::errs()<<d<<" ";
        llvm::errs()<<"\n";
    }
    
    void printPhiNodes(std::unique_ptr<CFG> &cfg,std::set<unsigned> vars)
    {
        for (auto *block : *cfg){
            unsigned bID=block->getBlockID();
            if(block->pred_size()>1)
            {
                for(auto v:vars)
                {
                    if(AEntry[bID][v].size()>1)
                        llvm::errs()<<"Block "<<bID<<" requires phi function for variable "<<varInfo.mapsfrom(v)<<"\n";
                }
            }
        }
    }
    
    void debug_print(unsigned id,std::set<unsigned> &vars)
    {
        if(debug_level<2) return;
        llvm::errs()<< "AEntry B"<<id<<"\n";
        for(auto v:vars)
        {
            std::string vn=varInfo.mapsfrom(v);
            if(AEntry[id][v].empty())
                llvm::errs()<< "Var "<<vn<<": [], ";
            else
            {   llvm::errs()<< "Var "<<vn<<": ";
                for(auto I=AEntry[id][v].begin(), F=AEntry[id][v].end();I!=F;I++)
                    llvm::errs()<<*I<<" ";
            }
            llvm::errs()<<" \n";
        }
        llvm::errs()<< "AExit: "<<id<<"\n";
        for(auto v:vars)
        {
            std::string vn=varInfo.mapsfrom(v);
            if(AExit[id][v].empty())
                llvm::errs()<< "Var "<<vn<<": [], ";
            else
            {   llvm::errs()<< "Var "<<vn<<": ";
                for(auto I=AExit[id][v].begin(), F=AExit[id][v].end();I!=F;I++)
                    llvm::errs()<<*I<<" ";
            }
            llvm::errs()<<" \n\n";
        }
    }
    
    void printDFValues_withDebug(std::unique_ptr<CFG> &cfg,std::set<unsigned> vars,std::map<unsigned,VarInfoInBlock> &blockToVars)
    {
        
        std::set<unsigned, std::greater<unsigned>> jNodes,joinNodes;
        for(auto v:vars){
            llvm::errs()<<"\nTable of "<<v<<"\n";
            for (auto *block : *cfg){
                unsigned bID=block->getBlockID();
                jMap[bID][v]=std::set<unsigned>();
                jNodes.insert(bID);
                if(blockMap[bID].preds.size()>1)
                {
                    joinNodes.insert(bID);
                    unsigned k=*(AExit[bID][v].begin());
                    llvm::errs()<<"Value ("<<bID<<", "<<k<<") = ";
                    for(auto d: AEntry[bID][v])
                    {
                        llvm::errs()<<d<<" ";
                        jMap[bID][v].insert(d);
                    }
                    llvm::errs()<<"\n";
                }
                else {
                    unsigned k=*(AExit[bID][v].begin());
                    llvm::errs()<<"Value ("<<bID<<", "<<k<<") = ";
                    for(auto d: AEntry[bID][v])
                    {
                        llvm::errs()<<d<<" ";
                        jMap[bID][v].insert(d);
                    }
                    llvm::errs()<<"\n";
                }
            }
            resolve_equations(joinNodes,v,blockToVars);
        }
    }
    
    void printDFValues(std::unique_ptr<CFG> &cfg,std::set<unsigned> vars,std::map<unsigned,VarInfoInBlock> &blockToVars)
    {
        
        //std::set<unsigned, std::greater<unsigned>> jNodes,joinNodes;
        for(auto v:vars){
           for (auto *block : *cfg)
           {
              unsigned bID=block->getBlockID();
              jMap[bID][v].insert(AEntry[bID][v].begin(),AEntry[bID][v].end());
              
            }
            resolve_equations(joinNodes,v,blockToVars);
      }
   }
    
    
    bool mutual_resolution(std::set<unsigned> R, unsigned D, std::set<unsigned> Nr,unsigned v)   // extend this function
    {
        R.insert(D);
        for(auto n:Nr)
        {
            std::set<unsigned> RR,diff;
            for(auto d:jMap[n][v])
                if(d<maxNode) RR.insert(d);
            //else RR.insert(muN[n]);
          //  if(RR.size()>0) RR.insert(muN[n]);
            if(RR.size()>0) RR.insert(muN(n));
            std::set_intersection (R.begin(), R.end(),RR.begin(), RR.end(),
                                   std::inserter(diff, diff.begin()));
            
            if(diff.empty() && R.size()>1 && RR.size()>1)
                return true;
        }
        return false;
    }
    
    unsigned forward_resolution(std::set<unsigned> R,unsigned d,unsigned v, bool isdef)
    {
        std::set<unsigned> R1,R2,R3,R4;
        R1.insert(R.begin(),R.end());
        R2.insert(R.begin(),R.end());
        for(auto n:jMap[muN(d)][v])
        {
            if(n>=maxNode && jMap[muN(d)][v].size()>1) R4.insert(muN(d));
            else R3.insert(n);
        }
        
        if(R3.size()>=2 || isdef) R2.insert(muN(d));
        else R2.insert(R3.begin(),R3.end());
        R1.insert(R4.begin(),R4.end());
        if(R1.size()>=2 && R2.size()>=2) return 2;
        if(R2.size()>=2 && R4.size()==0) return 0;
        return 0;
    }
    
    std::set<unsigned> singleUnknown_resolution(std::set<unsigned> S, llvm::BitVector inProcessing,unsigned n,unsigned v,std::set<unsigned> Res)
    {
        if(!((S.size()==1) && *S.begin()>=maxNode )) return Res;
        unsigned val=*S.begin();
        std::set<unsigned> Sp=singleUnknown_resolution(jMap[muN(val)][v],inProcessing,muN(val), v, S);
        jMap[muN(n)][v]=Sp;
        return Sp;
    }
    
    unsigned calculateMu(unsigned n,unsigned D, unsigned v, std::set<unsigned> S, std::map<unsigned,VarInfoInBlock> &blockToVars,llvm::BitVector &inProcessing, llvm::BitVector &Visit)
    {
        std::set<unsigned> R={},NotResolved={};
        if(inProcessing[D]) return D;
        S=singleUnknown_resolution(S, inProcessing,n,v,S);
        for(auto d:S)
        {
            if(d<maxNode)
                R.insert(d);
            else if(inProcessing[d])
                NotResolved.insert(d);
            else if(Visit[muN(d)])
                R.insert(*AExit[muN(d)][v].begin());
            else{
                inProcessing[invMuN(n)]=true;
                unsigned k=calculateMu (muN(d),d,v,jMap[muN(d)][v],blockToVars,inProcessing,Visit);
                inProcessing[invMuN(n)]=false;
                if(k<maxNode) R.insert(k);
                else {
                    if(forward_resolution(R,k,v,blockToVars[muN(k)].isDef(v)))
                    {
                        jMap[n][v]={n};
                        AExit[n][v]={n};
                        R.insert(muN(k));
                    }
                    else if(k!=D) NotResolved.insert(k);
                }
            }
        }
        
        if(R.size()>1)
        {
            jMap[n][v]={n};
            AEntry[n][v]=R;
            AExit[n][v]={n};
            Visit[n]=true;
            return n;
        }
        else if(R.size()<=1 && NotResolved.empty()){
            jMap[n][v]={*R.begin()};
            AEntry[n][v]=R;
            AExit[n][v]=R;
            Visit[n]=true;
            return *R.begin();
        }
        else // if(!NotResolved.empty())
        {
            std::set<unsigned> T={};
            T.insert(R.begin(),R.end());
            T.insert(NotResolved.begin(),NotResolved.end());
            jMap[n][v]=T;
            if(T.size()< 2) return *T.begin();
            return D;
        }
    }

    // Create the G_{\mu}^{\alpha} graph
    void cyclic_resolution_aux(unsigned n, unsigned v, std::map<unsigned, std::set<unsigned>> &parentOf,std::set<unsigned> &resolved, llvm::BitVector &V,std::map<unsigned, std::set<unsigned>> &children)
    {
        V[n]=true;
        if(n<maxNode) resolved.insert(n);
        for(auto d:jMap[muN(n)][v])
        {
          if(!V[d]){
              if(d>= maxNode && jMap[muN(d)][v].size()==1 && *jMap[muN(d)][v].begin()<maxNode) d=*jMap[muN(d)][v].begin();  // experimental code
              parentOf[d].insert(n);
              children[n].insert(d);
              if(d<maxNode) resolved.insert(d);
              else {
                     cyclic_resolution_aux(d,v,parentOf,resolved,V,children);
                   }
            }
            else{
                if(d>= maxNode && jMap[muN(d)][v].size()==1 && *jMap[muN(d)][v].begin()<maxNode) d=*jMap[muN(d)][v].begin();  // experimental code
                parentOf[d].insert(n);
                children[n].insert(d);
               }
        }
    }
    
    
    void optimize_Gmu(std::set<unsigned> unresolved,std::map<unsigned, std::set<unsigned>> &parentOf,std::map<unsigned, std::set<unsigned>> &children)
    {
        for(auto m:unresolved)
        {
            if(parentOf[m].size()==1)
            {
                unsigned parent=*parentOf[m].begin();
                for(auto child:children[m])
                {
                    children[parent].insert(child);
                    parentOf[child].insert(parent);
                    parentOf[child].erase(m);
                }
                children[parent].erase(m);
            }
        }
    }
    
    void cyclic_assignment(std::set<unsigned> res,std::map<unsigned, std::set<unsigned> > parentOf, std::map<unsigned, std::set<unsigned> > &resolved, std::set<unsigned> &nonResolved)
    {
        for(auto m:res)
        {   unsigned M=2*maxNode+1;
            std::vector<unsigned> W;
            llvm::BitVector V(2*maxNode+2);
            V[M]=true;
            for(auto d:parentOf[m])
               W.push_back(d);
            while(W.size()>0)
            {
                unsigned q=W.back();
                W.pop_back();
                if(q==M) continue;
                nonResolved.insert(q);
                resolved[q].insert(m);
                V[q]=true;
                for(auto d:parentOf[q])
                    if(!V[d])
                        W.push_back(d);
            }
        }
    }
        
    
    void cyclic_dataflow(std::vector<std::pair<unsigned,unsigned> > W,std::map<unsigned, std::set<unsigned> > parentOf, std::map<unsigned, std::set<unsigned> > &resolved, std::set<unsigned> &nonResolved, std::set<unsigned> res,unsigned v,std::map<unsigned, std::set<unsigned> > children)
    {
         unsigned M=2*maxNode+1;
        std::map<unsigned, llvm::BitVector > Visited;
         for(auto r:res){
              Visited[muN(r)]=llvm::BitVector(2*maxNode+2);
              Visited[muN(r)][M]=true;
         }
         llvm::BitVector V(2*maxNode+2);
         V[M]=true;
         while(W.size()>0)
         {
            std::pair<unsigned,unsigned> q=W.back();
            W.pop_back();
            if(q.first==M) continue;
            nonResolved.insert(q.first);
            if(!V[q.first])
                resolved[q.first].clear();                  // resolved is sigma in the paper
             
            resolved[q.first].insert(q.second);
            V[q.first]=true;
            Visited[muN(q.second)][q.first]=true;
            if(!(jMap[muN(q.first)][v].size()==1 && *jMap[muN(q.first)][v].begin()<maxNode) && q.first >= maxNode)
            for(auto d:parentOf[q.first])
                if(!Visited[muN(q.second)][d])
                    W.push_back(std::pair<unsigned,unsigned>(d,q.second));
        }
    }
    
    
    
        
    unsigned cyclic_resolution(unsigned D, unsigned v, llvm::BitVector &Visit)
    {
        std::set<unsigned> nonResolved={},res={}, defs;
        std::map<unsigned, std::set<unsigned> > parentOf,resolved, children;
        llvm::BitVector V(2*maxNode);
        parentOf[D]={2*maxNode+1};
        cyclic_resolution_aux(D,v, parentOf,res,V,children);
        defs=res;
        for(auto r:res)
                resolved[r].insert(r);
            
        llvm::BitVector VT(2*maxNode+2);
        VT[2*maxNode+1]=true;
        std::vector<std::pair<unsigned,unsigned> > W;
        std::set<unsigned> allRes=res;
        std::queue<unsigned> Queue;
        while(!res.empty()) {
            std::set<unsigned> newRes;
            for(auto r:res)
            {
                for(auto d:parentOf[r]){
                    if(!VT[d]){
                    W.push_back(std::pair<unsigned,unsigned>(d,muN(r)));
                    }
                }
            }
            cyclic_dataflow(W,parentOf, resolved, nonResolved,allRes,v,children);
            for(auto q:res)
                VT[q]=true;
            
            std::set<unsigned> Q,T;
            std::set<std::pair<unsigned,unsigned> > Q1;
            for(auto q:res){
                if(resolved[q].size()<=1)
                    VT[q]=true;
                else T.insert(q);
                for(auto p:parentOf[q])
                    Q1.insert(std::pair<unsigned,unsigned>(p,q));
                    parentOf[q].clear();
            }
            
            while(!Q1.empty())
            {
                std::pair<unsigned,unsigned> Q2=*Q1.begin();
                unsigned m=Q2.first,mp=Q2.second;
                Q1.erase(Q2);
                if((VT[m] || m==2*maxNode+1)) continue;
                if(resolved[m].size()>1)
                {
                    jMap[muN(m)][v]={muN(m)};
                    newRes.insert(m);
                    Queue.push(m);
                    parentOf[mp].insert(m);
                }
                else{
                    VT[m]=true;
                    for(auto dp:parentOf[m])
                        if(!VT[dp])
                            Q1.insert(std::pair<unsigned,unsigned>(dp,mp));
                    }
                    
            }
            W.clear();
            allRes.insert(newRes.begin(),newRes.end());
            res.clear();
            res=newRes;
        }
        for(auto m:nonResolved)
        {
            resolved[m].clear();
            for(auto d:children[m]){
                if(d==m && defs.find(m)!=defs.end())
                    resolved[m].insert(d);
                else if(jMap[muN(d)][v].size()==1 && *jMap[muN(d)][v].begin()==muN(d))
                    resolved[m].insert(muN(d));
                else
                    resolved[m].insert(resolved[d].begin(),resolved[d].end());
            }
                
            if(resolved[m].size()>1)
            {
                std::set<unsigned> S;
                jMap[muN(m)][v]={muN(m)};
                AEntry[muN(m)][v]=resolved[m];
                AExit[muN(m)][v]={muN(m)};
                Visit[muN(m)]=true;
            }
            else if(resolved[m].size()==1)
            {
                unsigned p=muN(m);
                jMap[p][v]=resolved[m];
                AEntry[p][v]=resolved[m];
                AExit[p][v]=resolved[m];
                Visit[p]=true;
            }
        }
        resolved.clear();
        return D;
    }
    
    void exportResults(std::unique_ptr<CFG> &cfg,std::set<unsigned> vars, std::map<unsigned,std::set<unsigned> > &phiNodes)
    {
        for(auto v:vars)
            phiNodes[v]=std::set<unsigned>();
        
        for (auto *block : *cfg){
            unsigned bID=block->getBlockID();
            //phiNodes[bID]=std::set<unsigned>();
            if(block->pred_size()>1)
            {
                for(auto v:vars)
                {
                    std::set<unsigned> rd;
                    for(auto d:AEntry[bID][v])
                        if(d<maxNode) rd.insert(d);
                    
                    if(rd.size()>1){
                        phiNodes[v].insert(bID);
                    }
                }
            }
        }
    }
    
    bool lessThanQual(DataT oldV,DataT newV,int k)
    {
        if(!(oldV==newV)) return true;
        if(oldV.empty() && !k) return true;
        return false;
    }
    
    bool lessThanQualSet(std::set<unsigned> oldV, std::set<unsigned> newV,int k)
    {

        if(!(oldV==newV)) return true;
        if(oldV.empty() && !k) return true;
        return false;
    }
    
    bool lessThanQualVal(DataT oldV,DataT newV)
    {
        if(!(oldV==newV)) return true;
        if(oldV.empty()) return true;
        return false;
    }
    
    
    void resolve_equations(std::set<unsigned, std::greater<unsigned>> joinNodes,unsigned v,std::map<unsigned,VarInfoInBlock> &blockToVars)
    {
        llvm::BitVector V(maxNode);
        for(auto n:joinNodes)
        {
            llvm::BitVector inProcessing(2*maxNode);
            if(V[n]) continue;
            unsigned D=*(AExit[n][v].begin());
            if(D<maxNode) D=n;
            unsigned k= cyclic_resolution(D,v,V);
        }
    }
    
    // Main Fixpoint computation
    void computeFixpoint(std::set<unsigned> &vars, std::map<unsigned,VarInfoInBlock> &blockToVars)
    {
        WorkListT *wList1;
        wList1 =&workList;
        llvm::BitVector visited(maxNode);
        while(wList1->notEmpty()){
            WElemT E=wList1->DequeueOP();
            applyTF(E.first,vars,blockToVars);
            for(auto v:vars)
            {
                std::set<unsigned> defs;
                for(auto pre_B:blockMap[E.second->getBlockID()].preds)
                {
                    if(!AExit[pre_B][v].empty())
                    {
                        unsigned i=*(AExit[pre_B][v].begin());
                        defs.insert(i);
                    }
                    else defs.insert(invMuN(pre_B));
                    
                }
                AEntry[E.second->getBlockID()][v]=defs;
                
            }
            if(!visited[E.second->getBlockID()]){
            for (CFGBlock::const_succ_iterator I = E.second->succ_begin(), F = E.second->succ_end(); I != F; ++I)
            {
                if(!(*I)) continue; // ignore unreachable block
                wList1->EnqueueOP(E.second,*I);
            }
              visited[E.second->getBlockID()]=true;
            }
            if(E.second->succ_size()==0) {
                applyTF(E.second,vars,blockToVars);
            }
            
        }
    }
    
    void applyTF(const CFGBlock *cfgblock, std::set<unsigned> &vars, std::map<unsigned,VarInfoInBlock> &blockToVars)
    {
        unsigned bID=cfgblock->getBlockID();
        if(blockMap[bID].preds.size() <=1)
        {
            for(auto v:vars)
            {
                if(blockToVars[bID].isDef(v))
                   AExit[bID][v]={bID};
                else  AExit[bID][v]=AEntry[bID][v];
            }
        }
        else{     // It is a join node
            for(auto v:vars)
            {
                unsigned size=AEntry[bID][v].size();
                bool status=true;
                for(auto d:AEntry[bID][v])
                {
                    if(d>=maxNode) { size=size-1; status=false;}
                }
                if(blockToVars[bID].isDef(v))
                    AExit[bID][v]={bID};
                else if(size>1){
                    AExit[bID][v]={bID};
                }
                else if(!status)
                {
                    AExit[bID][v]={invMuN(bID)};
                }
                else
                {
                    AExit[bID][v]=AEntry[bID][v];
                }
            }
        }
        return;
    }
};


#endif /* dataFlowAnalysis_h */
