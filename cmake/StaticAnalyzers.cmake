option(ENABLE_CPPCHECK "Enable cppcheck" OFF)
option(ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)

if (ENABLE_CPPCHECK)
    find_program(CPPCHECK cppcheck)
    if (CPPCHECK)
        message(STATUS "cppcheck found: ${CPPCHECK}")
        set(CMAKE_CXX_CPPCHECK ${CPPCHECK} --enable=all --suppress=missingIncludeSystem --inconclusive)
    else()
        message(WARNING "cppcheck not found")
    endif()
endif(ENABLE_CPPCHECK)

if (ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY clang-tidy)
    if (CLANG_TIDY)
        message(STATUS "clang-tidy found: ${CLANG_TIDY}")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
    else()
        message(WARNING "clang-tidy not found")
    endif()
endif(ENABLE_CLANG_TIDY)