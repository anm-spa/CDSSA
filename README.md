<h1>Control dependency, SSA transformation, and their duality</h1>

This repository contains the following two subdirectories: 

1. <strong>WCCPhiAnal</strong> - It is a Clang-based program analysis tool that computes control dependencies, weak control closure, and phi nodes (required for SSA computation)
2. <strong> SSA </strong> - It is implemented as an LLVM pass to transform any LLVM IR program to the SSA form

Both are implemented and tested in LLVM/Clang version 11.

<h2> Setup and Installation of SSA and WCCPhiAnal </h2>

<strong> WCCPhiAnal </strong> can run as a standalone tool built using the [Libtooling](https://clang.llvm.org/docs/LibTooling.html) library support. To build WCCPhiAnal, do the following:
1. Put <strong> WCCPhiAnal </strong> in (your llvm directory)/clang/tools directory,
2. Add the command <code>add_clang_subdirectory(WCCPhiAnal) </code> in the <strong>CMakeLists.txt </strong> file in the tools directory,
3. build the WCCPhiAnal project by building LLVM (or selectively build projects in LLVM)  which will generate the <strong> wccphigen </strong> executable. See the details of how to build [Clang/LLVM](https://llvm.org/docs/GettingStarted.html).

