#ifndef __DARK_LEXER_NUMERIC_LITERAL_HPP__
#define __DARK_LEXER_NUMERIC_LITERAL_HPP__

#include "common/big_num.hpp"
#include "common/cow.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/APInt.h>
#include <optional>
#include <variant>

namespace dark::lexer {

    struct NumericLiteral {
        enum class Radix: std::int8_t {
            Binary = 2,
            Octal = 8,
            Decimal = 10,
            Hexadecimal = 16
        };

        struct IntValue {
            SignedBigNum value;
        };

        struct RealValue {
            Radix radix{ Radix::Decimal };
            SignedBigNum mantissa;
            SignedBigNum exponent;
        };

        struct UnrecoverableError {};
        
        using value_type = std::variant<IntValue, RealValue, UnrecoverableError>;

        static auto lex(llvm::StringRef input) -> std::optional<NumericLiteral>;

        auto compute_value(DiagnosticEmitter<char const*>) const -> value_type;

        constexpr auto get_source() const -> llvm::StringRef { return m_source; }

    private:
        struct Parser;
    
    private:
        llvm::StringRef m_source;
        unsigned m_radix_point{};
        unsigned m_exponent{};
    };

    [[nodiscard]] inline auto to_cow_string(NumericLiteral::Radix radix) -> CowString {
        switch (radix) {
        case NumericLiteral::Radix::Binary: return make_borrowed("binary");
        case NumericLiteral::Radix::Decimal: return make_borrowed("decimal");
        case NumericLiteral::Radix::Hexadecimal: return make_borrowed("hexadecimal");
        case NumericLiteral::Radix::Octal: return make_borrowed("octal");
        }
    }

}

#endif // __DARK_LEXER_NUMERIC_LITERAL_HPP__