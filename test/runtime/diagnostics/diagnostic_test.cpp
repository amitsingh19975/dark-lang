#include <catch2/catch_test_macros.hpp>
#include <diagnostics/diagnostic_consumer.hpp>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include "./mock.hpp"

TEST_CASE("Diagnostic output test", "[diagnostic][output]") {
    StreamMock mock;

    SECTION("Test output message") {
        auto diag = dark::Diagnostic {
            .level = dark::DiagnosticLevel::Error,
            .collections = {}
        };

        diag.build(
            dark::DiagnosticKind::EmptyDigitSequence, 
            dark::DiagnosticLocation {
                .filename = "",
                .line = "",
                .line_number = 1,
                .column_number = 1
            },
            dark::DiagnosticLevel::Error,
            dark::Formatter("cannot open file '{}' for reading: {}", "std/std.dark", "No such file or directory")
        ).emit();
        mock.consumer->consume(std::move(diag));
        REQUIRE(mock.get_line() == "error: cannot open file 'std/std.dark' for reading: No such file or directory");
        REQUIRE(mock.empty());
    }

    SECTION("Test file location") {
        auto diag = dark::Diagnostic {
            .level = dark::DiagnosticLevel::Error,
            .collections = {}
        };

        diag.build(
            dark::DiagnosticKind::EmptyDigitSequence, 
            dark::DiagnosticLocation {
                .filename = "std/std.dark",
                .line = "",
                .line_number = 1,
                .column_number = 1
            },
            dark::DiagnosticLevel::Error,
            dark::Formatter("cannot open file '{}' for reading: {}", "std/std.dark", "No such file or directory")
        ).emit();
        mock.consumer->consume(std::move(diag));
        REQUIRE(mock.get_line() == "error: cannot open file 'std/std.dark' for reading: No such file or directory");
        REQUIRE(mock.get_line() == "  --> std/std.dark:1:1");
        REQUIRE(mock.empty());
    }

    SECTION("Test suggestions with enough space") {
        auto diag = dark::Diagnostic {
            .level = dark::DiagnosticLevel::Error,
            .collections = {}
        };

        diag.build(
            dark::DiagnosticKind::EmptyDigitSequence, 
            dark::DiagnosticLocation {
                .filename = "std/std.dark",
                .line = "auto out = get_stream(diagnostic.level, m_stream)",
                .line_number = 1,
                .column_number = 1
            },
            dark::DiagnosticLevel::Error,
            dark::Formatter("cannot open file '{}' for reading: {}", "std/std.dark", "No such file or directory")
        )
        .add_note(dark::make_borrowed("'auto' is not allowed in C++98 mode"), dark::Span(0, 4))
        .add_info(dark::make_borrowed("diagnostic"), dark::Span(11, 12))
        .add_error(dark::make_borrowed("llvm::raw_ostream&"), dark::Span(11, 15))
        .emit();
        mock.consumer->consume(std::move(diag));
        REQUIRE(mock.get_line() == "error: cannot open file 'std/std.dark' for reading: No such file or directory");
        REQUIRE(mock.get_line() == "  --> std/std.dark:1:1");
        REQUIRE(mock.get_line() == " 1 | auto out = get_stream(diagnostic.level, m_stream)");
        REQUIRE(mock.get_line() == "   | ^~~~       ^~~~");
        REQUIRE(mock.get_line() == "   | |         /|");
        REQUIRE(mock.get_line() == "   | |        | diagnostic");
        REQUIRE(mock.get_line() == "   | |        llvm::raw_ostream&");
        REQUIRE(mock.get_line() == "   | 'auto' is not allowed in C++98 mode");
        REQUIRE(mock.empty());
    }


    SECTION("Test suggestions with not enough space") {
        auto diag = dark::Diagnostic {
            .level = dark::DiagnosticLevel::Error,
            .collections = {}
        };

        diag.build(
            dark::DiagnosticKind::EmptyDigitSequence, 
            dark::DiagnosticLocation {
                .filename = "std/std.dark",
                .line = "auto out = get_stream(diagnostic.level, m_stream)",
                .line_number = 1,
                .column_number = 1
            },
            dark::DiagnosticLevel::Error,
            dark::Formatter("cannot open file '{}' for reading: {}", "std/std.dark", "No such file or directory")
        )
        .add_note(dark::make_borrowed("'auto' is not allowed in C++98 mode"), dark::Span(0, 4))
        .add_info(dark::make_borrowed("diagnostic"), dark::Span(2, 12))
        .add_error(dark::make_borrowed("llvm::raw_ostream&"), dark::Span(2, 15))
        .emit();
        mock.consumer->consume(std::move(diag));
        REQUIRE(mock.get_line() == "error: cannot open file 'std/std.dark' for reading: No such file or directory");
        REQUIRE(mock.get_line() == "  --> std/std.dark:1:1");
        REQUIRE(mock.get_line() == " 1 | auto out = get_stream(diagnostic.level, m_stream)");
        REQUIRE(mock.get_line() == "   | ^~^~~~~~~~~~~~~");
        REQUIRE(mock.get_line() == "   | | |");
        REQUIRE(mock.get_line() == "   | | |-llvm::raw_ostream&");
        REQUIRE(mock.get_line() == "   | | |-diagnostic");
        REQUIRE(mock.get_line() == "   | 'auto' is not allowed in C++98 mode");
        REQUIRE(mock.empty());
    }

    SECTION("Multiple line test") {
        auto diag = dark::Diagnostic {
            .level = dark::DiagnosticLevel::Error,
            .collections = {}
        };

        diag.build(
            dark::DiagnosticKind::EmptyDigitSequence, 
            dark::DiagnosticLocation {
                .filename = "std/std.dark",
                .line = "auto out = get_stream(diagnostic.level, m_stream)",
                .line_number = 1,
                .column_number = 1
            },
            dark::DiagnosticLevel::Error,
            dark::Formatter("cannot open file '{}' for reading: {}", "std/std.dark", "No such file or directory")
        )
        .add_note(dark::make_borrowed("'auto' is not allowed in C++98 mode"), dark::Span(0, 4))
        .add_info(dark::make_borrowed("diagnostic"), dark::Span(2, 12))
        .add_error(dark::make_borrowed("llvm::raw_ostream&"), dark::Span(2, 15))
        .next_child_section(dark::DiagnosticLocation {
            .filename = "std/std.dark",
            .line = "    auto out = get_stream(diagnostic.level, m_stream)",
            .line_number = 2,
            .column_number = 4,
            .length = 4
        })
        .add_note(dark::make_borrowed("'auto' is not allowed in C++98 mode"))
        .add_info(dark::make_borrowed("diagnostic"), dark::Span(2 + 4, 12 + 4))
        .add_error(dark::make_borrowed("llvm::raw_ostream&"), dark::Span(2 + 4, 15 + 4))
        .emit();
        mock.consumer->consume(std::move(diag));
        llvm::outs() << mock.buffer << "\n";
        REQUIRE(mock.get_line() == "error: cannot open file 'std/std.dark' for reading: No such file or directory");
        REQUIRE(mock.get_line() == "  --> std/std.dark:1:1");
        REQUIRE(mock.get_line() == " 1 | auto out = get_stream(diagnostic.level, m_stream)");
        REQUIRE(mock.get_line() == "   | ^~^~~~~~~~~~~~~");
        REQUIRE(mock.get_line() == "   | | |");
        REQUIRE(mock.get_line() == "   | | |-llvm::raw_ostream&");
        REQUIRE(mock.get_line() == "   | | |-diagnostic");
        REQUIRE(mock.get_line() == "   | 'auto' is not allowed in C++98 mode");
        REQUIRE(mock.get_line() == " 2 |     auto out = get_stream(diagnostic.level, m_stream)");
        REQUIRE(mock.get_line() == "   |     ^~^~~~~~~~~~~~~");
        REQUIRE(mock.get_line() == "   |     | |");
        REQUIRE(mock.get_line() == "   |     | |-llvm::raw_ostream&");
        REQUIRE(mock.get_line() == "   |     | |-diagnostic");
        REQUIRE(mock.get_line() == "   |     'auto' is not allowed in C++98 mode");
        REQUIRE(mock.empty());
    }
}