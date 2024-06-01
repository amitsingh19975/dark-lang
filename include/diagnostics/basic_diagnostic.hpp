#ifndef __DARK_DIAGNOSTIC_BASIC_DIAGNOSTIC_HPP__
#define __DARK_DIAGNOSTIC_BASIC_DIAGNOSTIC_HPP__

#include "common/assert.hpp"
#include "common/cow.hpp"
#include "common/format.hpp"
#include "common/small_string.hpp"
#include "diagnostics/diagnostic_kind.hpp"
#include <llvm/ADT/Any.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include "common/span.hpp"

#define DARK_DIAGNOSTIC(DiagnosticName, Level, Format, ...)     \
  static constexpr auto DiagnosticName =                        \
      ::dark::detail::DiagnosticBase<__VA_ARGS__>{              \
          ::dark::DiagnosticKind::DiagnosticName,               \
          ::dark::DiagnosticLevel::Level, Format                \
    }

namespace dark {

    enum class DiagnosticLevel: std::uint8_t {
        Error,
        Warning,
        Note,
        Info,
    };

    struct DiagnosticLocation {
        detail::SmallStringRef filename{};
        detail::SmallStringRef line{};
        
        // 1-based line number and 0 means invalid
        unsigned line_number{0};

        // 1-based column number and 0 means invalid
        unsigned column_number{0};

        unsigned length{1};

        constexpr auto get_filename() const noexcept -> llvm::StringRef {
            return filename;
        }

        constexpr auto get_line() const noexcept -> llvm::StringRef {
            return line;
        }

        constexpr auto can_be_printed() const noexcept -> bool {
            return get_filename().trim().empty() == false;
        }

        friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, DiagnosticLocation const& loc) {
            llvm::StringRef temp_filename = loc.get_filename().trim();
            if (temp_filename.empty()) return os;
            os << temp_filename;

            if (loc.line_number > 0) {
                os << ":" << loc.line_number;
                if (loc.column_number > 0) {
                    os << ":" << loc.column_number;
                }
            }
            return os;
        }
    };

    enum class DiagnosticPatchKind: std::uint8_t {
        None,
        Remove,
        Insert,
    };

    struct DiagnosticMessageSuggestions {
        CowString message;
        Span span;
        DiagnosticLevel level;
        DiagnosticPatchKind patch_kind{DiagnosticPatchKind::None};
        CowString patch_content{};
    };

    struct DiagnosticMessageContext {
        CowString message;
        DiagnosticLevel level;
    };

    struct DiagnosticMessage {
        DiagnosticLocation location;
        llvm::SmallVector<DiagnosticMessageSuggestions> suggestions;
    };

    struct DiagnosticMessageCollection {
        DiagnosticKind kind;
        DiagnosticLevel level;
        Formatter formatter;
        llvm::SmallVector<DiagnosticMessage> messages;
        llvm::SmallVector<DiagnosticMessageContext, 0> contexts;
    };

    namespace detail {
        template <typename T>
        concept IsRawStreamable = requires(T&& t) {
            { std::declval<llvm::raw_ostream&>() << t };
        };
    }

    struct Diagnostic {
        DiagnosticLevel level;
        llvm::SmallVector<DiagnosticMessageCollection, 0> collections;

    private:
        struct [[nodiscard]] DiagnosticMessageBuilder {
            DiagnosticMessageBuilder(
                Diagnostic& diag, 
                DiagnosticKind kind, 
                DiagnosticLocation location,
                DiagnosticLevel level, 
                Formatter format
            )
                : m_diagnostic(&diag)
                , m_message{kind, level, std::move(format), { DiagnosticMessage { .location = location, .suggestions = {} } }, {}}
            {}

            DiagnosticMessageBuilder(DiagnosticMessageBuilder const&) = delete;
            DiagnosticMessageBuilder(DiagnosticMessageBuilder&&) = default;
            DiagnosticMessageBuilder& operator=(DiagnosticMessageBuilder const&) = delete;
            DiagnosticMessageBuilder& operator=(DiagnosticMessageBuilder&&) = default;
            ~DiagnosticMessageBuilder() = default;

            [[nodiscard]] auto add_suggestion(DiagnosticLevel diag_level, CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                dark_assert(!m_message.messages.empty(), "DiagnosticMessageBuilder::add_suggestion() called without a message");
                m_message.messages.back().suggestions.push_back({std::move(message), span, diag_level});
                return *this;
            }
            
            [[nodiscard]] auto add_info(CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                return add_suggestion(DiagnosticLevel::Info, std::move(message), span);
            }

            [[nodiscard]] auto add_note(CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                return add_suggestion(DiagnosticLevel::Note, std::move(message), span);
            }

            [[nodiscard]] auto add_warning(CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                return add_suggestion(DiagnosticLevel::Warning, std::move(message), span);
            }

            [[nodiscard]] auto add_error(CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                return add_suggestion(DiagnosticLevel::Error, std::move(message), span);
            }

            [[nodiscard]] auto add_patch(CowString message, CowString patch_text, DiagnosticPatchKind kind, Span span = {}) -> DiagnosticMessageBuilder& {
                dark_assert(!m_message.messages.empty(), "DiagnosticMessageBuilder::add_patch() called without a message");
                auto temp_level = kind == DiagnosticPatchKind::Insert ? DiagnosticLevel::Info : DiagnosticLevel::Error;
                m_message.messages.back().suggestions.push_back({
                    std::move(message),
                    span,
                    temp_level,
                    kind,
                    std::move(patch_text)}
                );
                return *this;
            }
    
            [[nodiscard]] auto add_insert_patch(CowString message, CowString insert_text, unsigned pos) -> DiagnosticMessageBuilder& {
                auto const size = static_cast<unsigned>(insert_text.borrow().size());
                return add_patch(
                    std::move(message),
                    std::move(insert_text),
                    DiagnosticPatchKind::Insert,
                    Span(pos, pos + size)
                );
            }

            [[nodiscard]] auto add_remove_patch(CowString message, Span span = {}) -> DiagnosticMessageBuilder& {
                return add_patch(std::move(message), CowString(), DiagnosticPatchKind::Remove, span);
            }

            [[nodiscard]] auto next_child_section(DiagnosticLocation location) -> DiagnosticMessageBuilder& {
                dark_assert(!m_message.messages.empty(), "DiagnosticMessageBuilder::next_child_section() called without a message");
                m_message.messages.emplace_back(DiagnosticMessage{location, {}});
                return *this;
            }

            auto emit() -> void {
                m_diagnostic->collections.push_back(std::move(m_message));
            }

            [[nodiscard]] auto next(
                DiagnosticKind kind, 
                DiagnosticLocation location, 
                DiagnosticLevel diag_level,
                Formatter format
            ) -> DiagnosticMessageBuilder {
                emit();
                return DiagnosticMessageBuilder{*m_diagnostic, kind, location, diag_level, std::move(format)};
            }

            [[nodiscard]] auto next(
                DiagnosticKind kind,
                DiagnosticLocation location, 
                DiagnosticLevel diag_level,
                llvm::StringLiteral message
            ) -> DiagnosticMessageBuilder {
                return next(kind, location, diag_level, Formatter{message});
            }

        private:
            Diagnostic* m_diagnostic;
            DiagnosticMessageCollection m_message;
        };

    public:
        [[nodiscard]] auto build(
            DiagnosticKind kind, 
            DiagnosticLocation location, 
            DiagnosticLevel diag_level,
            Formatter format
        ) -> DiagnosticMessageBuilder {
            return DiagnosticMessageBuilder{*this, kind, location, diag_level, std::move(format)};
        }
        
        [[nodiscard]] auto build(
            DiagnosticKind kind,
            DiagnosticLocation location, 
            DiagnosticLevel diag_level,
            llvm::StringLiteral message
        ) -> DiagnosticMessageBuilder {
            return build(kind, location, diag_level, Formatter{message});
        }
    };
    
    namespace detail {
        template <typename... Args>
        struct DiagnosticBase {
            
            explicit constexpr DiagnosticBase(
                DiagnosticKind kind, 
                DiagnosticLevel level, 
                llvm::StringLiteral format
            ) noexcept
                : kind(kind)
                , level(level)
                , format(format) 
            {
                static_assert((... && !std::is_same_v<Args, llvm::StringRef>),
                            "Use std::string or llvm::StringLiteral for diagnostics to "
                            "avoid lifetime issues.");
            }

            DiagnosticKind kind;
            DiagnosticLevel level;
            llvm::StringLiteral format;
        };
    } // namespace detail
    
} // namespace dark


#endif // __DARK_DIAGNOSTIC_BASIC_DIAGNOSTIC_HPP__