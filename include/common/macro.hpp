#ifndef __DARK_COMMON_MACRO_HPP__
#define __DARK_COMMON_MACRO_HPP__

#if defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)
    #define DARK_RESTRICT __restrict__
#elif defined(_MSVC_VER)
    #define DARK_RESTRICT __restrict
#else
    #define DARK_RESTRICT
#endif

#ifdef __has_cpp_attribute
  #if __has_cpp_attribute(assume) >= 202207L
    #define DARK_ASSUME(...) [[assume(__VA_ARGS__)]]
  #endif
#endif
// first fallback: compiler intrinsics/attributes for assumptions
#ifndef DARK_ASSUME
  #if defined(__clang__)
    #define DARK_ASSUME(...) do { __builtin_assume(__VA_ARGS__); } while(0)
  #elif defined(_MSC_VER)
    #define DARK_ASSUME(...) do { __assume(__VA_ARGS__); } while(0)
  #elif defined(__GNUC__)
    #if __GNUC__ >= 13
      #define DARK_ASSUME(...) __attribute__((__assume__(__VA_ARGS__)))
    #endif
  #endif
#endif
// second fallback: possibly evaluating uses of unreachable()
#if !defined(DARK_ASSUME) && defined(DANGEROUS_BEHAVIOR_ASSUMPTIONS_ALLOWED_TO_EVALUATE)
  #if defined(__GNUC__)
    #define DARK_ASSUME(...) do { if (!bool(__VA_ARGS__)) __builtin_unreachable(); } while(0)
  #elif __cpp_lib_unreachable >= 202202L
    #include <utility>
    #define DARK_ASSUME(...) do { if (!bool(__VA_ARGS__)) ::std::unreachable(); ) while(0)
  #endif
#endif
// last fallback: define macro as doing nothing
#ifndef DARK_ASSUME
  #define DARK_ASSUME(...)
#endif

#endif // __DARK_COMMON_MACRO_HPP__