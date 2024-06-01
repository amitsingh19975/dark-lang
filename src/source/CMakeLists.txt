add_library(dark_source_buffer INTERFACE)
target_sources(dark_source_buffer INTERFACE 
    source_buffer.cpp
)

target_link_libraries(dark_core INTERFACE dark_source_buffer)