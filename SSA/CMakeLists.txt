# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

add_llvm_library(SmartSSA MODULE BUILDTREE_ONLY
  ssa_main.cpp
  SmartMem2Reg.cpp
  SmartMem2Reg.h
  VerifySSA.h
  VerifySSA.cpp
  CDSSA.h
  CDSSA.cpp
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
