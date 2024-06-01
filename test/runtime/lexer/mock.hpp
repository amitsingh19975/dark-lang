#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_consumer.hpp"
#include "diagnostics/dianostic_converter.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

struct StreamMock {
    StreamMock()
        : consumer(new dark::StreamDiagnosticConsumer(stream))
    {}
    StreamMock(StreamMock const&) = delete;
    StreamMock(StreamMock&&) = delete;
    StreamMock& operator=(StreamMock const&) = delete;
    StreamMock& operator=(StreamMock&&) = delete;
    ~StreamMock() = default;

    void reset() {
        stream.flush();
        buffer.clear();
        stream.str().clear();
        consumer->reset();
    }
    
    auto get_line() -> std::string {
        stream.flush();
        auto it = buffer.find('\n');
        if (it == std::string::npos) {
            auto line = buffer;
            reset();
            return line;
        }

        auto line = llvm::StringRef(buffer).substr(0, it).str();
        buffer.erase(0, it + 1);
        return line;
    }

    constexpr auto empty() const -> bool {
        return buffer.empty();
    }

    std::string buffer;
    llvm::raw_string_ostream stream{buffer};
    std::unique_ptr<dark::StreamDiagnosticConsumer> consumer;
};

struct MockDiagnosticConsumer: dark::DiagnosticConsumer {
    void consume(dark::Diagnostic &&diagnostic) override {
        diagnostics.push_back(std::move(diagnostic));
    }

    constexpr auto empty() const noexcept -> bool {
        return diagnostics.empty();
    }

    llvm::SmallVector<dark::Diagnostic, 0> diagnostics;

    static auto create() -> MockDiagnosticConsumer* {
        static auto* consumer = new MockDiagnosticConsumer();
        return consumer;
    }
};

struct FakeLocationConverter: dark::DiagnosticConverter<char const*> {
    using context_fn_t =  dark::DiagnosticConverter<char const*>::context_fn_t;

    llvm::StringRef line{};
    llvm::StringRef file{"test.cpp"};

    mutable llvm::SmallVector<unsigned> line_offsets{0};

    void set_line(llvm::StringRef line) {
        this->line = line;
        line_offsets.clear();
        line_offsets.push_back(0);
        for (unsigned i = 0; i < line.size(); ++i) {
            if (line[i] == '\n') {
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

    auto convert_loc(char const* loc, context_fn_t context_fn) const -> dark::DiagnosticLocation override {
        auto [line_num, column_num] = find_loc(loc);
        return dark::DiagnosticLocation{
            .filename = file,
            .line = line,
            .line_number = line_num,
            .column_number = column_num,
        };
    }
};
