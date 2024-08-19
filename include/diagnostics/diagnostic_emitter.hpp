#ifndef __DARK_DIAGNOSTIC_EMITTER_HPP__
#define __DARK_DIAGNOSTIC_EMITTER_HPP__

#include "common/assert.hpp"
#include "common/cow.hpp"
#include "common/format.hpp"
#include "common/span.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_consumer.hpp"
#include "diagnostics/dianostic_converter.hpp"
#include <llvm/ADT/STLFunctionalExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <string_view>
#include <type_traits>

namespace dark {
    template <typename LocT, typename AnnotationFn>
    struct DiagnosticAnnotationScope;

    template <typename LocT>
    struct DiagnosticEmitter {
        struct [[nodiscard]] DiagnosticBuilder{
            DiagnosticBuilder(DiagnosticEmitter const&) = delete;
            DiagnosticBuilder(DiagnosticBuilder&&) noexcept = default;
            DiagnosticBuilder& operator=(DiagnosticBuilder const&) = delete;
            DiagnosticBuilder& operator=(DiagnosticBuilder&&) noexcept = default;
            ~DiagnosticBuilder() = default;

            template <typename... Args>
            auto add_note(LocT loc, detail::DiagnosticBase<Args...> const& base, Args&&... args) -> DiagnosticBuilder& {
                dark_assert(base.level == DiagnosticLevel::Note);
                add_message(loc, base, Formatter(base.format, std::forward<Args>(args)...));
                return *this;
            }

            template <typename... Args>
            auto add_info(LocT loc, detail::DiagnosticBase<Args...> const& base, Args&&... args) -> DiagnosticBuilder& {
                dark_assert(base.level == DiagnosticLevel::Info);
                add_message(loc, base, Formatter(base.format, std::forward<Args>(args)...));
                return *this;
            }

            template <typename... Args>
            auto add_warning(LocT loc, detail::DiagnosticBase<Args...> const& base, Args&&... args) -> DiagnosticBuilder& {
                dark_assert(base.level == DiagnosticLevel::Warning);
                add_message(loc, base, Formatter(base.format, std::forward<Args>(args)...));
                return *this;
            }

            template <typename... Args>
            auto add_error(LocT loc, detail::DiagnosticBase<Args...> const& base, Args&&... args) -> DiagnosticBuilder& {
                dark_assert(base.level == DiagnosticLevel::Error);
                add_message(loc, base, Formatter(base.format, std::forward<Args>(args)...));
                return *this;
            }

            auto add_info_suggestion(CowString message, Span span = {}) -> DiagnosticBuilder& {
                add_suggestion(DiagnosticLevel::Info, std::move(message), span);
                return *this;
            }

            auto add_note_suggestion(CowString message, Span span = {}) -> DiagnosticBuilder& {
                add_suggestion(DiagnosticLevel::Note, std::move(message), span);
                return *this;
            }

            auto add_warning_suggestion(CowString message, Span span = {}) -> DiagnosticBuilder& {
                add_suggestion(DiagnosticLevel::Warning, std::move(message), span);
                return *this;
            }

            auto add_error_suggestion(CowString message, Span span = {}) -> DiagnosticBuilder& {
                add_suggestion(DiagnosticLevel::Error, std::move(message), span);
                return *this;
            }

            auto next_child_section(LocT loc) -> DiagnosticBuilder& {
                dark_assert(m_diagnostic.collections.size() > 0, "Cannot add a child location without a message");
                m_diagnostic.collections.back().messages.push_back(DiagnosticMessage {
                    .location = m_emitter->m_converter->convert_loc(
                        loc,
                        [this](DiagnosticLocation context_loc, detail::DiagnosticBase<> const& context_base) {
                            add_message_with_loc(context_loc, context_base, Formatter(context_base.format));
                        }
                    ),
                    .suggestions = {},
                });
                return *this;
            }

            auto add_child_note_context(CowString message) -> DiagnosticBuilder& {
                add_child_context(DiagnosticLevel::Note, std::move(message));
                return *this;
            }

            auto
            add_child_info_context(CowString message) -> DiagnosticBuilder& {
                add_child_context(DiagnosticLevel::Info, std::move(message));
                return *this;
            }

            auto add_child_warning_context(CowString message) -> DiagnosticBuilder& {
                add_child_context(DiagnosticLevel::Warning, std::move(message));
                return *this;
            }

            auto add_child_error_context(CowString message) -> DiagnosticBuilder& {
                add_child_context(DiagnosticLevel::Error, std::move(message));
                return *this;
            }

            auto add_patch_insert(CowString message, CowString insert_text, unsigned pos, DiagnosticLevel level = DiagnosticLevel::Info) -> DiagnosticBuilder& {
                auto span = Span::from_size(pos, insert_text.borrow().size());
                add_patch(level, std::move(message), std::move(insert_text), span, DiagnosticPatchKind::Insert);
            }

            auto add_patch_insert(CowString message, CowString insert_text, DiagnosticLevel level = DiagnosticLevel::Info) -> DiagnosticBuilder& {
                auto span = Span::from_size(0, message.borrow().size()).to_relative();
                add_patch(level, std::move(message), std::move(insert_text), span, DiagnosticPatchKind::Insert);
            }

            auto patch_remove(std::string_view message, Span span, DiagnosticLevel level = DiagnosticLevel::Error) -> DiagnosticBuilder& {
                add_patch(level, std::move(message), CowString(), span, DiagnosticPatchKind::Insert);
            }

            auto set_span_length(unsigned length) -> DiagnosticBuilder& {
                dark_assert(m_diagnostic.collections.size() > 0, "Cannot set length without a message");
                m_diagnostic.collections.back().messages.back().location.length = length;
                return *this;
            }

            auto emit() -> void {
                for (auto& annotation : m_emitter->m_annotations) {
                    annotation(*this);
                }
                m_emitter->m_consumer->consume(std::move(m_diagnostic));
            }

        private:
            friend struct DiagnosticEmitter<LocT>;

            template <typename... Args>
            DiagnosticBuilder(DiagnosticEmitter<LocT>* emitter, LocT loc, detail::DiagnosticBase<Args...> const& base, Formatter formatter) noexcept
                : m_emitter(emitter)
                , m_diagnostic{ .level = base.level, .collections = {} }
            {
                dark_assert(base.level != DiagnosticLevel::Note, "Note messages must be added with add_note");
                add_message(loc, base, std::move(formatter));
            }

            auto add_suggestion(DiagnosticLevel level, CowString message, Span span) -> void {
                dark_assert(m_diagnostic.collections.size() > 0, "Cannot add a suggestion without a message");
                m_diagnostic.collections.back().messages.back().suggestions.push_back({std::move(message), span, level});
            }

            auto add_patch(DiagnosticLevel level, CowString message, CowString patch_text, Span span, DiagnosticPatchKind patch_kind) -> void {
                dark_assert(m_diagnostic.collections.size() > 0, "Cannot add a patch without a message");
                m_diagnostic.collections.back().messages.back().suggestions.push_back({
                    .message = std::move(message),
                    .span = span,
                    .level = level,
                    .patch_kind = patch_kind,
                    .patch_content = std::move(patch_text)
                });
            }

            template <typename... Args>
            auto add_message(LocT loc, detail::DiagnosticBase<Args...> const& base, Formatter formatter) -> void {
                add_message_with_loc(
                    m_emitter->m_converter->convert_loc(
                        loc,
                        [this](DiagnosticLocation context_loc, detail::DiagnosticBase<> const& context_base) {
                            add_message_with_loc(context_loc, context_base, Formatter(context_base.format));
                        }
                    ),
                    base,
                    std::move(formatter)
                );
            }

            template <typename... Args>
            auto add_message_with_loc(DiagnosticLocation location, detail::DiagnosticBase<Args...> const& base, Formatter formatter) -> void {
                m_diagnostic.collections.emplace_back(
                    DiagnosticMessageCollection {
                        .kind = base.kind,
                        .level = base.level,
                        .formatter = std::move(formatter),
                        .messages = {
                            DiagnosticMessage {
                                .location = location,
                                .suggestions = {},
                            }
                        }
                    }
                );
            }

            auto add_child_context(DiagnosticLevel level, CowString message) -> void {
                dark_assert(m_diagnostic.collections.size() > 0, "Cannot add a child message without a parent message");
                m_diagnostic.collections.back().contexts.emplace_back(DiagnosticMessageContext {
                    .message = std::move(message),
                    .level = level
                });
            }
        private:
            DiagnosticEmitter<LocT>* m_emitter;
            Diagnostic m_diagnostic;
        };

        explicit DiagnosticEmitter(DiagnosticConverter<LocT>& converter, DiagnosticConsumer& consumer) noexcept
            : m_converter(&converter)
            , m_consumer(&consumer)
        {}

        template<typename... Args, typename... Ts>
            requires (sizeof...(Args) == sizeof...(Ts) &&
                (... && detail::is_constructable_to_format_args<Args>::value) &&
                (... && std::same_as< std::decay_t<std::remove_cvref_t<Args>>, std::decay_t<std::remove_cvref_t<Ts>> >)
            )
        auto emit(LocT loc, detail::DiagnosticBase<Args...> const& base, Ts&&... args) -> void {
            build(loc, base, std::forward<Ts>(args)...)
                .emit();
        }

        template<typename... Args, typename... Ts>
            requires (sizeof...(Args) == sizeof...(Ts) &&
                (... && detail::is_constructable_to_format_args<Args>::value) &&
                (... && std::same_as< std::decay_t<std::remove_cvref_t<Args>>, std::decay_t<std::remove_cvref_t<Ts>> >)
            )
        auto build(LocT loc, detail::DiagnosticBase<Args...> const& base, Ts&&... args) -> DiagnosticBuilder {
            return DiagnosticBuilder(this, loc, base, Formatter(base.format, std::forward<Ts>(args)...));
        }

    private:
        template <typename OtherLocT, typename AnnotateFn>
        friend struct DiagnosticAnnotationScope;

    private:
        DiagnosticConverter<LocT>* m_converter;
        DiagnosticConsumer* m_consumer;
        llvm::SmallVector<llvm::function_ref<void(DiagnosticBuilder&)>> m_annotations;
    };

    template <typename LocT>
    DiagnosticEmitter(DiagnosticConverter<LocT>& converter, DiagnosticConsumer& consumer) noexcept -> DiagnosticEmitter<LocT>;

    template <typename LocT, typename AnnotateFn>
    struct DiagnosticAnnotationScope {
        DiagnosticAnnotationScope(DiagnosticEmitter<LocT>& emitter, AnnotateFn annotate)
            : m_emitter(&emitter)
        {
            m_emitter->m_annotations.push_back(annotate);
        }

        ~DiagnosticAnnotationScope() {
            m_emitter->m_annotations.pop_back();
        }
    private:
        DiagnosticEmitter<LocT>* m_emitter;
        AnnotateFn m_annotate;
    };

    template <typename LocT, typename AnnotateFn>
    DiagnosticAnnotationScope(DiagnosticEmitter<LocT>& emitter, AnnotateFn annotate) -> DiagnosticAnnotationScope<LocT, AnnotateFn>;
}

#endif // __DARK_DIAGNOSTIC_EMITTER_HPP__
