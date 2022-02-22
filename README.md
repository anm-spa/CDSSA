Control dependency, SSA transformation, and their duality

This repository contains the following two subdirectories: 

1. SSA - It is implemented as an LLVM pass to transform any LLVM IR program to the SSA form
2. WCCPhi - It is a Clang-based program analysis tool that computes control dependencies, weak control closure, and phi nodes (required for SSA computation)
