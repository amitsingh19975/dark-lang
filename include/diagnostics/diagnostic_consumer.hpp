#ifndef __DARK_DIAGNOSTIC_CONSUMER_HPP__
#define __DARK_DIAGNOSTIC_CONSUMER_HPP__

#include "diagnostics/basic_diagnostic.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
namespace dark {
    class DiagnosticConsumer {
    public:
        virtual ~DiagnosticConsumer() = default;
        virtual auto consume(Diagnostic&& diagnostic) -> void = 0;
        virtual auto flush() -> void {}
    };

    class StreamDiagnosticConsumer : public DiagnosticConsumer {
    public:
        explicit StreamDiagnosticConsumer(llvm::raw_ostream& stream)
            : m_stream(&stream)
        {}
        StreamDiagnosticConsumer(StreamDiagnosticConsumer const&) = default;
        StreamDiagnosticConsumer(StreamDiagnosticConsumer&&) = default;
        StreamDiagnosticConsumer& operator=(StreamDiagnosticConsumer const&) = default;
        StreamDiagnosticConsumer& operator=(StreamDiagnosticConsumer&&) = default;
        ~StreamDiagnosticConsumer() override = default;


        auto consume(Diagnostic&& diagnostic) -> void override;
        auto flush() -> void override { m_stream->flush(); }

        constexpr auto reset() noexcept -> void {
            has_printed = false;
        }
    private:
        llvm::raw_ostream* m_stream;
        bool has_printed{ false };
    };

    auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer&;

    class ErrorTrackingDiagnosticConsumer: public DiagnosticConsumer {
    public:
        explicit ErrorTrackingDiagnosticConsumer(DiagnosticConsumer* consumer)
            : m_consumer(consumer)
        {}

        ErrorTrackingDiagnosticConsumer(ErrorTrackingDiagnosticConsumer const&) = default;
        ErrorTrackingDiagnosticConsumer(ErrorTrackingDiagnosticConsumer&&) = default;
        ErrorTrackingDiagnosticConsumer& operator=(ErrorTrackingDiagnosticConsumer const&) = default;
        ErrorTrackingDiagnosticConsumer& operator=(ErrorTrackingDiagnosticConsumer&&) = default;
        ~ErrorTrackingDiagnosticConsumer() override = default;

        void consume(Diagnostic &&diagnostic) override {
            m_seen_error |= diagnostic.level == DiagnosticLevel::Error;
            m_consumer->consume(std::move(diagnostic));
        }

        [[nodiscard]] constexpr auto seen_error() const noexcept -> bool {
            return m_seen_error;
        }

        void flush() override {
            m_consumer->flush();
        }

        void reset() noexcept {
            m_seen_error = false;
        }
    private:
        DiagnosticConsumer* m_consumer;
        bool m_seen_error{ false };
    };
}

#endif // __DARK_DIAGNOSTIC_CONSUMER_HPP__