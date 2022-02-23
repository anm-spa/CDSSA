import sys
import re
from verify import *
import os

curdir="Outputs/"
llvmdir="/Users/userid/DevTools/llvm-xcodeNew/"
fcsv=llvmdir+"llvm-project/build/test/CDSSA/Revision/" + curdir

def writetocsv(FD,fname):
    header = ['FileName', 'TotalInstm2r', 'TotalInstdual', 'PhiInsertedm2r', 'PhiInserteddual','Phim2r','Phidual']
    with open(fcsv+fname, 'w') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        TotalInstLive=0
        TotalInstnoLive=0
        PhiInsertedLive=0
        PhiInsertednoLive=0
        PhiLive=0
        PhinoLive=0
        
        for key, S in FD.items():
            TILive=0
            TInoLive=0
            PILive=0
            PInoLive=0
            PLive=0
            PnoLive=0
            if isinstance(S[0], int):
                TILive=int(S[0])
                TotalInstLive = TotalInstLive + TILive
            if isinstance(S[1], int):
                PILive=int(S[1])
                PhiInsertedLive=PhiInsertedLive+PILive
            if isinstance(S[2], int):
                PLive=int(S[2])
                PhiLive=PhiLive+PLive
            if isinstance(S[3], int):
                TInoLive=int(S[3])
                TotalInstnoLive=TotalInstnoLive+TInoLive
            if isinstance(S[4], int):
                PInoLive=int(S[4])
                PhiInsertednoLive=PhiInsertednoLive+PInoLive
            if isinstance(S[5], int):
                PnoLive=int(S[5])
                PhinoLive=PhinoLive+PnoLive
            row=[key,TILive, TInoLive, PILive, PInoLive, PLive, PnoLive]
            writer.writerow(row)
                
        row=['FinalResults',TotalInstLive, TotalInstnoLive, PhiInsertedLive,PhiInsertednoLive,PhiLive,PhinoLive]
        writer.writerow(row)
        
    
def processStats(InfoFile):
    #header = ['FileName', 'TotalInst', 'Phi Inserted', 'Phi']
    #csvf=open(fcsv, 'w')
    #writerf = csv.writer(csvf)
    #writerf.writerow(header)
    #fhandle= open(fout,"a")
    #fhandle.write("\n" + InfoFile + "\n")
    file=open(InfoFile,'r')
    # reading each line
    filename=""
    totalInst=0
    phiInst=0
    localTotal=0
    localPhi=0
    localPhiEarlier=0
    filedict={}
    sourcesdict={}
    id=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        #print(words)
        if words[0].isdigit() and words[1]=="instcount":
            temp=' '.join(words[3:len(words):1])
            if temp == "Number of instructions (of all types)":
                localTotal = localTotal + int(words[0])
                #print("\n Total Inserted")
        elif words[0].isdigit() and words[1]=="mem2reg":
            temp=' '.join(words[3:len(words):1])
            if temp == "Number of PHI nodes inserted":
                localPhi=localPhi+int(words[0])
        elif words[0].isdigit() and words[1]=="instcount":
            temp=' '.join(words[3:len(words):1])
            if temp == "Number of PHI insts":
                localPhiEarlier=localPhiEarlier+int(words[0])
        elif words[0]=="Analizing":
            id=id+1
            if localTotal !=0 or localPhi!=0 or localPhiEarlier !=0:
                #sourcesdict[(filename,id)] =[localTotal, localPhi, localPhiEarlier]
                sourcesdict[filename] =[localTotal, localPhi, localPhiEarlier]

                localTotal=0
                localPhi=0
                localPhiEarlier=0
            filename=words[2]
            #print("\n File" + filename + "read")
        else:
                continue
                
    if localTotal !=0 or localPhi!=0 or localPhiEarlier !=0:
        #sourcesdict[(filename,id)] =[localTotal, localPhi, localPhiEarlier]
        sourcesdict[filename] =[localTotal, localPhi, localPhiEarlier]
    file.close()
    return sourcesdict

def joinDict(D1,D2):
    joinD={}
    for key, S1 in D1.items():
        if D2.get(key) is None:
            S2=[]
        else:
            S2=D2[key]
        if len(S1)==0:
            S1=[-1,-1,-1]
        if len(S2)==0:
            S2=[-1,-1,-1]
        joinD[key] =S1+S2
    return joinD

if __name__ == "__main__":
    FV1=processStats(curdir+"m2rlivestats.txt")
    FV2=processStats(curdir+"duallivestats.txt")
    FV3=processStats(curdir+"m2rstats.txt")
    FV4=processStats(curdir+"dualstats.txt")
    FV12=joinDict(FV1,FV2)
    FV34=joinDict(FV3,FV4)
    writetocsv(FV12,"statslive.csv")
    writetocsv(FV34,"stats.csv")
   
