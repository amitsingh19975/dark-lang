#ifndef __DARK_LEXER_TOKEN_BUFFER_HPP__
#define __DARK_LEXER_TOKEN_BUFFER_HPP__

#include "base/index_base.hpp"
#include "common/assert.hpp"
#include "common/ostream.hpp"
#include "base/value_store.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include "diagnostics/dianostic_converter.hpp"
#include "lexer/token_kind.hpp"
#include "source/source_buffer.hpp"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/iterator.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <utility>

namespace dark::lexer {

    struct TokenizedBuffer;

    struct TokenIndex: public IndexBase {
        using IndexBase::IndexBase;
        
        static const TokenIndex invalid;
        static const TokenIndex non_first_comment_token;
    };

    constexpr TokenIndex TokenIndex::invalid = TokenIndex();
    constexpr TokenIndex TokenIndex::non_first_comment_token = TokenIndex(1);

    struct LineIndex: public IndexBase {
        using IndexBase::IndexBase;

        static const LineIndex invalid;
    };

    constexpr LineIndex LineIndex::invalid = LineIndex();

    struct TokenIterator: 
        public llvm::iterator_facade_base<TokenIterator, std::random_access_iterator_tag, TokenIndex const, int>,
        public Printable<TokenIterator>
    {
        TokenIterator() = delete;

        constexpr explicit TokenIterator(TokenIndex token) noexcept
            : m_token(token)
        {
        }

        constexpr TokenIterator(TokenIterator const&) noexcept = default;
        constexpr TokenIterator(TokenIterator&&) noexcept = default;
        constexpr TokenIterator& operator=(TokenIterator const&) noexcept = default;
        constexpr TokenIterator& operator=(TokenIterator&&) noexcept = default;
        constexpr ~TokenIterator() noexcept = default;

        constexpr auto operator==(TokenIterator const& other) const noexcept -> bool {
            return m_token == other.m_token;
        }

        constexpr auto operator<=>(TokenIterator const& other) const noexcept -> std::strong_ordering {
            return m_token <=> other.m_token;
        }

        constexpr auto operator*() const noexcept -> TokenIndex {
            return m_token;
        }

        using iterator_facade_base::operator-;
        constexpr auto operator-(TokenIterator const& other) const noexcept -> int {
            return m_token.index - other.m_token.index;
        }

        using iterator_facade_base::operator+;
        constexpr auto operator+(int n) const noexcept -> TokenIterator {
            return TokenIterator(TokenIndex(m_token.index + n));
        }

        constexpr auto operator+=(int n) noexcept -> TokenIterator& {
            m_token.index += n;
            return *this;
        }

        constexpr auto operator-=(int n) noexcept -> TokenIterator& {
            m_token.index -= n;
            return *this;
        }

        auto print(llvm::raw_ostream& out) const -> void {
            out << m_token;
        }
    private:
        friend struct TokenizedBuffer;
        TokenIndex m_token;
    };

    struct TokenDiagnosticConverter: DiagnosticConverter<TokenIndex> {
        constexpr explicit TokenDiagnosticConverter(TokenizedBuffer const* buffer) noexcept
            : m_buffer(buffer)
        {
        }
        constexpr TokenDiagnosticConverter(TokenDiagnosticConverter const&) noexcept = default;
        constexpr TokenDiagnosticConverter(TokenDiagnosticConverter&&) noexcept = default;
        constexpr TokenDiagnosticConverter& operator=(TokenDiagnosticConverter const&) noexcept = default;
        constexpr TokenDiagnosticConverter& operator=(TokenDiagnosticConverter&&) noexcept = default;
        constexpr ~TokenDiagnosticConverter() noexcept = default;

        auto convert_loc(TokenIndex loc, context_fn_t context_fn) const -> DiagnosticLocation override;
    private:
        TokenizedBuffer const* m_buffer;
    };

    struct TokenizedBuffer: public Printable<TokenizedBuffer> {
        [[nodiscard]] constexpr auto get_kind(TokenIndex token) const noexcept -> TokenKind {
            return get_token_info(token).kind;
        }

        [[nodiscard]] constexpr auto get_line(TokenIndex token) const noexcept -> LineIndex {
            return get_token_info(token).line;
        }

        [[nodiscard]] constexpr auto get_line_number(TokenIndex token) const noexcept -> unsigned {
            return get_line_number(get_line(token));
        }

        [[nodiscard]] constexpr auto get_line_number(LineIndex line) const noexcept -> unsigned {
            return line.as_unsigned() + 1;
        }

        [[nodiscard]] constexpr auto get_column_number(TokenIndex index) const noexcept -> unsigned {
            return static_cast<unsigned>(std::max(get_token_info(index).column + 1, 0));
        }
        
        [[nodiscard]] constexpr auto get_indent_column_number(LineIndex index) const noexcept -> unsigned {
            return get_line_info(index).indent + 1;
        }

        [[nodiscard]] constexpr auto get_identifier(TokenIndex token) const noexcept -> IdentifierId {
            auto const& info = get_token_info(token);
            return info.id;
        }
        
        [[nodiscard]] constexpr auto get_int_literal(TokenIndex token) const noexcept -> IntId {
            auto const& info = get_token_info(token);
            return info.integer;
        }
        
        [[nodiscard]] auto get_real_literal(TokenIndex token) const noexcept -> RealId {
            auto const& info = get_token_info(token);
            return info.reals;
        }

        [[nodiscard]] constexpr auto get_string_literal(TokenIndex token) const noexcept -> StringLiteralId {
            auto const& info = get_token_info(token);
            return info.string_literal;
        }

        [[nodiscard]] constexpr auto get_type_literal_size(TokenIndex token) const noexcept -> IntId {
            auto const& info = get_token_info(token);
            return info.integer;
        }

        [[nodiscard]] constexpr auto get_matched_closing_token(TokenIndex opening_token) const noexcept -> TokenIndex {
            auto const& info = get_token_info(opening_token);
            dark_assert(info.kind.is_opening_symbol(), "Token is not an opening token!");
            return info.close_paren;
        }
        
        [[nodiscard]] constexpr auto get_matched_opening_token(TokenIndex closing_token) const noexcept -> TokenIndex {
            auto const& info = get_token_info(closing_token);
            dark_assert(info.kind.is_closing_symbol(), "Token is not a closing token!");
            return info.open_paren;
        }

        [[nodiscard]] auto has_leading_whitespace(TokenIndex token) const noexcept -> bool {
            auto it = TokenIterator(token);
            return it == tokens().begin() || get_token_info(*(it - 1)).has_trailing_space;
        }
        [[nodiscard]] constexpr auto has_trailing_whitespace(TokenIndex token) const noexcept -> bool {
            return get_token_info(token).has_trailing_space;
        }

        [[nodiscard]] auto get_end_loc(TokenIndex token) const noexcept -> std::pair<LineIndex, unsigned>;
        [[nodiscard]] auto get_token_text(TokenIndex token) const noexcept -> llvm::StringRef;
        [[nodiscard]] constexpr auto is_recovery_token(TokenIndex token) const noexcept -> bool {
            return get_token_info(token).is_recovery;
        }

        [[nodiscard]] constexpr auto get_next_line(LineIndex token) const noexcept -> LineIndex {
            auto line = LineIndex(token.index + 1);
            dark_assert(static_cast<std::size_t>(line.index) < m_line_infos.size(), "LineIndex overflow!");
            return line;
        }
        [[nodiscard]] constexpr auto get_prev_line(LineIndex token) const noexcept -> LineIndex {
            dark_assert(token.index > 0, "LineIndex underflow!");
            return LineIndex(token.index - 1);
        }

        auto print(llvm::raw_ostream& os) const -> void;
        auto print_token(llvm::raw_ostream& os, TokenIndex token) const -> void;

        constexpr auto has_error() const noexcept -> bool { return m_has_errors; }
        auto tokens() const noexcept -> llvm::iterator_range<TokenIterator> {
            return llvm::make_range(TokenIterator(TokenIndex(0)), TokenIterator(TokenIndex(m_token_infos.size())));
        }

        [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return m_token_infos.size(); }
        [[nodiscard]] constexpr auto expected_parse_tree_size() const noexcept -> int { return m_expected_parse_tree_size; }
        [[nodiscard]] constexpr auto source() const noexcept -> SourceBuffer const& { return *m_source; }

    private:
        friend struct TokenIterator;
        friend struct TokenDiagnosticConverter;

    private:
        explicit TokenizedBuffer(SharedValueStores& value_store, SourceBuffer& source)
            : m_value_store(&value_store)
            , m_source(&source)
        {
        }

        struct SourceBufferDiagnosticConverter: DiagnosticConverter<char const*> {
            constexpr explicit SourceBufferDiagnosticConverter(TokenizedBuffer const* source) noexcept
                : m_buffer(source)
            {
            }
            constexpr SourceBufferDiagnosticConverter(SourceBufferDiagnosticConverter const&) noexcept = default;
            constexpr SourceBufferDiagnosticConverter(SourceBufferDiagnosticConverter&&) noexcept = default;
            constexpr SourceBufferDiagnosticConverter& operator=(SourceBufferDiagnosticConverter const&) noexcept = default;
            constexpr SourceBufferDiagnosticConverter& operator=(SourceBufferDiagnosticConverter&&) noexcept = default;
            constexpr ~SourceBufferDiagnosticConverter() noexcept = default;

            auto convert_loc(char const* loc, context_fn_t context_fn) const -> DiagnosticLocation override;
        private:
            TokenizedBuffer const* m_buffer;
        };

        struct PrintWidths {
            constexpr auto widen(PrintWidths const& other) noexcept -> void {
                index = std::max(index, other.index);
                kind = std::max(kind, other.kind);
                line = std::max(line, other.line);
                column = std::max(column, other.column);
                indent = std::max(indent, other.indent);
            };

            unsigned index;
            unsigned kind;
            unsigned line;
            unsigned column;
            unsigned indent;
        };

        struct TokenInfo {
            TokenKind kind;
            bool has_trailing_space;
            bool is_recovery;
            LineIndex line;
            int32_t column;

            union {
                static_assert(sizeof(TokenIndex) <= sizeof(int32_t), "Unable to pack token and identifier index into the same space!");
                IdentifierId id = IdentifierId::invalid;
                StringLiteralId string_literal;
                IntId integer;
                RealId reals;
                TokenIndex open_paren;
                TokenIndex close_paren;
                int32_t error_length;
            };
        };

        struct LineInfo {
            static constexpr unsigned npos = ~0u;
            constexpr explicit LineInfo(unsigned start) noexcept
                : start(start)
                , length(static_cast<unsigned>(llvm::StringRef::npos))
                , indent(0)
            {
            }

            constexpr LineInfo(unsigned start, unsigned length) noexcept
                : start(start)
                , length(length)
                , indent(0)
            {
            }

            constexpr LineInfo(LineInfo const&) noexcept = default;
            constexpr LineInfo(LineInfo&&) noexcept = default;
            constexpr LineInfo& operator=(LineInfo const&) noexcept = default;
            constexpr LineInfo& operator=(LineInfo&&) noexcept = default;
            constexpr ~LineInfo() noexcept = default;

            unsigned start;
            unsigned length;
            unsigned indent;
        };


        [[nodiscard]] constexpr auto get_line_info(LineIndex line) const noexcept -> LineInfo const& {
            return m_line_infos[line];
        }

        [[nodiscard]] constexpr auto get_line_info(LineIndex line) noexcept -> LineInfo& {
            return m_line_infos[line];
        }

        [[nodiscard]] constexpr auto get_token_info(TokenIndex line) const noexcept -> TokenInfo const& {
            return m_token_infos[line];
        }

        [[nodiscard]] constexpr auto get_token_info(TokenIndex line) noexcept -> TokenInfo& {
            return m_token_infos[line];
        }

        [[nodiscard]] auto add_token(TokenInfo info) -> TokenIndex {
            auto id = TokenIndex(static_cast<std::size_t>(m_token_infos.size()));
            dark_assert(id.index >= 0, "TokenIndex overflow!");
            m_token_infos.emplace_back(std::move(info));
            m_expected_parse_tree_size += info.kind.expected_parse_tree_size();
            return id;
        }

        [[nodiscard]] auto get_print_widths(TokenIndex token) const noexcept -> PrintWidths;

        auto print_token(llvm::raw_ostream& os, TokenIndex token, PrintWidths widths) const -> void;

    private:
        llvm::BumpPtrAllocator m_allocator;
        SharedValueStores* m_value_store;
        SourceBuffer* m_source;
        llvm::SmallVector<std::unique_ptr<std::string>> m_computed_strings;
        llvm::SmallVector<TokenInfo> m_token_infos;
        llvm::SmallVector<LineInfo> m_line_infos;
        int m_expected_parse_tree_size{};
        bool m_has_errors{false};
    };

    using LexerDiagnosticEmitter = DiagnosticEmitter<char const*>;
    using TokenDiagnosticEmitter = DiagnosticEmitter<TokenIndex>;

} // namespace dark::lexer

#endif // __DARK_LEXER_TOKEN_BUFFER_HPP__