<h1>Control dependency, SSA transformation, and their duality</h1>

This repository contains the following two subdirectories: 

1. <strong>WCCPhiAnal</strong> - It is a Clang-based program analysis tool that computes control dependencies, weak control closure, and phi nodes (required for SSA computation)
2. <strong> SSA </strong> - It is implemented as an LLVM pass to transform any LLVM IR program to the SSA form

Both are implemented and tested in LLVM/Clang version 11.

<h2> Setup and Installation of SSA and WCCPhiAnal </h2>

<strong> WCCPhiAnal </strong> can run as a standalone tool built using the [Libtooling](https://clang.llvm.org/docs/LibTooling.html) library support. To build WCCPhiAnal, do the following:
1. Copy <strong> WCCPhiAnal </strong> in <code >(your llvm directory)/clang/tools </code> directory,
2. Add the instruction <code>add_clang_subdirectory(WCCPhiAnal) </code> in the <strong>CMakeLists.txt </strong> file in the tools directory,
3. Build the WCCPhiAnal project by building LLVM (or selectively build projects in LLVM)  which will generate the <strong> wccphigen </strong> executable. See the details of how to build [Clang/LLVM](https://llvm.org/docs/GettingStarted.html).


<strong> SSA </strong> is an LLVM pass which can be built as follows:
1. Copy <strong> SSA </strong> to the directory  <code >(your llvm directory)/llvm/lib/Transforms </code> directory,
2. Add the instruction <code>add_subdirectory(SSA) </code> in the <strong>CMakeLists.txt </strong> file in the Transforms directory,
3. Build SSA by building LLVM (or selectively build projects in LLVM)  which will generate the <strong> SSA.dylib </strong> library in the Debug/lib/ directory of your LLVM build directory. 
