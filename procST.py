import sys
import re
import os
from verify import *

curdir="Outputs/"
llvmdir="/Users/userid/DevTools/llvm-xcodeNew/"
fout=llvmdir+"llvm-project/build/test/CDSSA/Revision/" + curdir + "SSARes.txt"
fcsv=llvmdir+"llvm-project/build/test/CDSSA/Revision/" + curdir + "comp.csv"

def compTimeDict(FV1,FV2):
    header = ['FileName', 'User', 'Sys']
    with open(fcsv, 'w') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        for key, row1 in FV1.items():
            if FV2.get(key) is None:
                row2=-1
            else:
                row2=FV2[key]
                row=[key,row1,row2]
                writer.writerow(row)
        
        
def genTime(InfoFile):
    filedict={}
    funcdict={}
    file=open(InfoFile,'r')
    filename=""
    user=0
    sys=0
    real=0
    localuser=0
    localreal=0
    localsys=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        if words[0]=="Analizing":
            if localreal !=0 or localsys!=0 or localuser!=0:
                user=user + localuser
                real=real+localreal
                sys=sys + localsys
                funcdict[filename]=localuser+localsys;
                #fhandle.write(filename + " "+ str(localreal) + " "+str(localuser) + " "+ str(localsys)+"\n")
            localuser=0
            localreal=0
            localsys=0
            filename=words[2]
            #fhandle.write('"'+ filename + '" ')
        else:
            rl=float(words[0])
            ur=float(words[2])
            sy=float(words[4])
            localuser = localuser + ur
            localreal = localreal + rl
            localsys = localsys + sy
    if localreal !=0 or localsys!=0 or localuser!=0:
        user=user + localuser
        real=real+localreal
        sys=sys + localsys
        funcdict[filename]=localuser+localsys;
        #fhandle.write(filename)
    tot=user+sys
    L=InfoFile.split("/")
    s=L[len(L)-1]
    funcdict[s]={user,sys};
    #fhandle.write("TotalTime: "+s +" "+ str(real) + " "+str(user) + " "+str(sys)+"  " + str(tot) +"\n")
    file.close()
    return funcdict
    #fhandle.close()
    
def processTime(InfoFile):
    fhandle= open(fout,"a")
    file=open(InfoFile,'r')
    filename=""
    user=0
    sys=0
    real=0
    localuser=0
    localreal=0
    localsys=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        if words[0]=="Analizing":
            if localreal !=0 or localsys!=0 or localuser!=0:
                user=user + localuser
                real=real+localreal
                sys=sys + localsys
                #fhandle.write(filename + " "+ str(localreal) + " "+str(localuser) + " "+ str(localsys)+"\n")
            localuser=0
            localreal=0
            localsys=0
            filename=words[2]
            #fhandle.write('"'+ filename + '" ')
        else:
            rl=float(words[0])
            ur=float(words[2])
            sy=float(words[4])
            localuser = localuser + ur
            localreal = localreal + rl
            localsys = localsys + sy
    if localreal !=0 or localsys!=0 or localuser!=0:
        user=user + localuser
        real=real+localreal
        sys=sys + localsys
        #fhandle.write(filename)

    tot=user+sys
    L=InfoFile.split("/")
    s=L[len(L)-1]
    fhandle.write("TotalTime: "+s +" "+ str(real) + " "+str(user) + " "+str(sys)+"  " + str(tot) +"\n")
    file.close()
    fhandle.close()
    
def processStats(InfoFile):
    fhandle= open(fout,"a")
    #fhandle.write("\n" + InfoFile + "\n")
    file=open(InfoFile,'r')
    # reading each line
    filename=""
    totalInst=0
    phiInst=0
    localTotal=0
    localPhi=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        #print(words)
        if words[0].isdigit() and words[1]=="instcount":
            temp=' '.join(words[3:len(words):1])
            if temp == "Number of instructions (of all types)":
                localTotal = localTotal + int(words[0])
        elif words[0].isdigit() and words[1]=="mem2reg":
            temp=' '.join(words[3:len(words):1])
            if temp == "Number of PHI nodes inserted":
                localPhi=localPhi+int(words[0])
        elif words[0]=="Analizing":
            if localTotal !=0 or localPhi!=0:
                totalInst = totalInst + localTotal
                phiInst=phiInst + localPhi
                localTotal=0
                localPhi=0
            filename=words[1]
        else:
                continue
                
    if localTotal !=0 or localPhi!=0:
        totalInst = totalInst + localTotal
        phiInst=phiInst + localPhi
    L=InfoFile.split("/")
    s=L[len(L)-1]
    fhandle.write(s + ": TotalInst: " + str(totalInst) + " PhiInst: "+str(phiInst) + "\n")
    file.close()
    fhandle.close()


def procTimes():
    print("***Begin processing time files...")
    processTime(curdir+ "m2rlivetime.txt")
    processTime(curdir+ "m2rtime.txt")
    processTime(curdir+ "duallivetime.txt")
    #processTime(curdir+ "duallivedefstarttime.txt")
    processTime(curdir+ "dualtime.txt")
    print("***Processing time files...DONE.")
    
def compareTimes():
    print("***Begin processing time files...")
    FV1=genTime(curdir+ "m2rlivetime.txt")
    #processTime(curdir+ "m2rtime.txt")
    FV2=genTime(curdir+ "duallivetime.txt")
    #processTime(curdir+ "dualtime.txt")
    print(FV1)
    compTimeDict(FV1,FV2)
    print("***Processing time files...DONE.")
    
def procStats():
    print("***Begin processing stats files...")
    processStats(curdir+ "m2rlivestats.txt")
    processStats(curdir+"m2rstats.txt")
    processStats(curdir+"duallivestats.txt")
    processStats(curdir+"dualstats.txt")
    fhandle= open(fout,"a")
    fhandle.write("\n\n")
    fhandle.close()
    
    print("***Processing stats files...DONE.")
if __name__ == "__main__":
    # code to cleanup
    #os.system("rm /Users/mar01/DevTools/llvm-xcodeNew/llvm-project/build/test/CDSSA/Revision/Outputs/538imagick/*.csv")
    #os.system("rm /Users/mar01/DevTools/llvm-xcodeNew/llvm-project/build/test/CDSSA/Revision/Outputs/538imagick/SSARes.txt")
    procStats()
    procTimes()
    compareTimes()
    verify()
   
