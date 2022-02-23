import sys
import re
import os

fout="/Users/userid/DevTools/llvm-xcodeNew/llvm-project/build/test/CDSSA/Revision/WCC-computation/"

def isfloat(num):
    if num == "nan":
        return False
    try:
        float(num)
        return True
    except ValueError:
        return False
    
def processStats(InfoFile):
    #fhandle= open(fout,"a")
    #fhandle.write("\n" + InfoFile + "\n")
    file=open(InfoFile,'r')
    # reading each line
    filename=""
    nCFG =0
    TWccFP=0
    TWCCSAS=0
    TWccDanicic=0
    tdFP=0.0
    tdDanicic=0.0
    K=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        K=K+1
        i=0
        if(K==1):
            i=1
        #print(words)
        if words[2+i].isdigit():
            nCFG=nCFG + int(words[2+i])
        if words[4+i].isdigit():
            TWccFP=TWccFP + int(words[4+i])
        if words[6+i].isdigit():
            TWCCSAS=TWCCSAS + int(words[6+i])
        if words[8+i].isdigit():
            TWccDanicic=TWccDanicic + int(words[8+i])
        
        #print(type(words[10+i]))
        if isfloat(words[10+i]):
            tdFP=tdFP+ float(words[10+i])
        if isfloat(words[12+i]):
            tdDanicic=tdDanicic+ float(words[12+i])
        
        if K==10:
            K=0
      
    file.close()
    return (nCFG, TWccFP, TWCCSAS, TWccDanicic, tdFP, tdDanicic)


if __name__ == "__main__":
    (nCFG, TWccFP, TWCCSAS, TWccDanicic, tdFP, tdDanicic)=processStats(fout+"glfw/CD.txt")
    print("nCFG = " + str(nCFG))
    print("TWccFP =" + str(TWccFP))
    print("TWCCSAS =" + str(TWCCSAS))
    print("TWccDanicic = " + str(TWccDanicic))
    print("tdFP = " + str(tdFP))
    print("tdDanicic = " + str(tdDanicic))

   
