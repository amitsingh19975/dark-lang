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

template<typename LocT = unsigned>
struct FakeLocationConverter: dark::DiagnosticConverter<LocT> {
    llvm::StringRef line{};
    llvm::StringRef file{"test.cpp"};
    using context_fn_t =  dark::DiagnosticConverter<LocT>::context_fn_t;

    auto convert_loc(LocT loc, context_fn_t context_fn) const -> dark::DiagnosticLocation override{
        if constexpr (std::is_same_v<LocT, dark::DiagnosticLocation>) {
           return loc; 
        } else  {
            return dark::DiagnosticLocation{
                .filename = file,
                .line = line,
                .line_number = 1,
                .column_number = loc,
            };
        }
    }
};
