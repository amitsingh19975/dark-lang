#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <diagnostics/diagnostic_consumer.hpp>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string_view>
#include "./mock.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include "diagnostics/diagnostic_kind.hpp"
#include "diagnostics/sorting_diagnostic_consumer.hpp"


struct Mock {
    std::unique_ptr<dark::SortingDiagnosticConsumer> consumer{new dark::SortingDiagnosticConsumer(MockDiagnosticConsumer::create())};
    FakeLocationConverter<dark::DiagnosticLocation> converter;
    dark::DiagnosticEmitter<dark::DiagnosticLocation> emitter{converter, *consumer.get()};
};

TEST_CASE("Sorted diagnostic test", "[diagnostic][sorting]") {
    Mock mock;

    DARK_DIAGNOSTIC(TestDiagnostic, Error, "{}", std::string_view);

    mock.emitter.emit({ "f", "line", 1, 1 }, TestDiagnostic, std::string_view("M1"));
    mock.emitter.emit({ "f", "line", 2, 1 }, TestDiagnostic, std::string_view("M2"));
    mock.emitter.emit({ "f", "line", 1, 3 }, TestDiagnostic, std::string_view("M3"));
    mock.emitter.emit({ "f", "line", 3, 4 }, TestDiagnostic, std::string_view("M4"));
    mock.emitter.emit({ "f", "line", 3, 2 }, TestDiagnostic, std::string_view("M5"));
    
    auto& consumer = *MockDiagnosticConsumer::create();
    mock.consumer->flush();

    REQUIRE(!consumer.empty());
    auto& diags = consumer.diagnostics;
    std::reverse(diags.begin(), diags.end());
    REQUIRE(diags.size() == 5);
    auto compare_consume_top = [&diags](dark::DiagnosticKind kind, dark::DiagnosticLevel level, unsigned line, unsigned column, std::string_view message) {
        auto top = diags.pop_back_val();

        REQUIRE(top.collections[0].kind == kind);
        REQUIRE(top.collections[0].level == level);
        REQUIRE(top.collections[0].messages[0].location.line_number == line);
        REQUIRE(top.collections[0].messages[0].location.column_number == column);
        REQUIRE(top.collections[0].formatter.format() == message);
    };

    compare_consume_top(TestDiagnostic.kind, dark::DiagnosticLevel::Error, 1, 1, "M1");
    compare_consume_top(TestDiagnostic.kind, dark::DiagnosticLevel::Error, 1, 3, "M3");
    compare_consume_top(TestDiagnostic.kind, dark::DiagnosticLevel::Error, 2, 1, "M2");
    compare_consume_top(TestDiagnostic.kind, dark::DiagnosticLevel::Error, 3, 2, "M5");
    compare_consume_top(TestDiagnostic.kind, dark::DiagnosticLevel::Error, 3, 4, "M4");

    REQUIRE(diags.empty());
}
