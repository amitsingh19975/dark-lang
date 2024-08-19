#include <catch2/catch_test_macros.hpp>
#include <diagnostics/diagnostic_consumer.hpp>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string_view>
#include "./mock.hpp"
#include "common/cow.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_emitter.hpp"


struct Mock {
    StreamMock consumer;
    FakeLocationConverter<unsigned> converter;
    dark::DiagnosticEmitter<unsigned> emitter{converter, *consumer.consumer.get()};
};

struct MockScope {
    MockScope(Mock& mock)
        : mock(mock)
    {}

    ~MockScope() {
        mock.converter.file = "";
        mock.converter.line = "";
        mock.consumer.reset();
    }

    Mock& mock;
};

using namespace dark::literal;

TEST_CASE("Diagnostic emitter test", "[diagnostic][emitter]") {
    Mock mock;

    SECTION("Emit Simple Error") {
        DARK_DIAGNOSTIC(TestDiagnostic, Error, "simple {}", std::string_view);
        {
            MockScope scope(mock);
            mock.converter.file = "test.cpp";

            mock.emitter.emit(1, TestDiagnostic, std::string_view{"error"});
            REQUIRE(mock.consumer.get_line() == "error: simple error");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
            REQUIRE(mock.consumer.empty());
        }

        {
            MockScope scope(mock);

            mock.converter.file = "test.cpp";

            mock.emitter.emit(2, TestDiagnostic, std::string_view{"error"});
            REQUIRE(mock.consumer.get_line() == "error: simple error");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Emit Error with suggestions") {
        DARK_DIAGNOSTIC(TestDiagnostic, Error, "simple {}", std::string_view);
        {
            MockScope scope(mock);
            mock.converter.file = "test.cpp";
            mock.converter.line = "auto out = get_stream(diagnostic.level, m_stream)";

            mock.emitter.build(1, TestDiagnostic, std::string_view{"error"})
                .add_note_suggestion("'auto' is not allowed in C++98 mode", dark::Span(0, 4))
                .emit();
            REQUIRE(mock.consumer.get_line() == "error: simple error");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
            REQUIRE(mock.consumer.get_line() == " 1 | auto out = get_stream(diagnostic.level, m_stream)");
            REQUIRE(mock.consumer.get_line() == "   | ^~~~");
            REQUIRE(mock.consumer.get_line() == "   | |");
            REQUIRE(mock.consumer.get_line() == "   | 'auto' is not allowed in C++98 mode");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Emit Simple Warning") {
        DARK_DIAGNOSTIC(TestDiagnostic, Warning, "simple {}", std::string_view);
        {
            MockScope scope(mock);
            mock.converter.file = "test.cpp";

            mock.emitter.emit(1, TestDiagnostic, std::string_view{"warning"});
            REQUIRE(mock.consumer.get_line() == "warning: simple warning");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
            REQUIRE(mock.consumer.empty());
        }

        {
            MockScope scope(mock);

            mock.converter.file = "test.cpp";

            mock.emitter.emit(2, TestDiagnostic, std::string_view{"warning"});
            REQUIRE(mock.consumer.get_line() == "warning: simple warning");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Emit Simple Info") {
        DARK_DIAGNOSTIC(TestDiagnostic, Info, "simple {}", std::string_view);
        {
            MockScope scope(mock);
            mock.converter.file = "test.cpp";

            mock.emitter.emit(1, TestDiagnostic, std::string_view{"info"});
            REQUIRE(mock.consumer.get_line() == "info: simple info");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
            REQUIRE(mock.consumer.empty());
        }

        {
            MockScope scope(mock);

            mock.converter.file = "test.cpp";

            mock.emitter.emit(2, TestDiagnostic, std::string_view{"info"});
            REQUIRE(mock.consumer.get_line() == "info: simple info");
            REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
            REQUIRE(mock.consumer.empty());
        }
    }

    SECTION("Emit Simple Note") {
        DARK_DIAGNOSTIC(TestDiagnostic, Warning, "simple {}", std::string_view);
        DARK_DIAGNOSTIC(TestDiagnosticNote, Note, "note");

        MockScope scope(mock);
        mock.converter.file = "test.cpp";

        mock.emitter.build(1, TestDiagnostic, std::string_view{"warning"})
            .add_note(2, TestDiagnosticNote)
            .emit();
        REQUIRE(mock.consumer.get_line() == "warning: simple warning");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
        REQUIRE(mock.consumer.get_line() == "note: note");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
        REQUIRE(mock.consumer.empty());
    }

    SECTION("Emit simple child note") {
        DARK_DIAGNOSTIC(TestDiagnostic, Warning, "simple {}", std::string_view);

        MockScope scope(mock);
        mock.converter.file = "test.cpp";

        mock.emitter.build(1, TestDiagnostic, std::string_view{"warning"})
            .add_child_note_context("note")
            .add_child_warning_context("simple child warning")
            .emit();
        REQUIRE(mock.consumer.get_line() == "warning: simple warning");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
        REQUIRE(mock.consumer.get_line() == "note: note");
        REQUIRE(mock.consumer.get_line() == "warning: simple child warning");
        REQUIRE(mock.consumer.empty());
    }

    SECTION("Emit complex child note") {
        DARK_DIAGNOSTIC(TestDiagnostic, Warning, "simple {}", std::string_view);
        DARK_DIAGNOSTIC(TestDiagnosticInfo, Info, "simple {}", std::string_view);

        MockScope scope(mock);
        mock.converter.file = "test.cpp";

        mock.emitter.build(1, TestDiagnostic, std::string_view{"warning"})
            .add_child_note_context("note"_cow)
            .add_child_warning_context("simple child warning"_cow)
            .add_info(2, TestDiagnosticInfo, std::string_view{"child info"})
            .add_child_error_context("simple child error")
            .add_child_info_context("simple child info")
            .emit();
        REQUIRE(mock.consumer.get_line() == "warning: simple warning");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:1");
        REQUIRE(mock.consumer.get_line() == "note: note");
        REQUIRE(mock.consumer.get_line() == "warning: simple child warning");
        REQUIRE(mock.consumer.get_line() == "info: simple child info");
        REQUIRE(mock.consumer.get_line() == "  --> test.cpp:1:2");
        REQUIRE(mock.consumer.get_line() == "error: simple child error");
        REQUIRE(mock.consumer.get_line() == "info: simple child info");
        REQUIRE(mock.consumer.empty());
    }
}
