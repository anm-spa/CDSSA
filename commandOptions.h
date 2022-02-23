#ifndef LLVM_CLANG_COMMANDOPTIONS_H
#define LLVM_CLANG_COMMANDOPTIONS_H

#include "clang/Tooling/CommonOptionsParser.h"

enum DLevel {
    O0, O1, O2, O3
};

enum AType {
    CDDanicic, CDSAS, CDDual, CompCD, PhiDF, PhiRD, PhiDual, CompPhi
};
#define DEBUGMAX 3
static cl::OptionCategory MainCat("Perform static analysis");
static cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

cl::opt<DLevel> DL("dl", cl::desc("Choose Debug level:"),
                           cl::values(
                                      clEnumValN(O0, "none", "No debugging"),
                                      clEnumVal(O1, "Minimal debug info"),
                                      clEnumVal(O2, "Expected debug info"),
                                      clEnumVal(O3, "Extended debug info")), cl::cat(MainCat));

cl::opt<AType> AlgType("alg", cl::desc("Choose Algorithms:"),
                       cl::values(
                                  //clEnumVal(ALL, "Data flow based and Dominance Frontier/Cytron method"),
                                  clEnumVal(CDDanicic, "Compute WCC using Danicic's Algorithm"),
                                  clEnumVal(CDSAS, "Compute WCC using the method described in SAS'21"),
                                  clEnumVal(CDDual, "Compute WCC using Duality-based fixpoint algorithm"),
                                  clEnumVal(CompCD, "Compare WCC computations using all the available methods"),
                                  clEnumVal(PhiDF, "Compute Phi nodes using the dominance frontier based method (Cytron's algorithm)"),
                                  clEnumVal(PhiRD, "Compute Phi nodes using RD-based method discussed in SCAM 2019 and JSS 2020"),
                                  clEnumVal(PhiDual, "Compute Phi nodes using the duality-based fixpoint method"),
                                  clEnumVal(CompPhi, "Compare Phi nodes computations using all the available methods")), cl::cat(MainCat));
cl::opt<bool> DumpCFG("cfg", cl::desc("Dump CFG if a specific function is analyzed"));
cl::opt<bool> DumpCDG("cdg", cl::desc("Dump CDG if a specific function is analyzed"));
cl::opt<bool> DefsAtStart("defs-at-start", cl::desc("Set all definitions of program variables at the start node in computing phi nodes"));
cl::opt<bool> UserNP("np", cl::desc("Use this option if the NP sets are to be given manually (the default is to select NP randomly from the CFG nodes)"));
cl::opt<string> ProcName("proc", cl::desc("Specify function name to be analyzed"), cl::value_desc("function name"), cl::init("none"));

#endif
