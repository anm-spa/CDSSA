//
//  VerifySSA.cpp
//  SmartSSA
//
//  Created by Abu Naser Masud on 2021-10-12.
//

#include "VerifySSA.h"

VerifyPhiNodes::VerifyPhiNodes(Function &Func, std::vector<std::pair<std::set<unsigned>,llvm::BitVector> > &PhiInfo)
{
    F=&Func;
    std::string fname=Func.getName().str();
   // outs()<<"Func: "<<fname<<" ";
    bool bb=false;
    for(auto &BB:Func){
        unsigned id;
       // if(is_number(BB.getName().str()))
        std::string s=BB.getName().str();
        std::stringstream ss(s);
        ss>>id;
       
    //    outs()<<BB.getName().str()<<"\n";
    //    id=stoi(BB.getName().str());
       if(ss.fail()){
            outs()<< BB.getName().str()<< "cannot be converted to number\n";
           bb=true;
           continue;
          //  return;
        }
        BB2ID[&BB]=id;
        ID2BB[id]=&BB;
    }
    if(bb) return;
    size=Func.getBasicBlockList().size();
    unsigned AllocaNum=0;
    for(auto PI:PhiInfo)
    {
        outs()<<"Function "<< fname<<"-"<<AllocaNum++<<" : [";
        for(auto phi:PI.first){
            unsigned pinfo=isACorrectPhiNode(phi,PI.second);
            outs()<<"("<<phi<<","<<pinfo<<"), ";
        }
        outs()<<"]\n";
    }
}


unsigned VerifyPhiNodes::isACorrectPhiNode(unsigned n, llvm::BitVector DefVect)
{
    
    std::queue<unsigned> Q;
    llvm::BasicBlock* B=ID2BB[n];
    std::map<unsigned,unsigned> paths;
    unsigned id=1;
    for(auto pB:llvm::predecessors(B))
    {
        paths[getBlockID(pB)]=id++;
        Q.push(getBlockID(pB));
    }
    llvm::BasicBlock *eB=&(F->getEntryBlock());
    paths[getBlockID(eB)]=-1;
    llvm::BitVector V(size);
    unsigned counter=0;
    while(!Q.empty())
    {
        unsigned n=Q.front();
        Q.pop();
        V[n]=true;
        if(DefVect[n]) {
            counter++;
            continue;
        }
        llvm::BasicBlock* B=ID2BB[n];
        if(!B) continue;
        for(auto pB:predecessors(B)){
            if(!pB) continue;
            unsigned bid=BB2ID[pB];
            if(!V[bid]){
                Q.push(bid);
                paths[getBlockID(pB)]=paths[n];
            }
        }
    }
    if(counter>1 )
        return 1;
    else if(counter==1 && paths[getBlockID(eB)]!=-1)
    {
        //outs()<<n<<" is a necessary PHI node due to undef\n";
        return 2;

    }
    else {
       // outs()<<n<<" is not a necessary PHI node\n";
        return 0;
    }
    
}


/*
bool VerifyPhiNodes::compare_results()
{
    for(auto m:Phis)
    {
        outs()<<"Comparing function: "<<m.first->getName()<<"Live vs M2R\n";
        for(unsigned i=0;i<m.second.size();i++)
        {
            std::set<unsigned> diff;
            std::set<unsigned> dual=m.second[i].first;
            std::set<unsigned> m2r=m.second[i].second;
            std::set_difference(dual.begin(), dual.end(), m2r.begin(), m2r.end(),
                                    std::inserter(diff, diff.begin()));
            
            if(diff.empty()) outs()<<"Alloca "<<i<< ": subset \n";
            else {
                outs()<<" NOT subset";
                for(auto n:diff)
                    outs()<<n<<" ";
                outs()<<"\n";
            }
        }
    }
    return false;
}
*/

