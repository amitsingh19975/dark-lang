#include "diagnostics/diagnostic_kind.hpp"

namespace dark {
    DARK_DEFINE_ENUM_CLASS_NAMES(DiagnosticKind) = {
        #define DARK_DIAGNOSTIC_KIND(Name) DARK_ENUM_CLASS_NAME_STRING(Name)
        #include "diagnostics/diagnostic_kind.def"
    };
}