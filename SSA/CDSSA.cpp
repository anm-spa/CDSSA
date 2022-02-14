//
//  CDSSA.cpp
//  SmartSSA
//
//  Created by Abu Naser Masud on 2021-09-30.
//

#include "CDSSA.h"


CDSSA::CDSSA(llvm::Function &Fs,DenseMap<BasicBlock *, unsigned> BBs,SmallVector<BasicBlock *, 32> defBlocks, SmallPtrSet<BasicBlock *, 32> liveBlocks, bool compSSA,bool liveanal)
{
    F=&Fs;
    BBNumbers=BBs;
    ABIter=AdjBlockIter(compSSA);
    size=BBs.size();
    DefVect=llvm::BitVector(size);
    UseVect=llvm::BitVector(size);
    LiveVect=llvm::BitVector(size);
    computeSSA=compSSA;
    for (llvm::BasicBlock &BB : *F){
        unsigned BBid=BBNumbers[&BB];
        BBIds[BBid]=&BB;
    }
  //  outs()<<"\nDef Blocks: ";

    for(auto B:defBlocks){
        defs.insert(getBlockID(B));
        DefVect[getBlockID(B)]=true;
    //    outs()<<getBlockID(B)<<" ";
    }
 //   outs()<<"\nLive Blocks: ";
  
    for(auto B:liveBlocks){
        LiveVect[getBlockID(B)]=true;
 //       outs()<<getBlockID(B)<<" ";
    }
    liveness=liveanal;
}


CDSSA::CDSSA(llvm::Function &Fs,DenseMap<BasicBlock *, unsigned> BBs,SmallVector<BasicBlock *, 32> defBlocks,SmallVector<BasicBlock *, 32> useBlocks, SmallPtrSet<BasicBlock *, 32> liveBlocks, bool compSSA,bool liveanal)
{
    F=&Fs;
    BBNumbers=BBs;
    ABIter=AdjBlockIter(compSSA);
    size=BBs.size();
    DefVect=llvm::BitVector(size+1);
    UseVect=llvm::BitVector(size+1);
    LiveVect=llvm::BitVector(size+1);
    computeSSA=compSSA;
    for (llvm::BasicBlock &BB : *F){
        unsigned BBid=BBNumbers[&BB];
        BBIds[BBid]=&BB;
    }

    for(auto B:defBlocks){
        defs.insert(getBlockID(B));
        DefVect[getBlockID(B)]=true;
    }
  
    for(auto B:useBlocks){
        uses.insert(getBlockID(B));
        UseVect[getBlockID(B)]=true;
    }
    
    for(auto B:liveBlocks){
        LiveVect[getBlockID(B)]=true;
 //       outs()<<getBlockID(B)<<" ";
    }
    liveness=liveanal;
   // backwardReachability(UseVect,uses);
}




void CDSSA::getPhiNodes(SmallVector<BasicBlock *, 32> &PhiBlocks)
{
    std::map<unsigned,std::set<unsigned> > Res;
    if(!computeSSA) return;
    std::set<unsigned> ssa=overapproxWXFPOptimized();
 //   std::set<unsigned> ssa=overapproxWXFPHeu();
    std::set<unsigned> livessa;
    
 /*   outs()<<"\nPhi Blocks: ";
    for(auto n:ssa) outs()<<n<<" ";
    outs()<<"\n";
 */
   // if(liveness) //livessa=getReachableNodes(UseVect, ssa);
  //  outs()<<"\nPhi Blocks (Final): ";
    
    if(liveness){
    for(auto n:ssa)
        if(LiveVect[n])
           {
            PhiBlocks.emplace_back(BBIds[n]);// livessa.insert(n);
            finalssa.insert(n);
      //      outs()<<n<<" ";
        }
    }
    else{
        for(auto n:ssa)
            //if(isACorrectPhiNode(n))
            {
                PhiBlocks.emplace_back(BBIds[n]);
                finalssa.insert(n);
            }
    }
  //      else if(!liveness) PhiBlocks.emplace_back(BBIds[n]);
  //  for(auto n:livessa){
 //       outs()<<n<<" ";
 //       PhiBlocks.emplace_back(BBIds[n]);
   // }
}

std::set<unsigned> CDSSA::getSSA()
{
    return finalssa;
}


std::set<unsigned>  CDSSA::getWCC()
{
    std::set<unsigned> wcc;
    if(computeSSA) return wcc;
    llvm::BitVector WgNodes(size);
    for(auto v:defs)
        DefVect[v]=true;
   // std::set<unsigned> Wg=overapproxWXFP(vDefs);
    std::set<unsigned> Wg=overapproxWXFPHeu();
    for(auto n:Wg)
        WgNodes[n]=1;
    wcc=checkReachability(WgNodes,defs);
    return wcc;
}

std::set<unsigned> CDSSA::getReachableNodes(llvm::BitVector reachableNodeVect, std::set<unsigned> FromNodes)
{
    std::queue<std::pair<unsigned,unsigned>> Q;
    for(auto n:FromNodes){
        Q.push(std::pair<unsigned,unsigned>(n,n));
    }
    
    std::set<unsigned> ReachableNodes;
    llvm::BitVector V(size);
    while(!Q.empty())
    {
        std::pair<unsigned,unsigned> np=Q.front();
     //   outs()<<"Visiting nodes: "<<np.first<<", "<<np.second<<"\n";
        Q.pop();
        V[np.second]=true;
        if(reachableNodeVect[np.second])
        {
            ReachableNodes.insert(np.first);
            continue;
        }
     //   if(reachableNodeVect[np.second]) outs()<<"Use of "<<np.second<<" true\n";
     //   else outs()<<"Use of "<<np.second<<" False\n";
        llvm::BasicBlock* B=BBIds[np.second];
        if(!B) continue;
        for(auto nB:llvm::successors(B))
        //llvm::succ_iterator ib=llvm::succ_begin(B);
        //llvm::succ_iterator ie=llvm::succ_end(B);
        //for(;ib!=ie;ib++)
        {
            //if(!(*ib)) continue;
            if(!nB) continue;
            unsigned nBid=BBNumbers[nB];
         //   if(!V[nBid])
            //    outs()<<np.first<<" and "<<nBid<<" are inserted to queue\n";
                Q.push(std::pair<unsigned,unsigned>(np.first,nBid));
        
        }
    }
    return ReachableNodes;
}


void CDSSA::backwardReachability(llvm::BitVector &vect, std::set<unsigned> &initNodes)
{
    outs()<<"Performing backward analysis\n";
    std::queue<unsigned> Q;
    for(auto n:initNodes){
        Q.push(n);
    }
    llvm::BitVector V(size);
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V[n]=true;
        vect[n]=true;
        llvm::BasicBlock* B=BBIds[n];
        if(!B) continue;
        for(auto B:predecessors(B)){
            if(!B) continue;
            unsigned bid=BBNumbers[B];
            if(!V[bid])
                Q.push(bid);
        }
    }
    outs()<<"Backward analysis terminated\n";

}

std::set<unsigned> CDSSA::checkReachability(llvm::BitVector wccPre, std::set<unsigned> Defs)
{
    std::queue<unsigned> Q;
    for(auto n:Defs){
        Q.push(n);
    }
    
    std::set<unsigned> wcc;
    llvm::BitVector V(size);
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V[n]=true;
       // llvm::errs()<<"Visiting node "<<n->getBlockID()<<"\n";
        if(wccPre[n]) wcc.insert(n);
        
        llvm::BasicBlock* B=BBIds[n];
        if(!B) continue;
        llvm::succ_iterator ib=llvm::succ_begin(B);
        llvm::succ_iterator ie=llvm::succ_end(B);
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned blockid=BBNumbers[(*ib)];
            if(!V[blockid])
                Q.push(blockid);
        
        }
    }
    return wcc;
}


std::set<unsigned>  CDSSA::overapproxWXFP()
{
    std::queue<unsigned> Q;
    std::map<unsigned,std::set<std::pair<unsigned,unsigned> >> Pre;
    unsigned UNINITVAL=size+10;
    unsigned Infty=5*size;
    std::set<unsigned> WX;
    llvm::BitVector V(size);
   
    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        unsigned blockid=BBNumbers[&B];
        Pre[blockid]=std::set<std::pair<unsigned,unsigned>>();
    }
    
    for(auto n:defs)
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
        
    //    llvm::errs()<<"Visiting "<<currNode<<"\n";
        
        llvm::BasicBlock *B=BBIds[currNode];
       
        for(auto Badjfront:ABIter.adj(B))
        {
            if(!Badjfront) continue;
            unsigned n=BBNumbers[Badjfront];
                        
            std::set<std::pair<unsigned,unsigned>> S;
            std::set<unsigned> Sx;
           
            for(auto Badjback:ABIter.adjRev(Badjfront)){
                if(!Badjback) continue;
                unsigned m=BBNumbers[Badjback];
                if(!Pre[m].empty()){
                   S.insert(*Pre[m].begin());
                   Sx.insert((*Pre[m].begin()).first);
                    
               //     llvm::errs()<<"("<<(*Pre[m].begin()).first<<", "<<(*Pre[m].begin()).second<<") ";
                }
            }
            
            
            
            std::set<std::pair<unsigned,unsigned>> Y;
            if(defs.find(n)!=defs.end())
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
            
        //    llvm::errs()<<"("<<(*Y.begin()).first<<", "<<(*Y.begin()).second<<") ";
            
            if(Sx.size()>1)
            {
                WX.insert(n);
        //        llvm::errs()<<"Inserted into WX "<<n<<"\n";
            }
            if(Y.empty()) continue;
            else if(Pre[n].empty() && !Y.empty()) {
                Pre[n]=Y;
      //          llvm::errs()<<"Pre[n] gets Y as Pre[n] is empty but Y is not:"<<n<<"\n";
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
       //                llvm::errs()<<"Pre[n] gets Y as Y is smaller than Pre[n]:("<<y.first<<", "<<y.second<<")\n";
                      if(!V[n]){
                       Q.push(n);
                       V[n]=true;
                      }
                    }
              }
       }
    }
    /*
    for(auto *B:*cfg){
        unsigned m=B->getBlockID();
         llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        llvm::BasicBlock *B=BBIds[n];
        llvm::BasicBlock::succ_iterator ib=B->succ_begin();
        llvm::BasicBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    std::map<unsigned,unsigned > Prem;
    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        unsigned n=getBlockID(&B);
        if((Pre[n]).size()==0) Prem[n]=UNINITVAL;
        else Prem[n]=(*Pre[n].begin()).first;
       
    }
    
    std::set<unsigned> Wg=excludeImpreciseResults(WX,defs,Prem);
  //  llvm::outs()<<"FixPoint: "<<WX.size()<<" ExcludeImpreciseResults: "<<Wg.size()<<"\n";
    /*
    llvm::errs()<<"\n";
    llvm::errs()<<"\n SSA nodes are (FP method): "<<curr_var<<"\n";
    for(auto n:WX)
    {
        llvm::BasicBlock *B=BBIds[n];
        std::set<unsigned> S;
        for(auto Badjfront:(B->*adjBar)())
        {
            if(!Badjfront) continue;
            unsigned m=Badjfront->getBlockID();
            for(auto z:Pre[m])
                S.insert(z.first);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }
    
    llvm::errs()<<"WG... ";
    for(auto n:Wg)
        llvm::errs()<<n<<", ";
     
   */
    return Wg;
}

std::set<unsigned>  CDSSA::overapproxWXFPOptimized()
{
    std::queue<unsigned> Q;
    std::map<unsigned,unsigned> Pre;
    
    //llvm::outs()<<"Running the FP operation\n";
    
    unsigned UNINITVAL=size+10;
    std::set<unsigned> WX;
    llvm::BitVector V(size);
    llvm::BitVector DEFN(size);

   
    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        Pre[getBlockID(&B)]=UNINITVAL;
       // DEFN[B->getBlockID()]=UNINITVAL;
    }
    
    for(auto n:defs)
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
        
     //   llvm::errs()<<"Visiting "<<currNode<<"\n";
        
        llvm::BasicBlock *B=BBIds[currNode];
       
        for(auto Badjfront:ABIter.adj(B))
        {
            if(!Badjfront) continue;
            unsigned n=getBlockID(Badjfront);
            std::set<unsigned> Sx;
            for(auto Badjback:ABIter.adjRev(Badjfront)){
                if(!Badjback) continue;
                unsigned m=getBlockID(Badjback);
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
    /*
    for(auto *B:*cfg){
        unsigned m=B->getBlockID();
         llvm::errs()<<"Pre ["<<m<<"]= "<<Pre[m]<<" ";
    }
    llvm::errs()<<"\n";
    llvm::errs()<<"\n WCC nodes are (in graph ranking based method): \n";
    for(auto n:wcc)
    {
        if(DefVect[n]) continue;
        std::set<unsigned> S;
        
        llvm::BasicBlock *B=BBIds[n];
        llvm::BasicBlock::succ_iterator ib=B->succ_begin();
        llvm::BasicBlock::succ_iterator ie=B->succ_end();
        for(;ib!=ie;ib++){
            if(!(*ib)) continue;
            unsigned m=(*ib)->getBlockID();
            if(Pre[m]!=UNINITVAL) S.insert(Pre[m]);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }*/
    
    std::map<unsigned,unsigned > Prem;
    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        unsigned n=getBlockID(&B);
        if(Pre[n]==UNINITVAL) Prem[n]=UNINITVAL;
        else Prem[n]=Pre[n];
       
    }
    
    //llvm::outs()<<"Exclude Imprecision\n";
    
    std::set<unsigned> Wg=excludeImpreciseResults(WX,defs,Pre);
 //   llvm::outs()<<"FixPoint: "<<WX.size()<<" ExcludeImpreciseResults: "<<Wg.size()<<"\n";
    /*
    llvm::errs()<<"\n";
    llvm::errs()<<"\n SSA nodes are (FP method): "<<curr_var<<"\n";
    for(auto n:WX)
    {
        llvm::BasicBlock *B=BBIds[n];
        std::set<unsigned> S;
        for(auto Badjfront:(B->*adjBar)())
        {
            if(!Badjfront) continue;
            unsigned m=Badjfront->getBlockID();
            for(auto z:Pre[m])
                S.insert(z.first);
        }
        llvm::errs()<<n<<" (";
        for(auto x:S) llvm::errs()<<x<<", ";
        llvm::errs()<<")\n";
    }
    
    llvm::errs()<<"WG... ";
    for(auto n:Wg)
        llvm::errs()<<n<<", ";
     
   */
    return Wg;
}



std::set<unsigned>  CDSSA::overapproxWXFPHeu()
{
    std::queue<unsigned> Q;
    typedef std::pair<unsigned,std::set<unsigned>> Elem;
    std::map<unsigned,Elem> Pre;
    std::map<unsigned,std::set<unsigned>> AllUndefPhi;
    
    unsigned UNINITVAL=size+10;
    std::set<unsigned> WX;
    llvm::BitVector V(size);
    llvm::BitVector DEFN(size);
    llvm::BitVector undefWX(size);


    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        Pre[getBlockID(&B)]=Elem(UNINITVAL,std::set<unsigned>());
    }
    
    for(auto n:defs)
    {
        Pre[n]=Elem(n,std::set<unsigned>());
        Q.push(n);
        V[n]=true;
        DEFN[n]=true;
 //       outs()<<n<<" is original def\n";
    }
    
  std::set<unsigned> reqDefs;
  while(1){
  std::set<unsigned> Z;
    while(!Q.empty())
    {
        unsigned currNode=Q.front();
        Q.pop();
        V[currNode]=false;
        llvm::BasicBlock *B=BBIds[currNode];
   //     outs()<<"visiting "<<currNode<<"\n";
        for(auto Badjfront:ABIter.adj(B))
        {
            if(!Badjfront) continue;
            unsigned n=getBlockID(Badjfront);
            
    //        outs()<<" its successor : "<<n<<"\n";
            
            std::set<unsigned> undef,udef;
            std::set<Elem> Sx;
            std::set<unsigned> Sz;
            for(auto Badjback:ABIter.adjRev(Badjfront)){
                if(!Badjback) continue;
                unsigned m=getBlockID(Badjback);
                
          /*      outs()<<" Predecessor of "<<n<<"is "<<m<<"\n";
                outs()<<"Pre["<<m<<"]= "<<Pre[m].first<<", {";
                for(auto k:Pre[m].second) outs()<<k<<" ";
                outs()<<"}\n";
           */

                if(Pre[m].first!=UNINITVAL){
                    Sx.insert(Pre[m]);
                    undef.insert(Pre[m].second.begin(),Pre[m].second.end());
                    Sz.insert(Pre[m].first);
                }
                else {
                   // undef.insert(m);
                    udef.insert(m);
                }
            }
            
        /*    outs()<<"Undef: ";
            for(auto x:undef) outs()<<x<<" ";
            outs()<<"\n";
            
            outs()<<"Sx: ";
            for(auto x:Sx) outs()<<x.first<<" ";
            outs()<<"\n";
         */
            
            if(Sz.size()>1)
            {
                WX.insert(n);
                if(!V[n] && Pre[n].first!=n) //&& elemValue[n]!=n
                {
                    Q.push(n);
                    V[n]=true;
                }
                Pre[n]=Elem(n,std::set<unsigned>());
                Z.insert(undef.begin(),undef.end());
               // for(auto X:Sx)
               //     if(!X.second.empty())
               //         Pre[n].second.insert(X.second.begin(),X.second.end());
             //   for(auto y:undef)
                   // if(UseVect[n])
            //            Pre[n].second.insert(y);
                  //  Pre[n].second.insert(undef.begin(),undef.end());

          
         /*      outs()<<n<<" is included: ";
               for(auto x:Sx) outs()<<x.first<<" ";
               outs()<<"\n";
        */
                
            }
            else{
                Elem oldVal=Pre[n];
              //  if(DEFN[n] && !Sz.empty() && !udef.empty())

                if( !Sz.empty() && !udef.empty())
                {
           //         outs()<<n<<" included in Z\n";
                    Z.insert(n);
                }
                if(DEFN[n] || Pre[n].first==n) continue;
                else {
                    Elem e=*Sx.begin();
               //     std::set<unsigned> S=e.second;
              //      if(!udef.empty() && UseVect[n]) S.insert(n);
                    if(!udef.empty()) undef.insert(n);
                    else undef=std::set<unsigned>();
                    Pre[n]=Elem(e.first,undef);
                }
                if(!V[n] && !(Pre[n].first==oldVal.first)){
                    Q.push(n);
                    V[n]=true;
                }
            }
    /*        outs()<<"Pre["<<n<<"]= "<<Pre[n].first<<", {";
            for(auto k:Pre[n].second) outs()<<k<<" ";
            outs()<<"}\n";
     */
       }
    }
      
       for(auto m:Z){
        //   if(undefWX[n]) continue;
     //     outs()<<"Considering "<<m<<"\n";
        //   if(!LiveVect[m]) continue;
       //    for(auto m:Pre[n].second){
               
       //        outs()<<"Working on "<<m<<"\n";
                   
               llvm::BasicBlock *B=BBIds[m];
               bool is_uninit=false,is_def=false;
           for(auto pB:predecessors(B)){
           //    outs()<<Pre[getBlockID(pB)].first<<" ";
                   if(Pre[getBlockID(pB)].first==UNINITVAL)
                       is_uninit=true;
                   else
                       //if(DefVect[Pre[getBlockID(pB)].first])
                       is_def=true;
           }
                    if(is_uninit && is_def)
                   {
                       Pre[m]=Elem(m,std::set<unsigned>());
                       WX.insert(m);
                       Q.push(m);
                       V[m]=true;
           //            outs()<<m<<" is included\n";
                       reqDefs.insert(m);
                   }
             
       }
      if(Q.empty()) break;
  }
    
    std::map<unsigned,unsigned > Prem;
    for(auto &B:*F)
    {
        llvm::BasicBlock *BB=&B;
        if(!(BB)) continue;
        unsigned n=getBlockID(&B);
        Prem[n]=Pre[n].first;
    //    outs()<<"Pre["<<n<<"]="<<Pre[n].first<<"\n";
       
    }
    
  /*
    outs()<<"Defining Blocks: ";
    for(auto n:defs)
        outs()<<n<<" ";
    outs()<<"\WX sets: ";
    for(auto n:WX)
        outs()<<n<<" ";
   */
    
 //   outs()<<"Algorithm 1 ends\n";
    std::set<unsigned> Wg=excludeImpreciseResults(WX,defs,Prem,&reqDefs);
    return Wg;
}

std::set<unsigned> CDSSA::excludeImpreciseResults(std::set<unsigned> WX,std::set<unsigned> Defs, std::map<unsigned,unsigned > &Pre, std::set<unsigned> *reqDefs)
{
    std::map<unsigned, unsigned> Acc;
    std::map<unsigned,bool> isFinal;
    std::map<unsigned, edgeType> solnGraph;
    unsigned UNINITVAL=size+10;
    llvm::BitVector V1(size+1);
    llvm::BitVector wccNode(size+1);
    std::queue<unsigned> Q;
    std::set<unsigned> Wg;
    
    for(auto n:WX)
    {
        isFinal[n]=false;
        wccNode[n]=false;
        
        llvm::BasicBlock *B=BBIds[n];
        for(auto Badjback:ABIter.adjRev(B)){
            if(!Badjback) continue;
            unsigned m=getBlockID(Badjback);
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
    if(reqDefs){
        unsigned undefNode=size;
        for(auto n:*reqDefs)
        {
            solnGraph[undefNode].first.insert(Pre[n]);
            solnGraph[Pre[n]].second.insert(undefNode);
        }
        Q.push(undefNode);
        V1[undefNode]=true;
        Acc[undefNode]=undefNode;
        wccNode[undefNode]=false;
        DefVect[undefNode]=true;

    }
    
 //  printGraph(solnGraph);
    
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V1[n]=false;
        std::set<unsigned> S;
        unsigned solvedNode;
        std::set<unsigned> unsolvedNode;
    //    outs()<<"Reading "<<n<<"\n";
        for(auto m:solnGraph[n].second){
  //          outs()<<"considering "<<m<<"Acc: "<<Acc[m]<<"\n";
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
     //       llvm::errs()<<n<<" is included\n";
        }
        else  // if(existUninit)
        {
            if(existInitWitness(solnGraph,unsolvedNode,solvedNode,wccNode))
            {
                Acc[n]=n;
                isFinal[n]=true;
                Wg.insert(n);
                if(!DefVect[n])
                    wccNode[n]=true;
   //              llvm::errs()<<n<<" is included\n";
            }
            else
            {
                isFinal[n]=true;
                if(Acc[n]!=n)
                    Acc[n]=Acc[solvedNode];
                
                Pre[n]=Acc[solvedNode];   // ******************Added for SCC computation
  //               llvm::errs()<<n<<" is excluded\n";
            }
        }
        for(auto m:solnGraph[n].first){
            if(!V1[m] && !isFinal[m]) {
                Q.push(m);
                V1[m]=true;
            }
        }
    }
    Wg.erase(size);
    return Wg;
}
    

bool CDSSA::existInitWitness(std::map<unsigned, edgeType> solnGraph,std::set<unsigned> startNodes, unsigned diffFrom, llvm::BitVector Res){
     llvm::BitVector V(size+1);
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
    std::queue<llvm::BasicBlock *> Q;
    llvm::BitVector inQ(size+1);
    for(auto u:Vp){
        //if(BBIds.find(u)==BBIds.end())
        //  continue;
        llvm::BasicBlock* uB= BBIds[u];
        if(!uB) continue;
     //   llvm::succ_iterator ib=uB->succ_begin();
      //  llvm::succ_iterator ie=uB->succ_end();
      //  for(;ib!=ie;ib++)
        for(auto BB:llvm::successors(uB))
        {
            if(BB){
                Q.push(BBIds[getBlockID(BB)]);
                inQ[getBlockID(BB)]=true;
            }
        }
    }
    
    llvm::BitVector V(size);
    std::set<std::pair<unsigned,unsigned> > Res;
    
    while(!Q.empty())
    {
        llvm::BasicBlock *n=Q.front();
        Q.pop();
        inQ[getBlockID(n)]=false;
        V[getBlockID(n)]=true;
     //   llvm::succ_iterator ib=n->succ_begin();
     //   llvm::succ_iterator ie=n->succ_end();
     //   for(;ib!=ie;ib++)
        for(auto nB: llvm::successors(n))
        {
          if(nB)  {
            Res.insert(std::pair<unsigned,unsigned>(getBlockID(n),getBlockID(nB)));
            if(!V[getBlockID(nB)]){
                    Q.push(nB);
                    inQ[getBlockID(nB)]=true;
                }
            }
        }
    }
    return Res;
}

std::set<unsigned> CDSSA::filterOutUnreachablenodes(std::set<unsigned> X,std::set<unsigned> Gamma)
{
    llvm::BitVector inGamma(size);
    llvm::BitVector visit(size);
    for(auto n:Gamma)
        inGamma[n]=true;
    std::set<unsigned> Y(X),GammaR;
    while(!Y.empty())
    {
        unsigned n=*Y.begin();
        Y.erase(Y.begin());
        visit[n]=true;
        if(inGamma[n]) GammaR.insert(n);
        llvm::BasicBlock *B=BBIds[n];
        if(!B) continue;
        for(auto pB:llvm::successors(B))
        {
            if(!(pB)) continue;
            if(!visit[getBlockID(pB)]) Y.insert(getBlockID(pB));
        }
    }
    return GammaR;
}






void CDSSA::printGraph()
{
    llvm::BasicBlock* src=&(F->getEntryBlock());
    std::queue<llvm::BasicBlock*> W;
    W.push(src);
    llvm::BitVector V(size);
    V[getBlockID(src)]=true;
    std::ofstream fout ("/tmp/temp.dot");
    //fout << endl
    //<< "-----file_with_graph_G begins here-----" << endl
    fout<< "digraph G {" << std::endl;
   // fout<<"  node [shape=circle];"<<endl;
    std::map<unsigned,std::string> nodes;
   // std::map<unsigned, edgeType>::iterator itb=graphCDNet.begin();
    //while(itb!=graphCDNet.end())
    for(auto &B:*F)
    {
        //if(DefVect[itb->first])
          //  fout <<itb->first<< "[fontcolor=blue label ="<<"\"" << itb->first<<"  | rank: "<<graphRank[itb->first]<<"  \""<<"]"<<endl;
        if(DefVect[getBlockID(&B)])
            fout <<getBlockID(&B)<< "[fontcolor=red label ="<<"\"" << getBlockID(&B)<<" \""<<"]"<<std::endl;
        else
            fout <<getBlockID(&B)<< "[label ="<<"\"" << getBlockID(&B)<<"  \""<<"]"<<std::endl;
    }
        
    while(!W.empty())
    {
        llvm::BasicBlock* x=W.front();
        W.pop();
       // llvm::BasicBlock::succ_iterator ib=x->succ_begin();
       // llvm::BasicBlock::succ_iterator ie=x->succ_end();
        //for(auto m:graphCDNet[n].first)
      //  for(;ib!=ie;ib++)
        for(auto xB:llvm::successors(x))
        {
            if(!(xB)) continue;
                fout << getBlockID(x) <<" -> " << getBlockID(xB) <<std::endl;
            if(!V[getBlockID(xB)]) {V[getBlockID(xB) ]=true; W.push(xB);}
        }
    }
    fout << "}" << std::endl;
   system("open /tmp/temp.dot");
}

void CDSSA::printGraph(std::map<unsigned, edgeType> graph)
{
    unsigned src=getBlockID(&(F->getEntryBlock()));
    std::queue<unsigned> W;
    W.push(src);
    llvm::BitVector V(size);
    V[src]=true;
    std::ofstream fout ("/tmp/soln.dot");
    //fout << endl
    //<< "-----file_with_graph_G begins here-----" << endl
    fout<< "digraph G {" << std::endl;
    // fout<<"  node [shape=circle];"<<endl;
    std::map<unsigned,std::string> nodes;
    std::map<unsigned, edgeType>::iterator itb=graph.begin();
    while(itb!=graph.end())
    {
        //if(DefVect[itb->first])
        //  fout <<itb->first<< "[fontcolor=blue label ="<<"\"" << itb->first<<"  | rank: "<<graphRank[itb->first]<<"  \""<<"]"<<endl;
        if(DefVect[itb->first])
            fout <<itb->first<< "[fontcolor=red label ="<<"\"" << itb->first<<" \""<<"]"<<std::endl;
        else
            fout <<itb->first<< "[label ="<<"\"" << itb->first<<"  \""<<"]"<<std::endl;
        itb++;
    }
    
    itb=graph.begin();
    while(itb!=graph.end())
    {
        for(auto n:itb->second.first)
            fout << itb->first <<" -> " << n<<std::endl;
        itb++;
    }
    fout << "}" << std::endl;
    system("open /tmp/soln.dot");
}

