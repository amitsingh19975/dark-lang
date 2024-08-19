#include "lexer/string_literal.hpp"
#include "adt/buffer.hpp"
#include "common/assert.hpp"
#include "common/bit_array.hpp"
#include "common/cow.hpp"
#include "common/span.hpp"
#include "common/static_string.hpp"
#include "common/utf8.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "lexer/character_set.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <string_view>

namespace dark::lexer {

    static constexpr auto string_indicator = R"(")";
    static constexpr auto reflection_indicator_double_quotes = R"(""")";
    static constexpr auto reflection_indicator = R"(''')";

    struct StringLiteral::Introducer {
        MultiLineKind kind;
        llvm::StringRef terminator;
        std::size_t prefix_size;

        static constexpr auto lex(llvm::StringRef source) noexcept -> std::optional<Introducer> {
            MultiLineKind kind = NotMultiLine;

            llvm::StringRef indicator;

            if (source.empty()) return std::nullopt;

            if (source.starts_with(reflection_indicator)) {
                kind = Reflection;
                indicator = reflection_indicator;
            } else if (source.starts_with(reflection_indicator_double_quotes)) {
                kind = ReflectionDoubleQuotes;
                indicator = reflection_indicator_double_quotes;
            } else if (source.starts_with(string_indicator)) {
                kind = MaybeMultiLine;
                indicator = string_indicator;
            }


            if (kind == MaybeMultiLine) {
                auto prefix_end = source.find_first_of("\n\"", indicator.size());
                if (prefix_end != llvm::StringRef::npos) {
                    kind = (source[prefix_end] == '\n' ? MultiLine : NotMultiLine);
                    return Introducer{
                        .kind = kind,
                        .terminator = indicator,
                        .prefix_size = indicator.size()
                    };
                }
            } else if (kind == Reflection || kind == ReflectionDoubleQuotes) {
                auto prefix_end = source.find_first_of("\n'\"", indicator.size());
                if (prefix_end != llvm::StringRef::npos) {
                    if (source[prefix_end] == '\n') {
                        return Introducer{
                            .kind = kind,
                            .terminator = indicator,
                            .prefix_size = prefix_end + 1
                        };
                    } else {
                        return Introducer{
                            .kind = kind,
                            .terminator = indicator,
                            .prefix_size = indicator.size()
                        };
                    }
                }
            }

            if (!source.empty() && source[0] == '"') {
                return Introducer{
                    .kind = NotMultiLine,
                    .terminator = string_indicator,
                    .prefix_size = 1
                };
            }

            return std::nullopt;
        }
    };

    template <StaticString S>
    inline constexpr auto get_char_bit_array() noexcept -> BitArray<std::numeric_limits<char>::max() + 1> {
        auto bit_array = BitArray<std::numeric_limits<char>::max() + 1>{};
        for (auto i = 0zu; i < S.size(); ++i) {
            bit_array.set(static_cast<std::size_t>(S[i]), true);
        }
        return bit_array;
    }

    auto StringLiteral::lex_reflection(
        llvm::StringRef source,
        std::size_t cursor,
        std::size_t prefix_len,
        llvm::StringRef terminator,
        std::size_t hash_level,
        bool has_introducer
    ) -> std::optional<StringLiteral> {
        bool content_needs_validation = false;
        bool is_format_string = false;

        static constexpr auto interesting_chars = get_char_bit_array<"'\"">();
        for (; cursor < source.size(); ++cursor) {
            auto const ch = source[cursor];
            if (interesting_chars[static_cast<std::size_t>(ch)] && source.substr(cursor).starts_with(terminator)) {
                auto content = source.substr(prefix_len, cursor - prefix_len);
                auto text = source.substr(0, cursor + terminator.size());
                return StringLiteral(
                    /* source = */ text,
                    /* content = */ content,
                    /* is_format_string = */ is_format_string,
                    /* needs_validation = */ content_needs_validation,
                    /* hash_level = */  static_cast<int>(hash_level),
                    /* multi_line_kind = */ Reflection,
                    /* is_terminate = */ true,
                    /* ident_error_pos = */ -1
                );
            }

            if (!has_introducer && ch == '{') {
                if (cursor + 1 < source.size() && source[cursor + 1] != '{') {
                    is_format_string = true;
                    content_needs_validation = true;
                } else {
                    ++cursor;
                }
            }
        }

        return StringLiteral(
            /* source = */ source,
            /* content = */ source.drop_front(prefix_len),
            /* is_format_string = */ is_format_string,
            /* needs_validation = */ content_needs_validation,
            /* hash_level = */  static_cast<int>(hash_level),
            /* multi_line_kind = */ Reflection,
            /* is_terminate = */ false,
            /* ident_error_pos = */ -1
        );
    }

    auto StringLiteral::lex(llvm::StringRef source) -> std::optional<StringLiteral> {
        auto cursor = std::size_t{0};
        auto source_text_size = source.size();

        cursor = source.find_first_not_of("#");

        auto hash_level = cursor;
        auto introducer = Introducer::lex(source.substr(cursor));
        if (!introducer) return std::nullopt;

        cursor += introducer->prefix_size;
        auto prefix_len = cursor;

        llvm::SmallString<16> terminator(introducer->terminator);
        llvm::SmallString<16> escape("\\");
        escape.resize(1 + static_cast<std::size_t>(hash_level), '#');

        terminator.resize(terminator.size() + static_cast<std::size_t>(hash_level), '#');

        bool is_reflection = (introducer->kind == Reflection || introducer->kind == ReflectionDoubleQuotes);
        if (is_reflection) {
            return lex_reflection(
                source,
                cursor,
                prefix_len,
                terminator,
                hash_level,
                introducer->prefix_size > 3
            );
        }

        bool content_needs_validation = false;
        bool found_char = false;
        bool is_format_string = false;

        static constexpr auto interesting_chars = get_char_bit_array<"\\\"\n \t{">();
        auto const set_format_string = [](
            std::size_t& cursor,
            std::size_t source_text_size,
            llvm::StringRef source,
            bool& is_format_string,
            bool& content_needs_validation
         ) -> void {
            auto open_brace_count = 0zu;
            while ((cursor < source_text_size) && source[cursor] == '{') {
                ++open_brace_count;
                ++cursor;
            }

            if ((open_brace_count & 1) == 1) {
                is_format_string = true;
                content_needs_validation = true;
            }
        };

        for (; cursor < source_text_size; ++cursor) {
            auto const ch = source[cursor];
            found_char = found_char && !interesting_chars[static_cast<std::size_t>(ch)];

            if (found_char) continue;

            switch (ch) {
                case '"': {
                    if (source.substr(cursor).starts_with(terminator)) {
                        auto content = source.substr(prefix_len, cursor - prefix_len);
                        auto text = source.substr(0, cursor + terminator.size());
                        return StringLiteral(
                            /* source = */ text,
                            /* content = */ content,
                            /* is_format_string = */ is_format_string,
                            /* needs_validation = */ content_needs_validation,
                            /* hash_level = */  static_cast<int>(hash_level),
                            /* multi_line_kind = */ introducer->kind,
                            /* is_terminate = */ true,
                            /* ident_error_pos = */ -1
                        );
                    }
                    break;
                }
                case '{': {
                    if (escape.size() == 1) set_format_string(
                        cursor,
                        source_text_size,
                        source,
                        is_format_string,
                        content_needs_validation
                    );
                }
                break;
                case ' ':
                    break;
                case '\n':
                    break;
                case '\\': {
                    if (escape.size() == 1 || source.substr(cursor + 1).starts_with(escape.substr(1))) {
                        cursor += escape.size();
                        content_needs_validation = true;
                        if (cursor < source_text_size) {
                            if (source[cursor] == 'u') ++cursor;
                            else if ((source[cursor] == '{') && escape.size() > 1) set_format_string(
                                cursor,
                                source_text_size,
                                source,
                                is_format_string,
                                content_needs_validation
                            );
                        }
                    }
                }
                [[fallthrough]];
                default:
                    found_char = true;
                    break;
            }
        }

        return StringLiteral(
            /* source = */ source,
            /* content = */ source.drop_front(prefix_len),
            /* is_format_string = */ is_format_string,
            /* needs_validation = */ content_needs_validation,
            /* hash_level = */  static_cast<int>(hash_level),
            /* multi_line_kind = */ introducer->kind,
            /* is_terminate = */ false,
            /* ident_error_pos = */ -1
        );
    }

    static inline auto get_and_check_code_point(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef digits
    ) -> std::optional<char32_t> {
        auto span = Span(0, static_cast<unsigned>(digits.size())).to_relative();
        if (digits.size() > 6) {
            DARK_DIAGNOSTIC(UnicodeEscapeDigitsTooLarge, Error, "Unicode escape sequence has too many digits.");
            emitter.build(digits.begin(), UnicodeEscapeDigitsTooLarge)
                .add_error_suggestion(std::format("Expected at most 6 digits, but got {} digits", digits.size()), span)
                .add_info_suggestion("Try reducing the number of digits in the unicode escape sequence", span)
                .emit();
            return {};
        }

        std::uint32_t code_point = 0;

        if (digits.getAsInteger(16, code_point)) {
            DARK_DIAGNOSTIC(UnicodeEscapeInvalidDigits, Error,
                    "Unicode escape sequence contains invalid hexadecimal digits."
            );
            emitter.build(digits.begin(), UnicodeEscapeInvalidDigits)
                .set_span_length(static_cast<unsigned>(digits.size()))
                .emit();
            return {};
        }

        if (digits.getAsInteger(16, code_point) || code_point > 0x10FFFF) {
            DARK_DIAGNOSTIC(UnicodeEscapeTooLarge, Error,
                "Invalid unicode escape sequence. Code point is too large."
            );
            emitter.build(digits.begin(), UnicodeEscapeTooLarge)
                .add_error_suggestion("Unicode code points must be in the range 0x0 to 0x10FFFF.", span)
                .emit();
            return {};
        }

        if (code_point >= 0xD800 && code_point < 0xE000) {
            DARK_DIAGNOSTIC(UnicodeEscapeSurrogate, Error,
                    "Invalid unicode escape sequence. Code point is a surrogate."
            );
            emitter.build(digits.begin(), UnicodeEscapeSurrogate)
                .add_error_suggestion("Unicode code points in the range 0xD800 to 0xDFFF are reserved for surrogates.", span)
                .emit();
            return {};
        }

        return static_cast<char32_t>(code_point);
    }

    static inline auto expand_unicode_escape_sequence(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef digits,
        Buffer<char>& buffer
    ) -> bool {

        auto code_point = get_and_check_code_point(emitter, digits);
        if (!code_point) return false;

        char temp_buffer[5] = {0};
        auto size = utf8::utf32_to_utf8(*code_point, temp_buffer);
        dark_assert(size > 0, "utf32_to_utf8 should never return 0 or fail");
        dark_assert(buffer.space_left() >= size, "Buffer should have enough space for the utf8 sequence");
        buffer.push_back(temp_buffer, size);
        return true;
    }

    auto StringLiteral::decode_unicode_escape(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef& input,
        Buffer<char>& buffer,
        bool should_check_prefix
    ) -> bool {
        return decode_unicode_escape_helper(emitter, input, should_check_prefix, [&buffer](LexerDiagnosticEmitter& emitter, llvm::StringRef digits) {
            return expand_unicode_escape_sequence(emitter, digits, buffer);
        });
    }

    auto StringLiteral::decode_unicode_escape(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef& input,
        char32_t& result,
        bool should_check_prefix
    ) -> bool {
        return decode_unicode_escape_helper(emitter, input, should_check_prefix, [&result](LexerDiagnosticEmitter& emitter, llvm::StringRef digits) {
            auto code_point = get_and_check_code_point(emitter, digits);
            if (!code_point) return false;
            result = *code_point;
            return true;
        });
    }

    static inline auto expand_and_consume_escape_sequence(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef& content,
        Buffer<char>& buffer
    ) -> void
    {
        dark_assert(!content.empty(), "should have escaped closing delimiter");
        auto ch = content.front();
        content = content.drop_front(1);

        switch (ch) {
            case 't':
                buffer.push_back('\t');
                return;
            case 'n':
                buffer.push_back('\n');
                return;
            case 'r':
                buffer.push_back('\r');
                return;
            case '"':
                buffer.push_back('"');
                return;
            case '\'':
                buffer.push_back('\'');
                return;
            case '\\':
                buffer.push_back('\\');
                return;
            case '0': {
                buffer.push_back('\0');
                if (!content.empty() && !char_set::is_oct_digit(content.front())) {
                    DARK_DIAGNOSTIC(DecimalEscapeSequence, Error,
                        "Invalid octal digit.");
                    emitter.build(content.begin(), DecimalEscapeSequence)
                        .add_error_suggestion(make_owned(std::format("Expected an octal digit, but got '{}'", content.front())))
                        .emit();
                    return;
                }
                buffer.push_back(content.front());
                return;
            }
            case 'x': {
                if (content.size() < 2) {
                    DARK_DIAGNOSTIC(HexadecimalEscapeMissingDigits, Error,
                        "Hexadecimal escape sequence is too short.");
                    emitter.build(content.begin() - 1, HexadecimalEscapeMissingDigits)
                        .add_error_suggestion(make_owned(std::format("Expected 2 hexadecimal digits after this, but got {} digits", content.size())))
                        .emit();
                    break;
                }
                bool is_valid[] = { char_set::is_hex_digit(content[0]), char_set::is_hex_digit(content[1]) };
                DARK_DIAGNOSTIC(HexadecimalEscapeNotValid, Error,
                    "Hexadecimal escape sequence contains invalid digit."
                );
                if (!is_valid[0]) {
                    emitter.build(content.begin(), HexadecimalEscapeNotValid)
                        .add_error_suggestion(make_owned(std::format("Expected a hexadecimal digit, but got '{}'", content[0])))
                        .emit();
                    break;
                }

                if (!is_valid[1]) {
                    emitter.build(content.begin() + 1, HexadecimalEscapeNotValid)
                        .add_error_suggestion(make_owned(std::format("Expected a hexadecimal digit, but got '{}'", content[1])))
                        .emit();
                    break;
                }

                char num = static_cast<char>(llvm::hexFromNibbles(content[0], content[1]));
                buffer.push_back(num);
                content = content.drop_front(2);
                return;
            }
            case 'u': {
                if (StringLiteral::decode_unicode_escape(emitter, content, buffer, false)) {
                    return;
                }
                break;
            }
            default: {
                DARK_DIAGNOSTIC(UnknownEscapeSequence, Error,
                    "Unknown escape sequence `{}`.", std::string_view
                );

                emitter.emit(content.begin() - 1, UnknownEscapeSequence, std::string_view{ content.begin() - 1, 1 });
                break;
            }
        }

        buffer.push_back(ch);
    }

    static inline auto expand_escape_sequence_and_remove_indent(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef content,
        llvm::StringRef indent,
        int hash_level,
        std::string_view terminator,
        bool is_reflection,
        Buffer<char>& buffer
    ) -> llvm::StringRef {
        llvm::SmallString<16> escape("\\");
        escape.resize(1 + static_cast<std::size_t>(hash_level), '#');

        while (true) {
            if (!content.consume_front(indent)) {
                auto line_start = content.begin();
                content = content.drop_while([](auto c) { return char_set::is_horizontal_space(c); });
                if (!content.starts_with("\n")) {
                    DARK_DIAGNOSTIC(MismatchedIndentInString, Error,
                        "Indentation does not match that of the closing `{}` in {} literal.",
                        std::string_view,
                        char const*
                    );
                    auto span_end = static_cast<unsigned>(content.begin() - line_start);
                    auto span = Span(0, span_end).to_relative();
                    emitter.build(line_start, MismatchedIndentInString, terminator, is_reflection ? "a codeblock" : "a multi-line string")
                        .add_error_suggestion(
                            make_owned(
                                std::format("Expected at least '{}' characters of indentation, but found '{}'", indent.size(), span.size())
                            ),
                            span
                        )
                        .emit();
                }
            }

            if (is_reflection) {
                auto end_pos_of_regular_text = content.find_first_of("\n");

                buffer.push_back(content, end_pos_of_regular_text);
                if (end_pos_of_regular_text == llvm::StringRef::npos) {
                    return llvm::StringRef{ buffer.data(), buffer.size() };
                }
                content = content.drop_front(end_pos_of_regular_text + 1);
                buffer.push_back('\n');
                continue;
            }

            auto last_buffer = buffer;

            while (true) {
                auto end_pos_of_regular_text = content.find_if([](auto c) {
                    return c == '\n' || c == '\\' || (char_set::is_horizontal_space(c) && c != ' ');
                });
                buffer.push_back(content, end_pos_of_regular_text);

                if (end_pos_of_regular_text == llvm::StringRef::npos) {
                    return llvm::StringRef{ buffer.data(), buffer.size() };
                }

                content = content.drop_front(end_pos_of_regular_text);

                // remove the whitespace between the newlines "...\n[     ]\n..."
                if (content.consume_front("\n")) {
                    while (buffer.size() > last_buffer.size()) {
                        auto back = buffer.back();
                        if (back == '\n' || !char_set::is_space(back)) {
                            break;
                        }
                        buffer.pop_back();
                    }
                    buffer.push_back('\n');
                    break;
                }

                if (char_set::is_horizontal_space(content.front())) {
                    dark_assert(content.front() != ' ', "should not have stopped at a plain space");
                    auto non_space_index = content.find_if([](auto c) { return !char_set::is_horizontal_space(c); });
                    if (non_space_index == llvm::StringRef::npos || content[non_space_index] != '\n') {
                        DARK_DIAGNOSTIC(InvalidHorizontalWhitespaceInString, Error,
                            "Whitespace other than plain space must be expressed with an "
                            "escape sequence in a string literal."
                        );
                        auto span = Span(0, static_cast<unsigned>(std::min(content.size(), non_space_index))).to_relative();
                        emitter.build(content.begin(), InvalidHorizontalWhitespaceInString)
                            .add_error_suggestion("Use an escape sequence to express the whitespace", span)
                            .emit();
                        buffer.push_back(content, non_space_index);
                    }
                    content = content.drop_front(non_space_index);
                    continue;
                }

                if (!content.consume_front(escape)) {
                    buffer.push_back(content.front());
                    content = content.drop_front(1);
                    continue;
                }

                if (content.consume_front("\n")) {
                    break;
                }

                expand_and_consume_escape_sequence(emitter, content, buffer);

                last_buffer = buffer;
            }
        }
    }

    static inline auto compute_indent_from_final_line(llvm::StringRef text) noexcept -> llvm::StringRef {
        auto end = static_cast<std::ptrdiff_t>(text.size());

        for (auto i = end - 1; i >= 0; --i) {
            auto ch = text[static_cast<std::size_t>(i)];
            if (ch == '\n') {
                auto start = i + 1;
                return text.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
            }
            if (!char_set::is_space(ch)) {
                end = i;
            }
        }

        llvm_unreachable("Given text does not contain a newline character");
    }

    static inline auto check_indent(
        LexerDiagnosticEmitter& emitter,
        llvm::StringRef text,
        llvm::StringRef content,
        std::string_view terminator
    ) -> llvm::StringRef {
        auto indent = compute_indent_from_final_line(text);

        if (indent.end() != content.end() && (terminator != "\"")) {
            DARK_DIAGNOSTIC(ContentBeforeStringTerminator, Error,
                "Only whitespace is permitted before the closing `{}` of a "
                "multi-line string.",
                std::string_view
            );
            emitter.emit(indent.end(), ContentBeforeStringTerminator, terminator);
        }

        return indent;
    }

    auto StringLiteral::compute_value(
        llvm::BumpPtrAllocator& allocator,
        LexerDiagnosticEmitter& emitter
    ) const -> llvm::StringRef {
        if (!is_terminated()) {
            return "";
        }

        std::string_view terminator = (m_multi_line_kind == Reflection ? "'''" : (m_multi_line_kind == ReflectionDoubleQuotes ? "\"\"\"" : "\""));
        auto const is_multi = is_reflection() || is_multi_line();

        auto indent = (is_multi
            ? check_indent(emitter, m_source, m_content, terminator)
            : llvm::StringRef{});

        if (!needs_validation() && (!is_multi || indent.empty())) {
            return m_content;
        }

        auto buffer = Buffer<char>(allocator.Allocate<char>(m_content.size()), m_content.size());

        auto result = expand_escape_sequence_and_remove_indent(
            emitter,
            m_content,
            indent,
            m_hash_level,
            terminator,
            is_reflection(),
            buffer
        );

        dark_assert(
            result.size() <= buffer.size(),
            std::format("Buffer overflow: grew from {} to {}", buffer.size(), result.size())
        );

        return result;
    }
} // namespace dark::lexer
