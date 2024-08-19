#include "common/cow.hpp"
#include "lexer/string_literal.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_consumer.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>


struct FakeLocationConverter: dark::DiagnosticConverter<char const*> {
    using context_fn_t =  dark::DiagnosticConverter<char const*>::context_fn_t;

    llvm::StringRef line{};
    llvm::StringRef file{"test.cpp"};

    mutable llvm::SmallVector<unsigned> line_offsets{0};

    void set_line(llvm::StringRef new_line) {
        this->line = new_line;
        line_offsets.clear();
        line_offsets.push_back(0);
        for (unsigned i = 0; i < new_line.size(); ++i) {
            if (new_line[i] == '\n') {
                line_offsets.push_back(i + 1);
            }
        }
    }

    std::pair<unsigned, unsigned> find_loc(char const* loc) const {
        auto offset = static_cast<unsigned>(loc - line.data());
        auto it = std::upper_bound(line_offsets.begin(), line_offsets.end(), offset);
        auto line_number = static_cast<unsigned>(it - line_offsets.begin());
        return {line_number, offset - line_offsets[line_number - 1]};
    }

    auto convert_loc(char const* loc, [[maybe_unused]] context_fn_t context_fn) const -> dark::DiagnosticLocation override {
        auto [line_num, column_num] = find_loc(loc);
        return dark::DiagnosticLocation{
            .filename = file,
            .line = line,
            .line_number = line_num,
            .column_number = column_num,
        };
    }
};


struct Mock {
    dark::StreamDiagnosticConsumer consumer{llvm::errs()};
    FakeLocationConverter converter;
    dark::lexer::LexerDiagnosticEmitter emitter{converter, consumer};
    llvm::BumpPtrAllocator allocator;
};

int main(int argc, char** argv) {
    llvm::cl::opt<std::string> STDSourceFilename("stds", llvm::cl::desc("standard source file"), llvm::cl::value_desc("std_source"));
    llvm::cl::ParseCommandLineOptions(argc, argv, "Highly sophisticated compiler\n");

    std::string_view originFilename = "std/std.dark";
    if (!STDSourceFilename.empty()) {
        originFilename = STDSourceFilename.getValue();
    }

    const char* str = "Test";
    auto temp = dark::CowString(str);
    llvm::outs() << temp.borrow() << "\n";
    // auto diag = dark::Diagnostic {
    //         .level = dark::DiagnosticLevel::Error,
    //         .collections = {}
    // };

    // auto mock = Mock{};

    // mock.converter.file = "test.cpp";
    // mock.converter.line = R"("""lexer
    //         Hello,
    //         World!
    //         """
    //        )";

    // auto allocator = llvm::BumpPtrAllocator();
    // auto s = dark::lexer::StringLiteral::lex(mock.converter.line);

    // // auto c = s->compute_value(allocator, mock.emitter);

    // llvm::outs() << "'" << s->get_codeblock_prefix() << "'\n";

    return 0;
}
