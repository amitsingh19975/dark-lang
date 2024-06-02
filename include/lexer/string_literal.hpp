#ifndef __DARK_LEXER_STRING_LITERAL_HPP__
#define __DARK_LEXER_STRING_LITERAL_HPP__

#include "adt/buffer.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include <array>
#include <functional>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
namespace dark::lexer {
    using LexerDiagnosticEmitter = DiagnosticEmitter<char const*>;

    struct StringLiteral {
        static auto lex(llvm::StringRef input) -> std::optional<StringLiteral>;

        auto compute_value(
            llvm::BumpPtrAllocator& allocator,
            LexerDiagnosticEmitter& emitter
        ) const -> llvm::StringRef;

        [[nodiscard]] constexpr auto get_source() const noexcept -> llvm::StringRef { return m_source; }
        [[nodiscard]] constexpr auto get_content() const noexcept -> llvm::StringRef { return m_content; }
        [[nodiscard]] constexpr auto is_format_string() const noexcept -> bool { return m_is_format_string; }
        [[nodiscard]] constexpr auto needs_validation() const noexcept -> bool { return m_needs_validation; }
        [[nodiscard]] constexpr auto get_hash_level() const noexcept -> int { return m_hash_level; }
        [[nodiscard]] constexpr auto is_multi_line() const noexcept -> bool { return m_multi_line_kind == MultiLine; }
        [[nodiscard]] constexpr auto is_reflection() const noexcept -> bool { return (m_multi_line_kind == Reflection || m_multi_line_kind == ReflectionDoubleQuotes); }
        [[nodiscard]] constexpr auto is_terminated() const noexcept -> bool { return m_is_terminated; }
        [[nodiscard]] constexpr auto get_ident_error_pos() const noexcept -> int { return m_ident_error_pos; }
        [[nodiscard]] constexpr auto get_codeblock_prefix() const noexcept -> llvm::StringRef {
            if (!is_reflection()) {
                return {};
            }

            auto content = get_source().substr(3zu + static_cast<std::size_t>(m_hash_level < 0 ? 0 : m_hash_level));
            auto prefix_end = content.find_first_of("\n");
            return content.substr(0, prefix_end).trim();
        }

        static auto decode_unicode_escape(
            LexerDiagnosticEmitter& emitter,
            llvm::StringRef& input,
            char32_t& result,
            bool should_check_prefix = true
        ) -> bool;

        static auto decode_unicode_escape(
            LexerDiagnosticEmitter& emitter,
            llvm::StringRef& input,
            Buffer<char>& buffer,
            bool should_check_prefix = true
        ) -> bool;
        
    private:

        template <typename Fn>
            requires (std::is_invocable_r_v<bool, Fn, LexerDiagnosticEmitter&, llvm::StringRef>)
        static auto decode_unicode_escape_helper(
            LexerDiagnosticEmitter& emitter,
            llvm::StringRef& input,
            bool should_check_prefix,
            Fn&& fn
        ) -> bool {
            if (should_check_prefix && input.starts_with("\\u")) {
                input = input.drop_front(2);
            }

            auto remaining = input;

            if (remaining.consume_front("{")) {
                auto temp_digits = remaining.take_while([](auto c) { return c != '}'; });
                auto digits = temp_digits.trim();
                remaining = remaining.drop_front(temp_digits.size());
                if (!remaining.consume_front("}")) {
                    DARK_DIAGNOSTIC(UnicodeEscapeMissingClosingBrace, Error, 
                        "Unicode escape sequence is missing closing brace."
                    );
                    emitter.build(input.begin(), UnicodeEscapeMissingClosingBrace)
                        .add_error_suggestion_borrowed("Try adding a closing brace `}`")
                        .emit();
                    return false;
                }

                if (digits.empty()) {
                    DARK_DIAGNOSTIC(UnicodeEscapeMissingBracedDigits, Error, 
                        "Unicode escape sequence is missing digits."
                    );
                    emitter.emit(input.begin(), UnicodeEscapeMissingBracedDigits);
                    return false;
                }
                
                if (std::invoke(std::forward<Fn>(fn), emitter, digits)) {
                    input = remaining;
                    return true;
                }
            } else {
                DARK_DIAGNOSTIC(UnicodeEscapeMissingOpeningBrace, Error, 
                    "Unicode escape sequence is missing opening brace."
                );

                emitter.build(input.begin(), UnicodeEscapeMissingOpeningBrace)
                    .add_error_suggestion_borrowed("Try adding an opening brace `{`")
                    .emit();
            }

            return false;
        }

        enum MultiLineKind: std::uint8_t {
            NotMultiLine,
            MultiLine,
            MaybeMultiLine,
            Reflection,
            ReflectionDoubleQuotes
        };

        struct Introducer;

        explicit StringLiteral(
            llvm::StringRef source,
            llvm::StringRef content,
            bool is_format_string,
            bool needs_validation,
            int hash_level,
            MultiLineKind multi_line_kind,
            bool is_terminated,
            int ident_error_pos
        ) noexcept
            : m_source(source)
            , m_content(content)
            , m_multi_line_kind(multi_line_kind)
            , m_hash_level(hash_level)
            , m_is_format_string(is_format_string)
            , m_needs_validation(needs_validation)
            , m_is_terminated(is_terminated)
            , m_ident_error_pos(ident_error_pos)
        {}

        static auto lex_reflection(
            llvm::StringRef source,
            std::size_t cursor,
            std::size_t prefix_len,
            llvm::StringRef terminator,
            std::size_t hash_level,
            bool has_introducer
        ) -> std::optional<StringLiteral>;

    private:
        llvm::StringRef m_source;
        llvm::StringRef m_content; // """[content]"""
        MultiLineKind   m_multi_line_kind{NotMultiLine};
        int             m_hash_level{0};
        bool            m_is_format_string{false};
        bool            m_needs_validation{false};
        bool            m_is_terminated{false};
        int             m_ident_error_pos{-1};
    };
} // namespace dark::lexer

#endif // __DARK_LEXER_STRING_LITERAL_HPP__
