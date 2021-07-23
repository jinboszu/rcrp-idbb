add_executable(dis-solve solve.c algorithm.c report.c lowerbound.c state.c instance.c heuristic.c relocation.c timer.c)
target_compile_definitions(dis-solve PRIVATE VERBOSE)
