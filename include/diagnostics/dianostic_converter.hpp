#ifndef __DARK_DIAGNOSTIC_CONVERTER_HPP__
#define __DARK_DIAGNOSTIC_CONVERTER_HPP__

#include "diagnostics/basic_diagnostic.hpp"
#include <llvm/ADT/STLFunctionalExtras.h>
namespace dark {

    template <typename LocT>
    struct DiagnosticConverter {
        using context_fn_t = llvm::function_ref<void(DiagnosticLocation, detail::DiagnosticBase<> const&)>;

        virtual ~DiagnosticConverter() = default;
        virtual auto convert_loc(LocT loc, context_fn_t context_fn) const -> DiagnosticLocation = 0;
    };
}

#endif // __DARK_DIAGNOSTIC_CONVERTER_HPP__