#include "lexer/token_kind.hpp"

namespace dark::lexer {

    DARK_DEFINE_ENUM_CLASS_NAMES(TokenKind) {
        #define DARK_TOKEN(TokenName, SnakeCaseName) DARK_ENUM_CLASS_NAME_STRING(TokenName)
        #include "lexer/token_kind.def"
    };
} // namespace dark::lexer
