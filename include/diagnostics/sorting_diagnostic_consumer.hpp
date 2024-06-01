#ifndef __DARK_DIAGNOSTIC_SORTING_DIAGNOSTIC_HPP__
#define __DARK_DIAGNOSTIC_SORTING_DIAGNOSTIC_HPP__

#include "common/assert.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_consumer.hpp"
#include <llvm/ADT/STLExtras.h>

namespace dark {
    struct SortingDiagnosticConsumer: public DiagnosticConsumer {

        explicit SortingDiagnosticConsumer(DiagnosticConsumer* consumer)
            : m_consumer(consumer)
        {}

        SortingDiagnosticConsumer(SortingDiagnosticConsumer const&) = default;
        SortingDiagnosticConsumer(SortingDiagnosticConsumer&&) = default;
        SortingDiagnosticConsumer& operator=(SortingDiagnosticConsumer const&) = default;
        SortingDiagnosticConsumer& operator=(SortingDiagnosticConsumer&&) = default;
        ~SortingDiagnosticConsumer() override {
            dark_assert(m_diagnostics.empty(), "Diagnostics not flushed");
        }

        auto consume(Diagnostic&& diagnostic) -> void override {
            m_diagnostics.push_back(std::move(diagnostic));
        }

        auto flush() -> void override {
            llvm::stable_sort(m_diagnostics,
                [](const Diagnostic& lhs, const Diagnostic& rhs) {
                    dark_assert(!lhs.collections[0].messages.empty(), "Diagnostic with no messages");
                    dark_assert(!rhs.collections[0].messages.empty(), "Diagnostic with no messages");
                    const auto& lhs_loc = lhs.collections[0].messages[0].location;
                    const auto& rhs_loc = rhs.collections[0].messages[0].location;
                    return std::tie(lhs_loc.filename, lhs_loc.line_number,
                                    lhs_loc.column_number) <
                            std::tie(rhs_loc.filename, rhs_loc.line_number,
                                    rhs_loc.column_number);
                });

            for (auto& diagnostic : m_diagnostics) {
                m_consumer->consume(std::move(diagnostic));
            }
            m_diagnostics.clear();
        }

    private:
        llvm::SmallVector<Diagnostic, 0> m_diagnostics;
        DiagnosticConsumer* m_consumer;
    };
}

#endif // __DARK_DIAGNOSTIC_SORTING_DIAGNOSTIC_HPP__