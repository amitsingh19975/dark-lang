add_subdirectory(diagnostics)
add_subdirectory(lexer)
add_subdirectory(common)

add_executable(${PROJECT_NAME} driver.cpp)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE project_options project_warnings ${llvm_libs} ${MLIR_LIB} dark_core)
