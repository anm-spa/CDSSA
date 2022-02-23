import sys
import re
import json
import os
import ntpath


def process_time(tt):
    tsplit=re.split('[hms]',tt)
    if(len(tsplit)==4):
        snd=float(tsplit[0])*60*60+float(tsplit[1])*60 + float(tsplit[2])
    if(len(tsplit)==3):
        snd=float(tsplit[0])*60 + float(tsplit[1])
    if(len(tsplit)==2):
        snd=float(tsplit[0])
    return snd;

def processResults(Files):
    fhandle= open(fout,"a")
    for f in Files:
        fhandle.write("\n" + f + "\n")
        file=open(f,'r')
        # reading each line
        filename=""
        totalInst=0
        phiInst=0
        user=0
        sys=0
        real=0
        for line in file:
            if len(line.split()) == 0:
                continue
            words=line.split()
            if(words[0]== "real"):
                real=real + process_time(words[1])
                continue
            if(words[0]== "user"):
                user=user + process_time(words[1])
                continue
            if(words[0]== "sys"):
                sys=sys + process_time(words[1])
                continue
            if words[0]== "===-------------------------------------------------------------------------===":
                continue
            if words[0]=="Analizing":
                filename=words[2]
            elif words[0]=="PassEnd":
                lnstr1 = filename + " " +str(totalInst) + " " + str(phiInst) + " time "+ str(user+sys)+"\n"
                fhandle.write(lnstr1)
                print(lnstr1)
                filename=""
                totalInst=0
                phiInst=0
                real=0
                user=0
                sys=0
                continue
            elif words[0].isdigit() and words[1]=="instcount":
                totalInst = totalInst + int(words[0])
                if words[5]=="PHI":
                        phiInst=phiInst+int(words[0])
            elif len(words)>= 2:
                if words[1] == "Statistics":
                    continue
            else:
                continue
        file.close()
    fhandle.close()

def analyzeFile(comm,file,commtype,time):
    ofile="Outputs/"+commtype
    if commtype == "duallivedefstart":
        param="-duallive "+ "-defstart"
    else:
        param="-"+commtype
    comm0=comm + " -S -emit-llvm -O -Xclang -disable-llvm-passes -o /dev/stdout |"

    comm1stats= " ../../../Debug/bin/opt -load ../../../Debug/lib/SmartSSA.dylib -S -smartssa " + param + " -o " + "llvmFiles/" + file + ".ll -instcount -stats -disable-verify >> " + ofile+"stats.txt" + " 2>&1"
    
    k=0
    print("Analizing File: " + file)
    with open(ofile+"time.txt", "a") as fobject:
        fobject.write("Analizing File: " + file + "\n")
    with open(ofile+"stats.txt", "a") as fstats:
        fstats.write("Analizing File: " + file + "\n")
    comm1time= "../../../Debug/bin/opt -load ../../../Debug/lib/SmartSSA.dylib -S -smartssa " + param + " -o " + "llvmFiles/" + file + ".ll -disable-verify "
    comm=comm0+"time " + comm1time + " >> " + ofile+"time.txt 2>&1"
    
    comm1stats= " ../../../Debug/bin/opt -load ../../../Debug/lib/SmartSSA.dylib -S -smartssa " + param + " -o " + "llvmFiles/" + file + ".ll -instcount -stats -disable-verify >> " + ofile+"stats.txt" + " 2>&1"
    commstats=comm0+comm1stats
    
    # Execute tests for 10 consecutive runs
    while (k < time+1):
        print("Pass: " + str(k))
        if k == time:
            #print("printing stats and time")
            os.system(commstats)
        else:
            #print("printing time")
            os.system(comm)
        k=k+1
        
if __name__ == "__main__":
    # input json file incluing all build configurations
    f = open('compile_commands.json')
    data = json.load(f)
    for i in data:
        start = i['command'].find('-o')
        mid= i['command'].find('-c')
        res=i['command']
        res1= res[0:start] + res[mid:len(res)]
        file=res[mid+3:len(res)]
        fileOnly=ntpath.basename(file)
        analyzeFile(res1,fileOnly,"m2rlive",10)
        analyzeFile(res1,fileOnly,"m2r",10)
        analyzeFile(res1,fileOnly,"duallive",10)
        analyzeFile(res1,fileOnly,"dual",10)
        #analyzeFile(res1,fileOnly,"duallivedefstart",10)
    f.close()
    
