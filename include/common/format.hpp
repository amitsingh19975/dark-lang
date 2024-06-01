#ifndef __DARK_FORMAT_HPP__
#define __DARK_FORMAT_HPP__

#include <cstddef>
#include <cstdint>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <type_traits>
#include <utility>
#include <sstream>
#include <variant>
#include <format>
#include "common/assert.hpp"
#include "common/cow.hpp"

namespace dark {
    using format_args_t = std::variant<
        std::string_view,
        CowString,
        llvm::StringLiteral,
        std::string,
        std::int8_t,
        std::int16_t,
        std::int32_t,
        std::int64_t,
        std::uint8_t,
        std::uint16_t,
        std::uint32_t,
        std::uint64_t,
        std::size_t,
        std::ptrdiff_t,
        double,
        long double,
        float
    >;
} // namespace dark

namespace std {
    template<>
    struct formatter<::dark::format_args_t, char> {
        template<typename parse_context_t>
        constexpr auto parse(parse_context_t& ctx) -> typename parse_context_t::iterator {
            for(auto it = ctx.begin(); it != ctx.end(); ++it)
            {
                m_format += *it;
                if (*it == '}')
                {
                    return it;
                }
            }
            m_format += '}';
            return ctx.end();
        }

        template<typename fmt_context_t>
        auto format(::dark::format_args_t const& variant, fmt_context_t& ctx) const -> typename fmt_context_t::iterator {
            return std::visit( 
                [&ctx, this]<typename T>(T const& v) -> fmt_context_t::iterator {
                    if constexpr (std::is_same_v<T, dark::CowString>) {
                        return std::vformat_to(ctx.out(), m_format, std::make_format_args(v.borrow())); 
                    } else {
                        return std::vformat_to(ctx.out(), m_format, std::make_format_args(v)); 
                    }
                }, 
                variant
            );
        }
        std::string m_format{"{:"};

    };
}


namespace dark {
    namespace detail {
        template <typename T>
        concept ImplementsToCowString = requires(T const& p) {
            { p.to_cow_string() } -> std::same_as<CowString>;
        };
        
        template <typename T>
        concept ImplementsGlobalToCowString = requires(T const& p) {
            { to_cow_string(p) } -> std::same_as<CowString>;
        };

        template <typename T>
        concept ImplementsLLVMOStream = requires(T const& p) {
            { std::declval<llvm::raw_ostream&>() <<  p } -> std::same_as<llvm::raw_ostream&>;
        };

        template <typename T>
        concept ImplementsOStream = requires(T const& p) {
            { std::declval<std::ostream&>() <<  p } -> std::same_as<std::ostream&>;
        };

        template <typename T>
        concept ImplementsPrint = requires(T const& p) {
            { p.print(std::declval<llvm::raw_ostream&>()) } -> std::same_as<void>;
        };

        template <typename T>
        struct is_constructable_to_format_args : std::bool_constant<
            std::is_constructible_v<format_args_t, T> ||
            std::is_constructible_v<format_args_t, std::decay_t<std::remove_cvref_t<T>>>
        > {};

        template <>
        struct is_constructable_to_format_args<std::string_view> : std::true_type {};
        
        template <>
        struct is_constructable_to_format_args<llvm::StringLiteral> : std::true_type {};

        template <typename T>
            requires (
                std::is_constructible_v<std::string_view, T> 
                || ImplementsToCowString<T> 
                || ImplementsLLVMOStream<T> 
                || ImplementsOStream<T>
                || ImplementsPrint<T>
                || ImplementsGlobalToCowString<T>
            )
        struct is_constructable_to_format_args<T>: std::true_type {};
    }

    template <typename T>
        requires detail::is_constructable_to_format_args<T>::value
    auto make_format_arg(T&& arg) -> format_args_t {
        using Arg = std::decay_t<std::remove_cvref_t<T>>;
        constexpr auto is_string_view = std::is_constructible_v<std::string_view, Arg> || std::is_convertible_v<Arg, std::string_view>;
        constexpr auto is_string = std::is_constructible_v<std::string, Arg> || std::is_convertible_v<Arg, std::string>;
        if constexpr (detail::ImplementsGlobalToCowString<Arg>) {
            return format_args_t(to_cow_string(std::forward<T>(arg)));
        } else if constexpr (detail::ImplementsToCowString<Arg>) {
            return format_args_t(arg.to_cow_string());
        } else if constexpr (std::is_same_v<Arg, llvm::StringLiteral>) {
            return format_args_t(std::forward<T>(arg));
        } else if constexpr (!is_string_view && is_string) {
            return format_args_t(std::string(std::forward<T>(arg)));
        } else if constexpr (is_string_view) {
            return format_args_t(std::string_view(arg));
        } else if constexpr (std::constructible_from<format_args_t, T>) {
            return format_args_t(std::forward<T>(arg));
        } else if constexpr (detail::ImplementsPrint<T>) {
            std::string str;
            auto stream = llvm::raw_string_ostream(str);
            arg.print(stream);
            return format_args_t(std::move(str));
        } else if constexpr (detail::ImplementsLLVMOStream<Arg>) {
            std::string str;
            llvm::raw_string_ostream stream(str);
            stream << arg;
            return format_args_t(std::move(str));
        } else if constexpr (detail::ImplementsOStream<Arg>) {
            std::string str;
            std::ostringstream stream(str);
            stream << arg;
            return format_args_t(std::move(str));
        } else {
            static_assert(detail::always_false_v<T>, "Unsupported argument type.");
        }
    }

    struct Formatter {
        static constexpr auto max_args = 20;

        Formatter(llvm::StringLiteral format)
            : m_format(format)
        {}

        template <typename... Args>
            requires ((sizeof...(Args) < max_args) && (... && detail::is_constructable_to_format_args<Args>::value))
        Formatter(llvm::StringLiteral format, Args&&... args)
            : m_format(format)
            , m_arguments { make_format_arg(std::forward<Args>(args))... }
        {}

        Formatter(llvm::StringLiteral format, llvm::SmallVector<format_args_t> args)
            : m_format(format)
            , m_arguments(std::move(args))
        {
            dark_assert(m_arguments.size() <= max_args, std::format("Too many arguments; maximum is {}.", max_args));
        }

        auto format() const -> std::string {
            switch (m_arguments.size()) {
                case 0: return std::string(m_format);
                case 1: return format_helper(m_arguments[0]);
                case 2: return format_helper(m_arguments[0], m_arguments[1]);
                case 3: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2]);
                case 4: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3]);
                case 5: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4]);
                case 6: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5]);
                case 7: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6]);
                case 8: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7]);
                case 9: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8]);
                case 10: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9]);
                case 11: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10]);
                case 12: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11]);
                case 13: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12]);
                case 14: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13]);
                case 15: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14]);
                case 16: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14], m_arguments[15]);
                case 17: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14], m_arguments[15], m_arguments[16]);
                case 18: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14], m_arguments[15], m_arguments[16], m_arguments[17]);
                case 19: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14], m_arguments[15], m_arguments[16], m_arguments[17], m_arguments[18]);
                case 20: return format_helper(m_arguments[0], m_arguments[1], m_arguments[2], m_arguments[3], m_arguments[4], m_arguments[5], m_arguments[6], m_arguments[7], m_arguments[8], m_arguments[9], m_arguments[10], m_arguments[11], m_arguments[12], m_arguments[13], m_arguments[14], m_arguments[15], m_arguments[16], m_arguments[17], m_arguments[18], m_arguments[19]);
            }
            std::unreachable();
        }

    private:
        template <typename... Args>
        auto format_helper(Args const&... args) const -> std::string {
            return std::vformat(m_format, std::make_format_args(args...));
        }

    private:
        llvm::StringLiteral m_format;
        llvm::SmallVector<format_args_t> m_arguments;
    };
}
#endif // __DARK_FORMAT_HPP__