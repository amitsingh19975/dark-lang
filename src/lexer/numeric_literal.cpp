#include "lexer/numeric_literal.hpp"
#include "common/assert.hpp"
#include "common/big_num.hpp"
#include "common/bit_array.hpp"
#include "common/cow.hpp"
#include "common/format.hpp"
#include "common/span.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include "lexer/character_set.hpp"
#include <algorithm>
#include <iterator>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <optional>

namespace dark::lexer {

    auto NumericLiteral::lex(llvm::StringRef input) -> std::optional<NumericLiteral> {
        auto res = NumericLiteral{};
        if (input.empty() || !char_set::is_digit(input.front())) return std::nullopt;

        bool seen_plus_minus = false;
        bool seen_radix_point = false;
        bool seen_potential_exponent = false;

        unsigned i{1};
        auto const n = input.size();
        for (; i < n; ++i) {
            auto const c = input[i];
            if (char_set::is_alnum(c) || '_') {
                if (char_set::is_lower(c) && seen_radix_point && !seen_potential_exponent) {
                    // 123.e2
                    //     ^
                    res.m_exponent = i;
                    seen_potential_exponent = true;
                }
                continue;
            }

            if (c == '.' && (i + 1 != n) && char_set::is_alnum(input[i + 1])) {
                if (seen_radix_point) break;
                seen_radix_point = true;
                res.m_radix_point = i;
                continue;
            }

            if ((c == '-' || c == '+') && seen_potential_exponent &&
                res.m_exponent == i - 1 && i + 1 != n && char_set::is_alnum(input[i + 1])
            ) {
                dark_assert(!seen_plus_minus, "should only consume one + or -");
                seen_plus_minus = true;
                continue;
            }

            break;
        }

        res.m_source = input.substr(0, i);
        if (!res.m_radix_point) res.m_radix_point = i;
        if (!res.m_exponent) res.m_exponent = i;

        return res;
    }

    inline static auto parse_int(
        llvm::StringRef source,
        NumericLiteral::Radix radix,
        bool needs_cleaning
    ) -> SignedBigNum {

        llvm::SmallString<32> cleaned_source;
        if (needs_cleaning) {
            cleaned_source.reserve(source.size());
            std::remove_copy_if(source.begin(), source.end(), std::back_inserter(cleaned_source), [](char c) {
                return c == '_' || c == '.';
            });
            source = cleaned_source;
        }

        return SignedBigNum(source, static_cast<unsigned>(radix)); 
    }

    struct NumericLiteral::Parser {
        constexpr Parser(DiagnosticEmitter<char const*>& emitter, NumericLiteral literal) noexcept
            : m_emitter(emitter)
            , m_literal(literal)
            , m_int_part(m_literal.get_source().substr(0, m_literal.m_radix_point))
            , m_frac_part(m_literal.get_source().substr(m_literal.m_radix_point + 1, m_literal.m_exponent - m_literal.m_radix_point - 1))
            , m_exp_part(m_literal.get_source().substr(m_literal.m_exponent + 1))
        {
            if (m_int_part.consume_front("0x")) {
                m_radix = Radix::Hexadecimal;
            } else if (m_int_part.consume_front("0b")) {
                m_radix = Radix::Binary;
            } else if (m_int_part.consume_front("0o")) {
                m_radix = Radix::Octal;
            } else {
                m_radix = Radix::Decimal;
            }

            if (!m_exp_part.consume_front("+")) {
                m_exponent_is_negative = m_exp_part.consume_front("-");
            }
        }
        constexpr auto is_integer() const noexcept -> bool { return m_literal.m_radix_point == m_literal.get_source().size(); }
        constexpr auto is_real() const noexcept -> bool { return !is_integer(); }

        auto check() noexcept -> bool {
            return check_leading_zeros() && check_integer_part() && check_fractional_part() && check_exponent_part();
        }

        // Returns the radix of the numeric literal 2, 8, 10, or 16
        constexpr auto get_radix() const noexcept -> Radix { return m_radix; }

        auto get_mantissa() const -> SignedBigNum {
            auto end = is_integer() ? m_int_part.end() : m_frac_part.end();
            auto digits = llvm::StringRef(m_int_part.begin(), static_cast<size_t>(end - m_int_part.begin()));
            return parse_int(digits, m_radix, m_mantissa_needs_cleaning);
        }
        auto get_exponent() -> SignedBigNum {
            SignedBigNum exponent;
            if (!m_exp_part.empty()) {
                exponent = parse_int(m_exp_part, Radix::Decimal, m_exponent_needs_cleaning);
            }

            if (m_exponent_is_negative) {
                exponent = -exponent;
            }

            std::size_t excess_exponent = m_frac_part.size();
            if (m_radix == Radix::Hexadecimal) {
                excess_exponent *= 4;
            }

            exponent -= excess_exponent;

            if (m_exponent_is_negative && !exponent.is_negative()) {
                exponent = -exponent;
            }

            return exponent;
        }
    
    private:
        struct CheckDigitSequenceResult {
            bool ok{false};
            bool has_digit_separators{false};
        };

        constexpr auto check_digit_sequence(
            llvm::StringRef source, 
            Radix radix, 
            bool allow_digit_separators = true
        ) const noexcept -> CheckDigitSequenceResult {
            auto const& valid_digits = (radix == Radix::Binary) ? char_set::detail::binary_digits
                : (radix == Radix::Octal) ? char_set::detail::octal_digits
                : (radix == Radix::Decimal) ? char_set::detail::decimal_digits
                : char_set::detail::hexadecimal_digits;

            unsigned num_digit_separators = 0;
            auto n = source.size();
            for (auto i = 0u; i < n; ++i) {
                char const c = source[i];
                if (valid_digits[static_cast<unsigned>(c)]) {
                    continue;
                }

                if (c == '_') {
                    if (!allow_digit_separators || i == 0 || i + 1 == n || source[i - 1] == '_') {
                        DARK_DIAGNOSTIC(InvalidDigitSeparator, Error, "Misplaced digit separator in numeric literal.");
                        m_emitter.build(source.begin() + 1, InvalidDigitSeparator)
                            .add_info_suggestion_borrowed("Try removing the misplaced digit separator.", Span(i, i + 1).to_relative())
                            .emit();
                    }
                    ++num_digit_separators;
                    continue;
                }

                DARK_DIAGNOSTIC(InvalidDigit, Error, "Invalid digit '{0}' in {1} numeric literal", char, ::dark::lexer::NumericLiteral::Radix);
                m_emitter.build(source.begin() + i, InvalidDigit, c, radix)
                    .add_info_suggestion_borrowed("Try removing the invalid digit.", Span(i, i + 1).to_relative())
                    .add_info_suggestion_borrowed("Try using a valid digit.", Span(i, i + 1).to_relative())
                    .emit();
                return {
                    .ok = false,
                };
            }

            if (num_digit_separators == source.size()) {
                DARK_DIAGNOSTIC(EmptyDigitSequence, Error, "Empty digit sequence in numeric literal.");
                m_emitter.emit(source.begin(), EmptyDigitSequence);
                return {
                    .ok = false,
                };
            }

            if (num_digit_separators != 0) {
                check_digit_separator_position(source, radix, num_digit_separators);
            }

            return {
                .ok = true,
                .has_digit_separators = num_digit_separators != 0,
            };
        }

        auto check_digit_separator_position(llvm::StringRef source, Radix radix, unsigned num_digit_separators) const noexcept -> void {
            dark_assert(std::count(source.begin(), source.end(), '_') == num_digit_separators, "num_digit_separators is incorrect");

            if (radix == Radix::Binary) {
                return;
            }

            auto stride = 4;
            switch (radix) {
                case Radix::Decimal: stride = 4; break;
                case Radix::Octal: stride = 3; break;
                case Radix::Hexadecimal: stride = 5; break;
                default: std::unreachable();
            }

            auto irregular_diagnostics = [this, source, radix, stride]() {
                DARK_DIAGNOSTIC(IrregularDigitSeparators, Error, 
                    "Digit separators in {} number should appear every {} characters "
                    "from the right.",
                    Radix, int
                );

                return m_emitter.build(
                    source.begin(),
                    IrregularDigitSeparators,
                    radix,
                    stride - 1
                );
            };

            auto remaining_separator = num_digit_separators;
            auto pos = source.end();
            while (std::distance(source.begin(), pos) >= stride) {
                pos -= stride;
                if (*pos != '_') {
                    auto distance = static_cast<unsigned>(std::distance(source.begin(), pos));
                    irregular_diagnostics()
                        .add_error_suggestion_borrowed("Misplaced digit separator.", Span(distance, distance + 1).to_relative())
                        .emit();
                    return;
                }
                
                --remaining_separator;
            }

            if (remaining_separator != 0) {
                irregular_diagnostics()
                    .add_child_info_context_borrow("Remove the misplaced digit separator.")
                    .emit();
            }
        }
        
        auto check_leading_zeros() const noexcept -> bool {
            if (m_radix == Radix::Decimal && m_int_part.starts_with("0") && m_int_part != "0") {
                DARK_DIAGNOSTIC(UnknownBaseSpecifier, Error, "Unknown base specifier in numeric literal.");
                m_emitter.build(m_int_part.begin(), UnknownBaseSpecifier)
                    .add_info_suggestion_borrowed("Try removing the unknown base specifier.", Span(0, 1).to_relative())
                    .emit();
                return false;
            }
            return true;
        }

        auto check_integer_part() noexcept -> bool {
            auto result = check_digit_sequence(m_int_part, m_radix);
            m_mantissa_needs_cleaning = m_mantissa_needs_cleaning || result.has_digit_separators;
            return result.ok;
        }
        auto check_fractional_part() noexcept -> bool {
            if (is_integer()) return true;

            if (m_radix == Radix::Binary) {
                DARK_DIAGNOSTIC(BinaryRealLiteral, Error, "Binary real number literals are not supported.");
                auto const span_end = static_cast<unsigned>(m_frac_part.size());
                m_emitter.build(m_literal.m_source.begin() + m_literal.m_radix_point, BinaryRealLiteral)
                    .add_error_suggestion_borrowed("Try removing the binary fractional part.", Span(0, span_end).to_relative())
                    .emit();
                return false;
            }

            if (m_radix == Radix::Octal) {
                DARK_DIAGNOSTIC(OctalRealLiteral, Error, "Octal real number literals are not supported.");
                auto const span_end = static_cast<unsigned>(m_frac_part.size());
                m_emitter.build(m_literal.m_source.begin() + m_literal.m_radix_point, OctalRealLiteral)
                    .add_error_suggestion_borrowed("Try removing the octal fractional part.", Span(0, span_end).to_relative())
                    .emit();
                return false;
            }

            m_mantissa_needs_cleaning = false;
            return check_digit_sequence(m_frac_part, m_radix, /*allow_digit_separators=*/false).ok;
        }
        
        auto check_exponent_part() noexcept -> bool {
            if (m_literal.m_exponent == static_cast<unsigned>(m_literal.get_source().size())) return true;
            char expected_exponent_char = m_radix == Radix::Decimal ? 'e' : 'p';
            if (m_literal.get_source()[m_literal.m_exponent] != expected_exponent_char) {
                DARK_DIAGNOSTIC(WrongRealLiteralExponent, Error, "Expected '{0}' to introduce exponent.", char);
                auto suggestion = Formatter("Change '{0}' to '{1}'", m_literal.get_source()[m_literal.m_exponent], expected_exponent_char);
                m_emitter.build(
                    m_literal.get_source().begin() + m_literal.m_exponent,
                    WrongRealLiteralExponent,
                    expected_exponent_char
                ).add_error_suggestion(make_owned(suggestion.format()), Span(m_literal.m_exponent, m_literal.m_exponent + 1).to_relative())
                .emit();
            }

            auto exponent_result = check_digit_sequence(m_exp_part, Radix::Decimal);
            m_exponent_needs_cleaning = m_exponent_needs_cleaning || exponent_result.has_digit_separators;
            return exponent_result.ok;
        }

    private:
        DiagnosticEmitter<char const*>& m_emitter;
        NumericLiteral m_literal;
        Radix m_radix;
        llvm::StringRef m_int_part;
        llvm::StringRef m_frac_part;
        llvm::StringRef m_exp_part;
        bool m_mantissa_needs_cleaning{false};
        bool m_exponent_needs_cleaning{false};
        bool m_exponent_is_negative{false};
    };

    auto NumericLiteral::compute_value(DiagnosticEmitter<char const*> emitter) const -> value_type {
        auto parser = Parser(emitter, *this);
        if (!parser.check()) return UnrecoverableError{};

        if (parser.is_integer()) {
            return IntValue{ parser.get_mantissa() };
        }

        return RealValue{
            .radix = parser.get_radix(),
            .mantissa = parser.get_mantissa(),
            .exponent = parser.get_exponent(),
        };
    }

} // namespace dark::lexer