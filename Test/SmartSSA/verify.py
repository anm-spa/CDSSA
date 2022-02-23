import sys
import re
import csv


# Change these paths to verify the results according to your test setting. The test code resides in the test/CDSSA/Revision directory of LLVM build directory. All files are generated in the Outputs directory.
llvmDir="/Users/youruserid/DevTools/llvm-xcodeNew/"
fout=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/SSARes.txt"
fcsv1=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rduallive.csv"
fcsv_diff1=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rduallive_diff.csv"

fcsv2=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rdual.csv"
fcsv_diff2=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rdual_diff.csv"

fcsv3=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rduallivedefstart.csv"
fcsv_diff3=llvmDir+"llvm-project/build/test/CDSSA/Revision/Outputs/m2rduallivedefstart_diff.csv"

def writeComparison(FD1,FD2,fcsv,fcsv_diff):
    header = ['FileName', 'FuncName and Var Id', 'M2R', 'Dual','M2R \ Dual','Dual\M2R','Comments']
    header_diff = ['FileName', 'FuncName and Var Id', 'M2R-Dual','Dual-M2R']
    f_diff=open(fcsv_diff, 'w')
    writer_diff = csv.writer(f_diff)
    writer_diff.writerow(header_diff)
    with open(fcsv, 'w') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        counter_m2r=0
        counter_dual=0
        counter=0
        for key, funcdict in FD1.items():
            #print("key= "+key)
            if FD2.get(key) is None:
                #print("***not found in FD2***")
                continue
            for k,v in funcdict.items():
                vstr=v
                if len(v)==0:
                    vstr='{}'
                #print("k= " + k)
                if FD2[key].get(k) is None:
                 #   print("---Not Found--- for " + key)
                    continue
                v2=FD2[key][k]
                w=v.difference(v2)
                wp=v2.difference(v)
                counter_m2r += len(v)
                counter_dual += len(v2)
                counter += len(w)
                v2str=FD2[key][k]
                if len(v2) ==0:
                    v2str='{}'
                res='not subset'
                if v2.issubset(v):
                    res='subset'
                row=[key,k,vstr,v2str,w,wp,res]
                writer.writerow(row)
                if len(w)>0 or len(wp)>0:
                    row_diff=[key,k,w,wp]
                    writer_diff.writerow(row_diff)
        row=['FinalResults','Total',counter_m2r,counter_dual,counter,'****']
        writer.writerow(row)
        row_diff=['FinalResults','Total',counter]
        writer_diff.writerow(row_diff)
        return (counter_m2r,counter_dual,counter)

def processPhiInfo(File):
    filedict={}
    file=open(File,'r')
    filename=""
    funcdict={}
    bigtotal=0
    for line in file:
        if len(line.split()) == 0:
            continue
        words=line.split()
        if words[0]=="Analizing":
            if len(funcdict) != 0:
                filedict[filename]=funcdict
                funcdict={}
            filename=words[2]
        elif words[0]=="Function":
            funcname=words[1]
           # print(funcname)
            i=3
            L=len(words)
            phi=set()
            while i<L:
                wp=words[i].strip("()[]),")
                i +=1
                if wp =='':
                    continue
                val=wp.split(",")
                phi.add(val[0])
            #print(phi)
            funcdict[funcname]=phi
            bigtotal=bigtotal + len(phi)
            #print("Func = " + funcname + "phi=" + str(phi))
    if len(funcdict) != 0:
        filedict[filename]=funcdict
    file.close()
    #fhandle.close()
    print(bigtotal)
    return filedict


if __name__ == "__main__":
#def verify():
    s="Outputs/"
    FD1=processPhiInfo(s+"m2rlive.txt")
    FD2=processPhiInfo(s+"duallive.txt")
    FD3=processPhiInfo(s+"m2r.txt")
    FD4=processPhiInfo(s+"dual.txt")
    FD5=processPhiInfo(s+"duallivedefstart.txt")
    fhandle= open(fout,"a")
    (m,d,diff)=writeComparison(FD1,FD2,fcsv1,fcsv_diff1)   #m2rlive vs duallive
    fhandle.write(" (m2rlive, duallive, diff) = " + str(m) + " "+str(d) + " "+str(diff) + "\n")
    
    (m,d,diff)=writeComparison(FD3,FD4,fcsv2,fcsv_diff2)   #m2r vs dual
    fhandle.write(" (m2r, dual, diff) = " + str(m) + " "+str(d) + " "+str(diff) + "\n")
    
    (m,d,diff)=writeComparison(FD1,FD5,fcsv3,fcsv_diff3)   # dual vs duallive
    fhandle.write("(m2rlive, duallivedefstart, diff) = " + str(m) + " "+str(d) + " "+str(diff)+ "\n")
    #writeComparison(FD3,FD1,fcsv4,fcsv_diff4)   #m2r vs m2rlive
    print('verification completed')
   
        
