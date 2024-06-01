#include <catch2/catch_test_macros.hpp>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/raw_ostream.h>
#include "lexer/string_literal.hpp"
#include "./mock.hpp"

using namespace dark::lexer;

TEST_CASE("String Literal", "[string_literal]") {
    SECTION("Single Line String") {
        auto s = StringLiteral::lex(R"("Hello, World!")");
        REQUIRE(s.has_value());
        REQUIRE(s->get_content() == "Hello, World!");
        REQUIRE(s->get_hash_level() == 0);
        REQUIRE(s->is_multi_line() == false);
        REQUIRE(s->is_format_string() == false);
        REQUIRE(s->needs_validation() == false);
        REQUIRE(s->is_terminated() == true);
        REQUIRE(s->get_ident_error_pos() == -1);
        REQUIRE(s->is_reflection() == false);
    }
    
    SECTION("Multiline Line String") {
        {
            auto s = StringLiteral::lex(
R"("
Hello,
World!
")"
            );
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\nHello,\nWorld!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(
R"("
    Hello,
    World!
")"
            );
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\n    Hello,\n    World!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(
R"("
    Hello,
World!
")"
            );
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\n    Hello,\nWorld!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(R"("\n    Hello,\nWorld!")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\\n    Hello,\\nWorld!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }

    SECTION("Escaped Format String") {
        {
            auto s = StringLiteral::lex(R"("Hello, {{World}}!")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, {{World}}!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(
R"("
Hello,
{{World}}!
")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\nHello,\n{{World}}!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }

    SECTION("Format String") {
        {
            auto s = StringLiteral::lex(R"("Hello, {World}!")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, {World}!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == true);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(
R"("
Hello,
{World}!
")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\nHello,\n{World}!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == true);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(
R"("
Hello,
\u{1F499}!
")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\nHello,\n\\u{1F499}!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }

    SECTION("Raw String Literal") {
        {
            auto s = StringLiteral::lex(R"(#"Hello, World!"#)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        
        {
            auto s = StringLiteral::lex(R"(###"Hello, World!"###)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 3);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }
}

TEST_CASE("Reflection String Literal", "[string_literal_reflection]") {
    SECTION("Single Line String") {
        {
            auto s = StringLiteral::lex(R"('''Hello, World!''')");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
        {
            auto s = StringLiteral::lex(R"("""Hello, World!""")");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
    }

    SECTION("Multiline Line String") {
        {
            auto s = StringLiteral::lex(
R"('''
Hello,
World!
''')"
            );
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello,\nWorld!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
        {
            auto s = StringLiteral::lex(
R"('''
    Hello,
World!
''')"
            );
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "    Hello,\nWorld!\n");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
    }

    SECTION("Escaped Format String") {
        {
            auto s = StringLiteral::lex(R"('''Hello, {{World}}!''')");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, {{World}}!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
    }

    SECTION("Format String") {
        {
            auto s = StringLiteral::lex(R"('''Hello, {World}!''')");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, {World}!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == true);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == true);
        }
    }
}

struct Mock {
    StreamMock consumer;
    FakeLocationConverter converter;
    LexerDiagnosticEmitter emitter{converter, *consumer.consumer.get()};
    llvm::BumpPtrAllocator allocator;
};

struct MockScope {
    MockScope(Mock& mock)
        : mock(mock)
    {}

    ~MockScope() {
        mock.converter.file = "";
        mock.converter.line = "";
        mock.consumer.reset();
        mock.allocator.Reset();
    }

    Mock& mock;
};

TEST_CASE("Computed String Literal", "[string_literal_computed]") {
    auto mock = Mock{};
    SECTION("Single Line String") {
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello, World!")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "Hello, World!");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello, \nWorld!")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, \\nWorld!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "Hello, \nWorld!");
            REQUIRE(mock.consumer.empty());
        }

        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello,\xfa \nWorld!")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello,\\xfa \\nWorld!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "Hello,\xfa \nWorld!");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{1f499}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "💙");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Checking octal errors") {
        auto scope = MockScope(mock);
        mock.converter.file = "test.cpp";
        mock.converter.line = R"("Hello,\09 \nWorld!")";

        auto s = StringLiteral::lex(mock.converter.line);
        REQUIRE(s.has_value());
        REQUIRE(s->get_content() == "Hello,\\09 \\nWorld!");
        REQUIRE(s->get_hash_level() == 0);
        REQUIRE(s->is_multi_line() == false);
        REQUIRE(s->is_format_string() == false);
        REQUIRE(s->needs_validation() == true);
        REQUIRE(s->is_terminated() == true);
        REQUIRE(s->get_ident_error_pos() == -1);
        REQUIRE(s->is_reflection() == false);

        auto c = s->compute_value(mock.allocator, mock.emitter);
        REQUIRE(!c.empty());
        REQUIRE(!mock.consumer.empty());
        REQUIRE(mock.consumer.get_line() == "error: Invalid octal digit.");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:9");
        REQUIRE(mock.consumer.get_line() == R"( 1 | "Hello,\09 \nWorld!")");
        REQUIRE(mock.consumer.get_line() ==   "   |          ^");
        REQUIRE(mock.consumer.get_line() ==   "   |          |");
        REQUIRE(mock.consumer.get_line() ==   "   |          Expected an octal digit, but got '9'");
        REQUIRE(mock.consumer.empty());
    }

    SECTION("Checking hexadecimal errors") {
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello,\x")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello,\\x");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Hexadecimal escape sequence is too short.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:8");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "Hello,\x")");
            REQUIRE(mock.consumer.get_line() ==   "   |         ^");
            REQUIRE(mock.consumer.get_line() ==   "   |         |");
            REQUIRE(mock.consumer.get_line() ==   "   |         Expected 2 hexadecimal digits after this, but got 0 digits");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello,\xhh \nWorld!")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello,\\xhh \\nWorld!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Hexadecimal escape sequence contains invalid digit.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:9");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "Hello,\xhh \nWorld!")");
            REQUIRE(mock.consumer.get_line() ==   "   |          ^");
            REQUIRE(mock.consumer.get_line() ==   "   |          |");
            REQUIRE(mock.consumer.get_line() ==   "   |          Expected a hexadecimal digit, but got 'h'");
            REQUIRE(mock.consumer.empty());
        }

        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("Hello,\xAh \nWorld!")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello,\\xAh \\nWorld!");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Hexadecimal escape sequence contains invalid digit.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:10");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "Hello,\xAh \nWorld!")");
            REQUIRE(mock.consumer.get_line() ==   "   |           ^");
            REQUIRE(mock.consumer.get_line() ==   "   |           |");
            REQUIRE(mock.consumer.get_line() ==   "   |           Expected a hexadecimal digit, but got 'h'");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Checking unicode errors") {
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\\u{}");
            REQUIRE(s->get_hash_level() == 0);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Unicode escape sequence is missing digits.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:3");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{}")");
            REQUIRE(mock.consumer.get_line() ==   "   |    ^");
            REQUIRE(mock.consumer.empty());
        }

        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{fffffff}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Unicode escape sequence has too many digits.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:4");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{fffffff}")");
            REQUIRE(mock.consumer.get_line() ==   "   |     ^~~~~~~");
            REQUIRE(mock.consumer.get_line() ==   "   |    /|");
            REQUIRE(mock.consumer.get_line() ==   "   |   | Try reducing the number of digits in the unicode escape sequence");
            REQUIRE(mock.consumer.get_line() ==   "   |   Expected at most 6 digits, but got 7 digits");
            REQUIRE(mock.consumer.empty());
        }

        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{1GFFFF}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "u{1GFFFF}");
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Unicode escape sequence contains invalid hexadecimal digits.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:4");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{1GFFFF}")");
            REQUIRE(mock.consumer.get_line() ==   "   |     ^~~~~~");
            REQUIRE(mock.consumer.empty());
        }
        
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{11FFFF}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "u{11FFFF}");
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Invalid unicode escape sequence. Code point is too large.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:4");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{11FFFF}")");
            REQUIRE(mock.consumer.get_line() ==   "   |     ^~~~~~");
            REQUIRE(mock.consumer.get_line() ==   "   |     |");
            REQUIRE(mock.consumer.get_line() ==   "   |     Unicode code points must be in the range 0x0 to 0x10FFFF.");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{D8f0}")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "u{D8f0}");
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Invalid unicode escape sequence. Code point is a surrogate.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:4");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{D8f0}")");
            REQUIRE(mock.consumer.get_line() ==   "   |     ^~~~");
            REQUIRE(mock.consumer.get_line() ==   "   |     |");
            REQUIRE(mock.consumer.get_line() ==   "   |     Unicode code points in the range 0xD800 to 0xDFFF are reserved for surrogates.");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("\u{1f49")";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "u{1f49");
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Unicode escape sequence is missing closing brace.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:3");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\u{1f49")");
            REQUIRE(mock.consumer.get_line() ==   "   |    ^");
            REQUIRE(mock.consumer.get_line() ==   "   |    |");
            REQUIRE(mock.consumer.get_line() ==   "   |    Try adding a closing brace `}`");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Checking error for unknown escape sequence") {
        auto scope = MockScope(mock);
        mock.converter.file = "test.cpp";
        mock.converter.line = R"("\q")";

        auto s = StringLiteral::lex(mock.converter.line);
        REQUIRE(s.has_value());

        auto c = s->compute_value(mock.allocator, mock.emitter);
        REQUIRE(c == "q");
        REQUIRE(!mock.consumer.empty());
        REQUIRE(mock.consumer.get_line() == "error: Unknown escape sequence `q`.");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
        REQUIRE(mock.consumer.get_line() == R"( 1 | "\q")");
        REQUIRE(mock.consumer.get_line() ==   "   |   ^");
        REQUIRE(mock.consumer.empty());
    }

    SECTION("Multiline String") {
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("
                Hello,
                World!
                "
            )";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "\nHello,\nWorld!\n");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("
                    Hello,
                World!
                "
            )";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "\n    Hello,\nWorld!\n");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("
                Hello,
                    World!
                "
            )";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(c == "\nHello,\n    World!\n");
            REQUIRE(mock.consumer.empty());
        }
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"("
                Hello,
            World!
                "
            )";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            REQUIRE(!c.empty());
            REQUIRE(!mock.consumer.empty());
            REQUIRE(mock.consumer.get_line() == "error: Indentation does not match that of the closing `\"` in a multi-line string literal.");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:25");
            REQUIRE(mock.consumer.get_line() == R"( 1 | "\n                Hello,\n            World!\n                "\n)");
            REQUIRE(mock.consumer.get_line() == R"(   |                            ^~~~~~~~~~~~)");
            REQUIRE(mock.consumer.get_line() == R"(   |                            |)");
            REQUIRE(mock.consumer.get_line() == R"(   |                            Expected at least '16' characters of indentation, but found '12')");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Reflection") {
        {
            auto scope = MockScope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = R"('''cpp
                #include <iostream>
                int add(int a, int b) {
                    return a + b;
                }
                void print() {
                    std::cout << "Hello World\n";
                }
                '''
            )";

            auto s = StringLiteral::lex(mock.converter.line);
            REQUIRE(s.has_value());

            auto c = s->compute_value(mock.allocator, mock.emitter);
            llvm::StringRef out = R"(#include <iostream>
int add(int a, int b) {
    return a + b;
}
void print() {
    std::cout << "Hello World\n";
}
)";

            // auto i = 0ul;
            // for (; i < std::min(out.size(), c.size()); ++i) {
            //     auto lhs = c[i];
            //     auto rhs = out[i];
            //     llvm::outs() << static_cast<char>(lhs) << " = " << static_cast<char>(rhs) << ", ";
            // };
            // while (i < c.size()) {
            //     auto lhs = c[i];
            //     llvm::outs() << static_cast<char>(lhs) << " = ?" << ", ";
            //     ++i;
            // }
            REQUIRE(c == out);
            REQUIRE(mock.consumer.empty());
        }
    }
}

TEST_CASE("Testing Raw String Literal", "[raw_string_literal]") {
    auto mock = Mock{};

    SECTION("Single Line") {
        {
            auto s = StringLiteral::lex(R"(#"Hello, World!"#)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, World!");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(R"(#"Hello, {} World!"#)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, {} World!");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(R"(#"Hello, \#{} World!"#)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, \\#{} World!");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == true);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
        {
            auto s = StringLiteral::lex(R"(#"Hello, \#u{1f499} World!"#)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "Hello, \\#u{1f499} World!");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == false);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == true);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }

    SECTION("Multiline") {
        {
            auto s = StringLiteral::lex(R"(#"
Hello,
World!
"#
)");
            REQUIRE(s.has_value());
            REQUIRE(s->get_content() == "\nHello,\nWorld!\n");
            REQUIRE(s->get_hash_level() == 1);
            REQUIRE(s->is_multi_line() == true);
            REQUIRE(s->is_format_string() == false);
            REQUIRE(s->needs_validation() == false);
            REQUIRE(s->is_terminated() == true);
            REQUIRE(s->get_ident_error_pos() == -1);
            REQUIRE(s->is_reflection() == false);
        }
    }
}