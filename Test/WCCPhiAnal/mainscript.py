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


def analyzeWCC(comm,file,time, fileOnly):
    ofile="CD.txt"
    
    #outputs=["Outputs/m2rlive", "Outputs/m2r","Outputs/duallive","Outputs/duallivestart","Outputs/dual"]
    #for ofile in outputs:
    with open(ofile, "a") as fobject:
        fobject.write(fileOnly + ": ")
    
    comm0="../../../../Debug/bin/wccphigen "
    
    comm1=comm0 + " "+ file + " -- " + comm + ">> " + ofile + " 2> /dev/null"
    print(comm1)
    k=0
    while (k < time):
        print("Pass :" + str(k+1))
        os.system(comm1)
        k=k+1
        
if __name__ == "__main__":
    f = open('SDL.json')
    data = json.load(f)
    for i in data:
        file=i['file']
        #print(file)
        
    
        res=i['command']
        start = i['command'].find('-o')
        mid= i['command'].find('-c')
        res=i['command']
        res1= res[0:start].split(' ', 1)[1]
        #+ res[mid:len(res)]
        file=res[mid+3:len(res)]
        #res1=' '.join([ t for t in res.split() if t.startswith('-I') ])
        fileOnly=ntpath.basename(file)
        #print(res1)
        #print("Analyzing " + fileOnly + "...\n")
        analyzeWCC(res1,file,10,fileOnly)
        f.close()
    
