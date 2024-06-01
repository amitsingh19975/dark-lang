add_library(dark_diagnostic INTERFACE)
target_sources(dark_diagnostic INTERFACE 
    diagnostic_kind.cpp
    diagnostic_consumer.cpp
)

target_link_libraries(dark_core INTERFACE dark_diagnostic)