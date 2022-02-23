<h1>Control dependency, SSA transformation, and their duality</h1>

This repository contains the following two subdirectories: 

1. WCCPhi - It is a Clang-based program analysis tool that computes control dependencies, weak control closure, and phi nodes (required for SSA computation)
2. SSA - It is implemented as an LLVM pass to transform any LLVM IR program to the SSA form

Both are implemented and tested in LLVM/Clang version 11.

<h2> Setup and Installation of SSA and WCCPhi </h2>

WCCPhi can run as a standalone tool built using the [Libtooling](https://clang.llvm.org/docs/LibTooling.html) library support. To build WCCPhi, do the following:
1. Put WCCPhi in (your llvm directory)/clang/tools directory,
2. Add the command "add_clang_subdirectory(WCCPhiAnal)" in the CMakeLists.txt file in the tools directory,
3. build the WCCPhi project of llvm which will generate the executable named wccphigen.
