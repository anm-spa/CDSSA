<h1>Control dependency, SSA transformation, and their duality</h1>

This repository contains the following two subdirectories: 

1. <strong>WCCPhiAnal</strong> - It is a Clang-based program analysis tool that computes control dependencies, weak control closure, and phi nodes (required for SSA computation)
2. <strong> SSA </strong> - It is implemented as an LLVM pass to transform any LLVM IR program to the SSA form

Both are implemented and tested in LLVM/Clang version 11.

<h2> Setup and Installation of SSA and WCCPhiAnal </h2>

<strong> WCCPhiAnal </strong> can run as a standalone tool built using the [Libtooling](https://clang.llvm.org/docs/LibTooling.html) library support. To build WCCPhiAnal, do the following:
1. Copy <strong> WCCPhiAnal </strong> in <code >(your llvm directory)/clang/tools </code> directory,
2. Add the instruction <code>add_clang_subdirectory(WCCPhiAnal) </code> in the <strong>CMakeLists.txt </strong> file in the <code>tools</code> directory,
3. Build the WCCPhiAnal project by building LLVM (or selectively build projects in LLVM)  which will generate the <strong> wccphigen </strong> executable. See the details of how to build [Clang/LLVM](https://llvm.org/docs/GettingStarted.html).


<strong> SSA </strong> is an LLVM pass which can be built as follows:
1. Copy <strong> SSA </strong> to the directory  <code >(your llvm directory)/llvm/lib/Transforms </code> directory and rename SSA by SmartSSA (just to resolve naming conflict),
2. Add the instruction <code>add_subdirectory(SmartSSA) </code> in the <strong>CMakeLists.txt </strong> file in the <code> Transforms </code> directory,
3. Build SSA by building LLVM (or selectively build projects in LLVM)  which will generate the <strong> SmartSSA.dylib </strong> library in the <code> Debug/lib/ </code> directory of your LLVM's build directory. 

<h2>  Usage of  WCCPhiAnal (wccphigen executable)</h2>
This tool performs intraprocedural analysis of source code written in C language and can be used to compute the following:

1. Compute the weak control closure of each procedure with respect to a given subset of CFG nodes. These subset of CFG nodes can be taken randomly or given as input. WCC can be computed using various state-of-the-art methods and a novel duality-based algorithm.
  (i) Compute WCC using the method of Danicic et al.
 (ii) Computte WCC using reaching definition-based analysis
 (iii) Computte WCC using novel duality-based algorithm
2. Compute the set of PHI nodes from the CFG of each procedure and for each program variable. 

<h2>  References of implemented algorithms </h2>

[1] Danicic, Sebastian et al. “A unifying theory of control dependence and its application to arbitrary program structures.” Theor. Comput. Sci. 412 (2011): 6809-6842.

[2] Abu Naser Masud. Efficient computation of minimal weak and strong control closure. J. Syst. Softw. 184, C (Feb 2022). DOI:https://doi.org/10.1016/j.jss.2021.111140

[3] Abu Naser Masud. Simple and Efficient Computation of Minimal Weak Control Closure. in proceeding of the 27th Intl. conf. in static analysis (SAS) (2020).

[4] Abu Naser Masud. The Duality in Computing SSA Programs and Control Dependency, under review in transactions on software engineering (TSE).
