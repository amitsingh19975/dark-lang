#ifndef __DARK_DIAGNOSTIC_DIAGNOSTIC_KIND_HPP__
#define __DARK_DIAGNOSTIC_DIAGNOSTIC_KIND_HPP__

#include "common/enum.hpp"

namespace dark {
    DARK_DEFINE_RAW_ENUM_CLASS(DiagnosticKind, std::uint16_t) {
        #define DARK_DIAGNOSTIC_KIND(Name) DARK_RAW_ENUM_ENUMERATOR(Name)
        #include "diagnostics/diagnostic_kind.def"
    };

    struct DiagnosticKind: public DARK_ENUM_BASE(DiagnosticKind) {
        #define DARK_DIAGNOSTIC_KIND(Name) DARK_ENUM_CONSTANT_DECL(Name)
        #include "diagnostics/diagnostic_kind.def"
    };

    #define DARK_DIAGNOSTIC_KIND(Name) DARK_ENUM_CONSTANT_DEFINITION(DiagnosticKind, Name)
    #include "diagnostics/diagnostic_kind.def"
}

#endif // __DARK_DIAGNOSTIC_DIAGNOSTIC_KIND_HPP__