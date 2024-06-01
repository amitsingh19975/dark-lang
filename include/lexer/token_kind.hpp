#ifndef __DARK_LEXER_TOKEN_KIND_HPP__
#define __DARK_LEXER_TOKEN_KIND_HPP__

#include "common/assert.hpp"
#include "common/cow.hpp"
#include "common/enum.hpp"
#include <initializer_list>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
namespace dark::lexer {

    DARK_DEFINE_RAW_ENUM_CLASS(TokenKind, std::uint8_t) {
        #define DARK_TOKEN(TokenName, SnakeCaseName) DARK_RAW_ENUM_ENUMERATOR(TokenName)
        #include "lexer/token_kind.def"
    };

    struct TokenKind: public DARK_ENUM_BASE(TokenKind) {
        #define DARK_TOKEN(TokenName, SnakeCaseName) DARK_ENUM_CONSTANT_DECL(TokenName)
        #include "lexer/token_kind.def"

        using EnumBase::EnumBase;

        static const llvm::ArrayRef<TokenKind> keyword_tokens;

        [[nodiscard]] constexpr auto is_symbol() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_SYMBOL_TOKEN(TokenName, Spelling, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }
        [[nodiscard]] constexpr auto is_grouping_symbol() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #define DARK_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }
        [[nodiscard]] constexpr auto is_one_char_symbol() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_ONE_CHAR_SYMBOL_TOKEN(TokenName, Spelling, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }
        [[nodiscard]] constexpr auto is_keyword() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_KEYWORD_TOKEN(TokenName, Spelling, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }
        [[nodiscard]] constexpr auto fixed_spelling() const noexcept -> llvm::StringRef { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_SYMBOL_TOKEN(TokenName, Spelling, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return Spelling;
                #define DARK_KEYWORD_TOKEN(TokenName, Spelling, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return Spelling;
                #include "lexer/token_kind.def"

                default: return "";
            }
        }
        [[nodiscard]] constexpr auto expected_parse_tree_size() const noexcept -> std::int8_t { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_EXPECTED_PARSE_TREE_SIZE(TokenName, Size, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return Size;
                #include "lexer/token_kind.def"

                default: return 0;
            }
        }

        [[nodiscard]] constexpr auto is_closing_symbol() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }

        [[nodiscard]] constexpr auto is_opening_symbol() const noexcept -> bool { 
            switch (DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                #define DARK_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName, SnakeCaseName) case DARK_RAW_ENUM_VALUE(TokenKind, TokenName): return true;
                #include "lexer/token_kind.def"

                default: return false;
            }
        }
        
        auto opening_symbol() const noexcept -> bool { 
            auto result = is_opening_symbol();
            dark_assert(result, "TokenKind is not an opening symbol");
            return result;
        }
        auto closing_symbol() const noexcept -> bool { 
            auto result = is_closing_symbol();
            dark_assert(result, "TokenKind is not a closing symbol");
            return result;
        }

        constexpr auto is_sized_type_literal() const noexcept -> bool {
            return false;
        }

        constexpr auto is_one_of(std::initializer_list<TokenKind> kinds) const noexcept -> bool {
            for (auto kind: kinds) {
                if (kind == DARK_CAST_RAW_ENUM(TokenKind, (as_int()))) {
                    return true;
                }
            }
            return false;
        }

        auto to_cow_string() const noexcept -> CowString {
            auto spelling = fixed_spelling();
            if (spelling.empty()) {
                return EnumBase::to_cow_string();
            } else {
                return make_borrowed(spelling);
            }
        }

        #define DARK_TOKEN(TokenName, SnakeCaseName) \
            [[nodiscard]] constexpr auto is_##SnakeCaseName() const noexcept -> bool { return DARK_CAST_RAW_ENUM(TokenKind, (as_int())) == TokenName; }
        #include "lexer/token_kind.def"

    private:
        static const TokenKind s_keyword_tokens_storage[];
    };

    #define DARK_TOKEN(TokenName, SnakeCaseName) DARK_ENUM_CONSTANT_DEFINITION(TokenKind, TokenName)
    #include "lexer/token_kind.def"

    constexpr TokenKind TokenKind::s_keyword_tokens_storage[] = {
        #define DARK_KEYWORD_TOKEN(TokenName, Spelling, SnakeCaseName) TokenKind::TokenName,
        #include "lexer/token_kind.def"
    };

    constexpr llvm::ArrayRef<TokenKind> TokenKind::keyword_tokens = s_keyword_tokens_storage;


} // namespace dark::lexer

#endif // __DARK_LEXER_TOKEN_KIND_HPP__