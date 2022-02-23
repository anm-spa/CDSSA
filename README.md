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
   - Compute WCC using the method of Danicic et al.[1]. Usage option: <code> -alg=CDDanicic </code> 
   - Computte WCC using reaching definition-based analysis [2,3]. Usage option: <code> -alg=CDSAS </code> 
   - Computte WCC using novel duality-based algorithm [4]. Usage option: <code> -alg=CDDual </code>
   - Compare the computation of WCC using all the above three methods.  Usage option: <code> -alg=CompCD </code>
2. Compute the set of PHI nodes from the CFG of each procedure and for each program variable. 
   - Compute Phi nodes using the dominance frontier based method (Cytron's algorithm [5]). Usage option: <code> -alg=PhiDF </code> 
   - Compute Phi nodes using RD-based method discussed in SCAM 2019 [6] and JSS 2020 [7]. Usage option: <code> -alg=PhiRD </code> 
   - Compute Phi nodes using the duality-based fixpoint method [4]. Usage option: <code> -alg=PhiDual </code> 
   - Compare the computation of Phi nodes using all the above methods. Usage option: <code> -alg=CompPhi </code>
3. Use the option <code> -proc=(Procedure name) </code> if you want to analyze a specific procedure. Then, you can use <code> -cfg</code> or <code> -cdg </code> to see the CFG or the control dependency information of the analyzed procedure.
4. Usually, WCC is computed with respect to a set of CFG nodes. By default, this set is obtained randomly from the set of CFG nodes. However, this option can be overridden by using the option <code> -np </code> which will then allow to get a subset of CFG nodes from console.
5. Dominance frontier based computation such as [5] of Phi nodes assumes that all program variables are defined at the beginning and introduces spuriousness in computing phi nodes. However, theis assumption is relaxed in [4,6,7] by assuming that only global variables and formal parameters are beginning at the beginning. Thus, the implementation of the methods in [4,6,7] produces precise phi nodes. However, the option  <code> -defs-at-start </code> forces these methods to assume that all program variables are definied at the beginning, and then the set of phi nodes in all computations are same.

<h2>  References of implemented algorithms </h2>

[1] Danicic, Sebastian et al. “A unifying theory of control dependence and its application to arbitrary program structures.” Theor. Comput. Sci. 412 (2011): 6809-6842. 

[2] Abu Naser Masud. Efficient computation of minimal weak and strong control closure. J. Syst. Softw. 184, C (Feb 2022). DOI:https://doi.org/10.1016/j.jss.2021.111140 

[3] Abu Naser Masud. Simple and Efficient Computation of Minimal Weak Control Closure. in proceeding of the 27th Intl. conf. in static analysis (SAS) (2020). 

[4] Abu Naser Masud. The Duality in Computing SSA Programs and Control Dependency, under review in transactions on software engineering (TSE).

[5] Ron Cytron, Jeanne Ferrante, Barry K. Rosen, Mark N. Wegman,
and F. Kenneth Zadeck. Efficiently computing static single as- signment form and the control dependence graph. ACM Trans.
Program. Lang. Syst., 13(4):451–490, October 1991.

[6] A. N. Masud and F. Ciccozzi. Towards constructing the ssa form using reaching definitions over dominance frontiers. In 2019 19th International Working Conference on Source Code Analysis and Manipulation (SCAM), pages 23–33, 2019.

[7] Abu Naser Masud and Federico Ciccozzi. More precise con- struction of static single assignment programs using reaching definitions. Journal of Systems and Software, 166:110590, 2020.
