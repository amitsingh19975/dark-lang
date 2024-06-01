#include "lexer/token_buffer.hpp"
#include "common/assert.hpp"
#include "common/format.hpp"
#include "common/string_utils.hpp"
#include "lexer/numeric_literal.hpp"
#include "lexer/token_kind.hpp"
#include "lexer/string_literal.hpp"
#include <algorithm>
#include <cstddef>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Format.h>

namespace dark::lexer {

    [[nodiscard]] auto TokenizedBuffer::get_end_loc(TokenIndex token) const noexcept -> std::pair<LineIndex, unsigned> {
        auto line = get_line(token);
        auto column = get_column_number(token);
        auto text = get_token_text(token);

        if (auto [first_half, second_half] = text.rsplit('\n'); second_half.empty()) {
            column += first_half.size();
        } else {
            line.index += static_cast<unsigned>(first_half.count('\n')) + 1;
            column = static_cast<unsigned>(1 + second_half.size());
        }

        return {line, column};
    }

    [[nodiscard]] auto TokenizedBuffer::get_token_text(TokenIndex token) const noexcept -> llvm::StringRef {
        auto const& info = get_token_info(token);
        auto spelling = info.kind.fixed_spelling();

        if (!spelling.empty()) return spelling;

        if (info.kind.is_error()) {
            auto const& line = get_line_info(info.line);
            auto token_start = line.start + static_cast<unsigned>(info.column);
            return m_source->get_source().substr(token_start, static_cast<unsigned>(info.error_length));
        }

        if (info.kind.is_real_literal() || info.kind.is_integer_literal()) {
            auto const& line_info = get_line_info(info.line);
            auto token_start = line_info.start + static_cast<unsigned>(info.column);
            auto relaxed_token = NumericLiteral::lex(m_source->get_source().substr(token_start));
            dark_assert(relaxed_token.has_value(), (std::format(
                "{}:{}:{}: Could not reconstruct the numeric literal.",
                m_source->get_filename(),
                line_info.start,
                info.column
            )));
            return relaxed_token->get_source();
        }

        if (info.kind.is_string_literal()) {
            auto const& line_info = get_line_info(info.line);
            auto token_start = line_info.start + static_cast<unsigned>(info.column);
            auto relaxed_token = StringLiteral::lex(m_source->get_source().substr(token_start));
            dark_assert(relaxed_token.has_value(), (std::format(
                "{}:{}:{}: Could not reconstruct the string literal.",
                m_source->get_filename(),
                line_info.start,
                info.column
            )));
            return relaxed_token->get_source();
        }

        if (info.kind.is_file_start() || info.kind.is_file_end()) {
            return {};
        }

        dark_assert(info.kind.is_identifier(), Formatter("{}", info.kind).format());

        return m_value_store->identifier().get(info.id);
    }

    [[nodiscard]] inline constexpr auto compute_number_of_digits(unsigned number) noexcept -> unsigned {
        unsigned digits = 1;
        while (number) {
            number /= 10;
            ++digits;
        }
        return digits;
    }

    [[nodiscard]] auto TokenizedBuffer::get_print_widths(TokenIndex token) const noexcept -> PrintWidths {
        return {
            .index = compute_number_of_digits(static_cast<unsigned>(m_token_infos.size())),
            .kind = static_cast<unsigned>(get_kind(token).name().size()),
            .line = compute_number_of_digits(static_cast<unsigned>(get_line(token).index)),
            .column = compute_number_of_digits(static_cast<unsigned>(get_column_number(token))),
            .indent = compute_number_of_digits(get_indent_column_number(get_line(token)))
        };
    }

    auto TokenizedBuffer::print_token(llvm::raw_ostream& os, TokenIndex token, PrintWidths widths) const -> void {
        widths.widen(get_print_widths(token));
        auto token_index = token.index;
        auto const& info = get_token_info(token);
        auto token_text = get_token_text(token);

        os << llvm::formatv(
                "    { index: {0}, kind: {1}, line: {2}, column: {3}, indent: {4}, "
                "spelling: '{5}'",
                llvm::format_decimal(token_index, widths.index),
                llvm::right_justify(llvm::formatv("'{0}'", info.kind.name()).str(), widths.kind + 2),
                llvm::format_decimal(get_line_number(info.line), widths.line),
                llvm::format_decimal(get_column_number(token), widths.column),
                llvm::format_decimal(get_indent_column_number(info.line), widths.indent),
                token_text
            );

        switch (info.kind) {
            case TokenKind::Identifier:
                os << ", Identifier: '" << get_identifier(token).index << "'";
                break;
            case TokenKind::IntegerLiteral:
                os << ", Value: `";
                m_value_store->ints().get(get_int_literal(token)).print(os, /*isSigned*/false);
                os << "`";
                break;
            case TokenKind::RealLiteral:
                os << ", Value: `"; 
                m_value_store->reals().get(get_real_literal(token)).print(os);
                os << "`";
                break;
            case TokenKind::StringLiteral:
                os << ", Value: `" << m_value_store->string_literal().get(get_string_literal(token)) << "`";
                break;
            default:
                if (info.kind.is_opening_symbol()) {
                    os << ", closing token: " << get_matched_closing_token(token).index;
                } else if (info.kind.is_closing_symbol()) {
                    os << ", opening token: " << get_matched_opening_token(token).index;
                }
                break;
        }
        
        if (info.has_trailing_space) {
            os << ", trailing_space: true";
        }

        if (info.is_recovery) {
            os << ", recovery: true";
        }

        os << " }";
    }

    auto TokenizedBuffer::print(llvm::raw_ostream& os) const -> void {
        if (tokens().begin() == tokens().end()) {
            return;
        }

        os << "- Filename: " << m_source->get_filename() << "\n"
           << "  tokens: [\n";

        auto widths = PrintWidths{
            .index = compute_number_of_digits(static_cast<unsigned>(m_token_infos.size()))
        };

        for (auto token: tokens()) {
            widths.widen(get_print_widths(token));
        }

        for (auto token: tokens()) {
            print_token(os, token, widths);
            os << '\n';
        }

        os << "  ]\n";
    }
    auto TokenizedBuffer::print_token(llvm::raw_ostream& os, TokenIndex token) const -> void {
        print_token(os, token, {});
    }

    auto TokenizedBuffer::SourceBufferDiagnosticConverter::convert_loc(char const* loc, [[maybe_unused]] context_fn_t context_fn) const -> DiagnosticLocation {
        dark_assert(utils::string_contains_ptr(m_buffer->m_source->get_source(), loc), "loc is not in the buffer");
        std::ptrdiff_t offset = loc - m_buffer->m_source->get_source().begin();

        auto line_it = std::partition_point(
            m_buffer->m_line_infos.begin(),
            m_buffer->m_line_infos.end(),
            [offset](LineInfo const& line) { return line.start <= offset; }
        );

        dark_assert(line_it != m_buffer->m_line_infos.begin(), "loc is before the first line");
        --line_it;
        auto const line_number = static_cast<unsigned>(line_it - m_buffer->m_line_infos.begin());
        auto const column_number = static_cast<unsigned>(offset - line_it->start);

        auto line = m_buffer->m_source->get_source().substr(line_it->start, line_it->length);

        if (line_it->length == LineInfo::npos) {
            dark_assert(
                line.take_front(column_number).count('\n') == 0,
                Formatter("Assumption: there is no unlexed newline before the error column\n{}:{}:{}", m_buffer->m_source->get_filename(), line_number, column_number).format()
            );

            auto end_pos = line.find('\n', column_number);
            if (end_pos != llvm::StringRef::npos) {
                line = line.take_front(end_pos);
            }
        }

        return {
            .filename = m_buffer->m_source->get_filename(),
            .line = line,
            .line_number = line_number + 1,
            .column_number = column_number + 1,
        };
    }

    auto TokenDiagnosticConverter::convert_loc(TokenIndex loc, context_fn_t context_fn) const -> DiagnosticLocation {
        auto const& token_info = m_buffer->get_token_info(loc);
        auto const& line_info = m_buffer->get_line_info(token_info.line);
        auto token_start = m_buffer->m_source->get_source().data() + line_info.start + token_info.column;

        auto new_loc = TokenizedBuffer::SourceBufferDiagnosticConverter{m_buffer}.convert_loc(token_start, context_fn);
        new_loc.length = static_cast<unsigned>(m_buffer->get_token_text(loc).size());
        return new_loc;
    }

} // namespace dark::lexer