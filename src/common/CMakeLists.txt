add_library(dark_common INTERFACE)
# target_sources(dark_common INTERFACE 
#     big_num.cpp
# )

target_link_libraries(dark_common INTERFACE PkgConfig::gmpxx)

target_link_libraries(dark_core INTERFACE dark_common)