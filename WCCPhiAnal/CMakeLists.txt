set( SOURCES
	dummy.cpp
    dataFlowAnalysis.h
)

add_clang_executable(wccphigen
  main.cpp
  )

target_link_libraries(wccphigen
  LINK_PRIVATE clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )


#COMMAND clang -emit-ast ${SourceFile} -o /repo/emasabu/spa-test/${SourceFiles}.ast)

install(TARGETS wccphigen
  RUNTIME DESTINATION bin)
