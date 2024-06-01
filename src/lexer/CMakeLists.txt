add_library(dark_lexer INTERFACE)
target_sources(dark_lexer INTERFACE 
    token_buffer.cpp
    token_kind.cpp
    numeric_literal.cpp
    string_literal.cpp
)

target_link_libraries(dark_core INTERFACE dark_lexer)