#ifndef __DARK_COMMON_ASSERT_HPP__
#define __DARK_COMMON_ASSERT_HPP__

#include <format>
#include <llvm/Support/raw_ostream.h>
#include <string_view>

namespace dark::detail {

    inline auto assert_impl(bool cond, std::string_view file, int line, std::string_view func, std::string_view expr) -> void {
        if (cond) return;
        llvm::errs() << "Assertion failed: " << expr << '\n';
        llvm::errs() << std::format("  {}:{}: ", file, line);
        if (!func.empty()) llvm::errs() << "Function('" << func <<"')" << '\n';
        std::abort();
    }

    inline auto assert_impl(bool cond, std::string_view file, int line, std::string_view func, std::string_view expr, std::string_view message) -> void {
        if (cond) return;
        llvm::errs() << "Assertion failed: " << expr << '\n';
        llvm::errs() << std::format("  {}:{}: ", file, line);
        if (!func.empty()) llvm::errs() << "Function('" << func <<"'): ";
        llvm::errs() << message << '\n';
        std::abort();
    }

    template <typename>
    struct always_false : std::false_type {};

    template <typename T>
    static constexpr bool always_false_v = always_false<T>::value;

} // namespace dark::detail

#ifndef __FUNCTION_NAME__
    #ifdef WIN32   //WINDOWS
        #define __FUNCTION_NAME__   __FUNCTION__  
    #else          //*NIX
        #define __FUNCTION_NAME__   __func__ 
    #endif
#endif

#define DARK_ASSERT_IMPL(cond) dark::detail::assert_impl(cond, __FILE__, __LINE__, __FUNCTION_NAME__, #cond)
#define DARK_ASSERT_MSG_IMPL(cond, msg) dark::detail::assert_impl(cond, __FILE__, __LINE__, __FUNCTION_NAME__, #cond, msg)
#define DARK_GET_ASSERT_IMPL(_1, _2, NAME, ...) NAME
#define dark_assert(...) DARK_GET_ASSERT_IMPL(__VA_ARGS__, DARK_ASSERT_MSG_IMPL, DARK_ASSERT_IMPL)(__VA_ARGS__)

#endif // __DARK_COMMON_ASSERT_HPP__