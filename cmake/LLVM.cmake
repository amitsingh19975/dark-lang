find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

llvm_map_components_to_libnames(llvm_libs Support Core IRReader)

add_library(MLIR_LIB INTERFACE)
target_link_libraries(MLIR_LIB INTERFACE MLIRIR MLIRParser MLIRTransforms MLIRSupport MLIRAnalysis MLIRTransformUtils MLIRPass MLIRStandardToLLVM)

set(NO_RTTI "-fno-rtti")
add_definitions(${NO_RTTI})
