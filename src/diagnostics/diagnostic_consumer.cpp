#include "diagnostics/diagnostic_consumer.hpp"
#include "common/assert.hpp"
#include "common/cow.hpp"
#include "common/format.hpp"
#include "common/span.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include <cstddef>
#include <cstdint>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <numeric>
#include <span>
#include <string_view>
#include <algorithm>
#include <tuple>
#include <vector>

namespace dark {
    inline static constexpr auto to_string(DiagnosticLevel level) -> llvm::StringRef {
        switch (level) {
            case DiagnosticLevel::Error:
                return "error";
            case DiagnosticLevel::Note:
                return "note";
            case DiagnosticLevel::Warning:
                return "warning";
            case DiagnosticLevel::Info:
                return "info";
        }
    }

    inline static constexpr auto get_color(DiagnosticLevel level) -> llvm::raw_ostream::Colors {
        switch (level) {
            case DiagnosticLevel::Error:
                return llvm::raw_ostream::Colors::RED;
            case DiagnosticLevel::Note:
                return llvm::raw_ostream::Colors::BLUE;
            case DiagnosticLevel::Warning:
                return llvm::raw_ostream::Colors::YELLOW;
            case DiagnosticLevel::Info:
                return llvm::raw_ostream::Colors::GREEN;
        }
    }

    inline static constexpr auto get_digits(unsigned number) -> unsigned {
        unsigned digits = 0;
        while (number) {
            number /= 10;
            digits++;
        }
        return digits;
    }

    inline static constexpr auto get_max_line_number_width(DiagnosticMessageCollection& diagnostic) -> unsigned {
        unsigned max = 0;
        for (auto const& message : diagnostic.messages) {
            max = std::max(max, get_digits(message.location.line_number));
        }
        return max;
    }

    struct NormalizedDiagnosticMessageContext {
        Span span;
        DiagnosticLevel level;
        DiagnosticPatchKind patch_kind;
        llvm::SmallVector<unsigned> ids;
    };

    inline static auto fix_and_construct_unique_sorted_span(DiagnosticMessage& message) -> llvm::SmallVector<NormalizedDiagnosticMessageContext> {
        llvm::SmallVector<NormalizedDiagnosticMessageContext> unique_spans;
        if (message.suggestions.empty() || message.location.line.empty()) return unique_spans;

        auto temp_suggestions = message.suggestions;

        if (message.suggestions.size() > 2) {
            bool no_overlap = false;
            // loop until there is no overlap.
            while (!no_overlap) {
                no_overlap = true;
                std::sort(message.suggestions.begin(), message.suggestions.end(), [](auto const& lhs, auto const& rhs) {
                    return std::tuple(lhs.span.start(), lhs.span.end(), lhs.level) < std::tuple(rhs.span.start(), rhs.span.end(), rhs.level);
                });

                for (auto i = 0ul; i < message.suggestions.size() - 1; ++i) {
                    auto const& lhs = message.suggestions[i];
                    auto const& rhs = message.suggestions[i + 1];
                    if (lhs.patch_kind == DiagnosticPatchKind::Insert || rhs.patch_kind == DiagnosticPatchKind::Insert) {
                        continue;
                    }

                    auto const raw_lhs = lhs.span.raw();
                    auto const raw_rhs = rhs.span.raw();
                    
                    // llvm::outs() << "LHS: " << lhs.span << ", RHS: " << rhs.span << '\n';
                    // llvm::outs() << "LHS: " << to_string(lhs.level) << ", RHS: " << to_string(rhs.level) << " = " << (lhs.level < rhs.level) << '\n';
                    // Higher the level, the less important it is.
                    no_overlap = lhs.span.outside_of(rhs.span) && no_overlap;

                    if (lhs.level <= rhs.level) {
                        auto const new_lhs = Span(raw_lhs.start(), raw_lhs.end()).set_shift(lhs.span.shift());
                        auto const new_rhs = Span(std::max(raw_rhs.start(), raw_lhs.end()), raw_rhs.end()).set_shift(rhs.span.shift());
                        message.suggestions[i].span = new_lhs;
                        message.suggestions[i + 1].span = new_rhs;
                    } else {
                        auto const new_lhs = Span(raw_lhs.start(), std::min(raw_lhs.end(), raw_rhs.start())).set_shift(lhs.span.shift());
                        auto const new_rhs = Span(raw_rhs.start(), raw_rhs.end()).set_shift(rhs.span.shift());
                        message.suggestions[i].span = new_lhs;
                        message.suggestions[i + 1].span = new_rhs;
                    }
                }
            }
        }

        // llvm::outs() << "After sorting and spliting: \n";
        // for (auto const& el: message.suggestions) {
        //     llvm::outs() << el.span << '\n';
        // }

        unique_spans.push_back({ message.suggestions[0].span, message.suggestions[0].level, message.suggestions[0].patch_kind, { 0 } });

        for (auto i = 1ul; i < message.suggestions.size(); ++i) {
            auto const& el = message.suggestions[i];
            if (unique_spans.empty()) {
                unique_spans.push_back({ el.span, el.level, el.patch_kind, { static_cast<unsigned>(i) } });
                continue;
            }
            auto top = unique_spans.pop_back_val();
            auto const [lhs, rhs] = top.span.split_if_intersect(el.span);
            auto level = std::min(top.level, el.level);

            // llvm::outs() << "LHS: " << lhs << ", RHS: " << rhs << '\n';

            auto const is_lhs_valid = lhs.is_valid();
            auto const is_rhs_valid = rhs.is_valid();

            // llvm::outs() << "LHS Valid: " << is_lhs_valid << ", RHS Valid: " << is_rhs_valid << '\n';

            if (is_lhs_valid && is_rhs_valid) {
                unique_spans.push_back({ lhs, top.level, top.patch_kind, top.ids });
                unique_spans.push_back({ rhs, el.level, el.patch_kind, { static_cast<unsigned>(i) } });
            } else if (is_lhs_valid) {
                top.span = lhs;
                top.level = level;
                top.ids.push_back(static_cast<unsigned>(i));
                unique_spans.push_back(top);
            } else {
                top.span = rhs;
                top.level = level;
                top.ids.push_back(static_cast<unsigned>(i));
                unique_spans.push_back(top);
            }
        }

        // llvm::outs() << "After merging: \n";
        // for (auto const& el: unique_spans) {
        //     llvm::outs() << el.span << ", Count: " << el.ids.size() << ", Level: " << to_string(el.level) << '\n';
        // }

        return unique_spans;
    }

    inline static auto print_line_number(llvm::raw_ostream& os, unsigned line_number, unsigned max_line_number_width, llvm::raw_ostream::Colors color = llvm::raw_ostream::Colors::MAGENTA) -> void {
        os.changeColor(color);
        if (line_number == 0) {
            os.indent(max_line_number_width) << " | ";
            os.resetColor();
            return;
        }
        os.indent(max_line_number_width - get_digits(line_number)).changeColor(color, true) << line_number;
        os.changeColor(color) << " | ";
        os.resetColor();
    }

    inline static auto append_escaping_sequence(llvm::SmallString<256>& out, llvm::SmallVector<std::uint8_t, 256>& positions, llvm::StringRef str) -> void {
        for (auto i = 0zu; auto c : str) {
            switch (c) {
                case '\n':
                    out.append("\\n");
                    positions[i] = 1;
                    break;
                case '\r':
                    out.append("\\r");
                    positions[i] = 1;
                    break;
                case '\t':
                    out.append("\\t");
                    positions[i] = 1;
                    break;
                default:
                    out.push_back(c);
                    break;
            }
            ++i;
        }
    }

    inline static auto escaped_string(llvm::StringRef str) -> std::pair<CowString, llvm::SmallVector<std::uint8_t, 256>> {
        auto count = static_cast<std::size_t>(std::count_if(str.begin(), str.end(), [](auto c) {
            return c == '\n' || c == '\r' || c == '\t';
        }));
        if (count == 0) return { make_borrowed(str), {} };
        llvm::SmallString<256> out;
        llvm::SmallVector<std::uint8_t, 256> positions(str.size(), 0);
        out.reserve(str.size() + count * 2);
        append_escaping_sequence(out, positions, str);
        return { make_owned(std::string(out)), positions };
    }

    inline static auto highlight_context(
        llvm::raw_ostream& os, 
        DiagnosticLocation const& location,
        unsigned max_line_number_width,
        llvm::SmallVector<DiagnosticMessageSuggestions> const& suggestions,
        llvm::SmallVector<NormalizedDiagnosticMessageContext>& unique_suggestion_spans,
        NormalizedDiagnosticMessageContext default_context
    ) -> void {
        unsigned last_end = 0;
        llvm::StringRef line = llvm::StringRef(location.line).rtrim();
        print_line_number(os, location.line_number, max_line_number_width);
        
        if (line.empty()) {
            return;
        }

        bool is_error_pushed {false};
        if (unique_suggestion_spans.empty()) {
            if (default_context.span.empty()) {
                os << line << '\n';
                return;
            }
            unique_suggestion_spans.push_back(default_context);
            is_error_pushed = true;
        }

        for (auto const& suggestion : unique_suggestion_spans) {
            auto const span = suggestion.span.raw();
            if (span.empty()) continue;
            auto prefix_span = Span{ last_end, span.start() };
            auto prefix = line.substr(prefix_span.start(), prefix_span.size());
            
            auto highlight = line.substr(span.start(), span.size());

            if (suggestion.patch_kind == DiagnosticPatchKind::Insert) {
                auto const& message = suggestions[suggestion.ids.front()].patch_content;
                highlight = message.borrow();
                last_end = span.start();
            } else {
                last_end = span.end();
            }

            if (!prefix.empty()) {
                os << prefix;
            }

            if (!highlight.empty()) {
                os.changeColor(get_color(suggestion.level), true) << highlight;
                os.resetColor();
            }
        }

        if (last_end < line.size()) {
            os << line.substr(last_end);
        }

        os << '\n';

        print_line_number(os, 0, max_line_number_width);

        last_end = 0;

        for (auto const& suggestion : unique_suggestion_spans) {
            auto const span = suggestion.span;
            if (span.empty()) continue;
            auto indent = span.start() - last_end;
            os.indent(indent).changeColor(get_color(suggestion.level), true);
            if (suggestion.patch_kind == DiagnosticPatchKind::Insert) {
                for (auto i = 0ul; i < span.size(); ++i) {
                    os << '+';
                }
            } else if (suggestion.patch_kind == DiagnosticPatchKind::Remove) {
                for (auto i = 0ul; i < span.size(); ++i) {
                    os << '-';
                }
            } else {
                for (auto i = 0ul; i < span.size(); ++i) {
                    os << (i == 0 ? '^' : '~');
                }
            }
            os.resetColor();
            last_end = span.end();
        }

        os << '\n';

        if (is_error_pushed) {
            unique_suggestion_spans.pop_back();
        }
    }

    struct SuggestionsPositionInfo {
        std::size_t row;
        std::size_t col;
        Span span;
        DiagnosticLevel level;
    };

    struct Cell {
        char c {0};
        llvm::raw_ostream::Colors color{ llvm::raw_ostream::Colors::WHITE };
        bool is_bold {false};
    };

    inline static auto add_span_path(
        std::span<Cell> buffer,
        llvm::SmallVector<SuggestionsPositionInfo>& suggestion_positions,
        std::ptrdiff_t current_row
    ) -> void {
        for (auto& el : suggestion_positions) {
            if (current_row >= static_cast<std::ptrdiff_t>(el.row)) continue;
            auto const start = el.col;
            auto& span = el.span;
            auto const level = el.level;

            auto c = (span.start() == start ? '|' : '/');
            // Case 1: When the suggestions start column is the same as the current column.
            //       xxx
            //       ^^^
            //       |
            //       yyyyyyyyy (message)
            // Case 2: When the suggestions start column is greater than the current column.
            //       xxx
            //       ^^^
            //       /
            //      yyyyyyyyy (message)

            dark_assert(span.start() >= start, Formatter("The span start should be greater than or equal to the start column. {} >= {}", span.start(), start).format());

            bool has_offset_applied = false;
            while(buffer[span.start()].c != 0) {
                if (span.start() == 0) break;
                span.set_offset(-1);
                has_offset_applied = true;
                c = (span.start() == start ? '|' : '/');
            }

            buffer[span.start()] = { .c = c, .color = get_color(level), .is_bold = false };
            if (has_offset_applied && c != '|') {
                span.set_offset(-1);
            }
        }
    }

    template <std::size_t text_padding = 4>
    inline static auto print_suggestions_message(
        llvm::raw_ostream& os,
        unsigned max_line_number_width,
        std::size_t col_count,
        llvm::SmallVector<NormalizedDiagnosticMessageContext>& unique_suggestion_span,
        llvm::SmallVector<DiagnosticMessageSuggestions> const& suggestions
    ) -> void {
        {
            // 1. Find the place inside the cells for text. 
            // | x |   | x |   |
            // |   | x |   |   |
            
            auto const row_count = std::max(suggestions.size(), 20ul) + 1;
            llvm::SmallVector<Cell, 100 * 21> cells(row_count * col_count, Cell{});

            auto const get_cell = [&cells, col_count](std::size_t row, std::size_t col) -> Cell& {
                return cells[row * col_count + col];
            };

            auto put_list_index = [&get_cell](std::size_t row, std::size_t col, DiagnosticLevel level) -> void {
                get_cell(row, col + 0) = { .c = '|', .color = get_color(level), .is_bold = false };
                get_cell(row, col + 1) = { .c = '-', .color = get_color(level), .is_bold = false };
            };

            auto max_line_index = 0zu;
            auto has_not_outputted_lines = false;
            auto col_start = col_count - 1;

            llvm::SmallVector<SuggestionsPositionInfo> suggestions_positions;
            suggestions_positions.reserve(unique_suggestion_span.size());
            auto line_index = 0zu;

            while (!has_not_outputted_lines) {
                has_not_outputted_lines = false;
                for (auto it = unique_suggestion_span.rbegin(); it != unique_suggestion_span.rend(); ++it) {
                    auto& el = *it;
                    // Remove the suggestions that have empty messages.
                    auto remove_it = std::remove_if(el.ids.begin(), el.ids.end(), [&suggestions](auto const& i) {
                        auto const& el = suggestions[i];
                        return el.message.borrow().empty();
                    });
                    auto const ids_count = static_cast<std::size_t>(std::distance(el.ids.begin(), remove_it));
                    if (ids_count == 0) continue;

                    auto first = suggestions[el.ids.front()];

                    has_not_outputted_lines = true;
                    auto remaining = el.ids.size() - 1;
                    auto first_text = first.message.borrow();
                    // auto const max_width = get_digits(static_cast<unsigned>(el.ids.size()));

                    col_start = first.span.start();

                    // Count the number of rows needed for the remaining suggestions if first is chosen.

                    auto collision = true;
                    while (collision) {
                        collision = false;
                        auto const end = std::min(first_text.size() + text_padding + col_start, col_count);

                        for (auto i = col_start; i < end; ++i) {
                            if (get_cell(line_index, i).c != 0) {
                                line_index++;
                                collision = true;
                                break;
                            }
                        }
                    }

                    dark_assert(line_index < row_count, "The row index should be less than the row count.");

                    // If the remaining suggestions are not able to fit in the row, then move the row_start to the right.
                    auto total_suggestion_that_can_fit = 0zu;
                    auto second_last_col = [&unique_suggestion_span, it=it] () -> unsigned {
                        if (it == unique_suggestion_span.rend()) return 0ul;
                        auto nit = std::next(it);
                        if (nit == unique_suggestion_span.rend()) return 0ul;
                        return (*nit).span.start();
                    }();

                    while (true) {
                        if (total_suggestion_that_can_fit > remaining) {
                            total_suggestion_that_can_fit -= 1;
                            break;
                        }
                        if (col_start < (total_suggestion_that_can_fit * 2)) {
                            break;
                        }

                        total_suggestion_that_can_fit++;
                    }

                    bool need_list = false;
                    auto const diff = col_start - second_last_col;
                    if (remaining != 0) {
                        need_list = (total_suggestion_that_can_fit > remaining) || (diff <= total_suggestion_that_can_fit * 2);
                        // llvm::outs() << "(total_suggestion_that_can_fit > remaining) => " << (total_suggestion_that_can_fit > remaining) << '\n';
                        // llvm::outs() << "(diff < total_suggestion_that_can_fit * 2) => " << (diff < total_suggestion_that_can_fit * 2) << '\n';
                    }

                    // llvm::outs() << "Total suggestion that can fit: " << total_suggestion_that_can_fit << "; diff: " << diff << '\n';
                    
                    suggestions_positions.push_back({ line_index, col_start, first.span, first.level });

                    if (need_list) {
                        // Print the list item index.
                        put_list_index(line_index, col_start, first.level);
                    }
                    
                    {
                        auto const temp_col_start = col_start + 2 * static_cast<unsigned>(need_list);
                        // Print the first suggestion.
                        auto const text_to_print = first_text.substr(0, std::min(first_text.size(), col_count - col_start - 1));
                        for (auto i = 0ul; i < text_to_print.size(); ++i) {
                            get_cell(line_index, temp_col_start + i) = {
                                .c = text_to_print[i],
                                .color = get_color(first.level),
                                .is_bold = false
                            };
                        }
                    }

                    for (auto col = 1zu; col < ids_count; ++col) {
                        auto const idx = el.ids[col];
                        auto const& suggestion = suggestions[idx];
                        // auto const span = suggestion.span;
                        auto text = suggestion.message.borrow();
                        if (text.empty()) continue;
                        auto current_col = col_start;
                        // llvm::outs() << "Span: " << suggestion.span << '\n';
                        // llvm::outs() << "Col Start: " << col_start << '\n';

                        if (need_list) {
                            put_list_index(line_index + col, col_start, suggestion.level);
                            current_col += 2;
                        } else {
                            col_start -= 2; // Shift the column to the left by 2 since the previous text will be above it.
                            current_col = col_start;
                            suggestions_positions.push_back({ line_index + col, current_col, suggestion.span, suggestion.level });
                        }

                        // llvm::outs() << "Current Col: " << current_col << '\n';

                        auto const text_to_print = text.substr(0, std::min(text.size(), col_count - current_col - 1));
                        for (auto i = 0ul; i < text_to_print.size(); ++i) {
                            get_cell(line_index + col, current_col + i) = {
                                .c = text_to_print[i],
                                .color = get_color(suggestion.level),
                                .is_bold = false
                            };
                        }

                        max_line_index = std::max(max_line_index, line_index + col);
                    }

                    el.ids.clear();
                }

                max_line_index = std::max(max_line_index, line_index);
            }

            {
                auto buffer = std::vector<Cell>(col_count, Cell{});
                auto fist_line = std::span(&get_cell(0, 0), row_count);
                add_span_path(buffer, suggestions_positions, -1);
                print_line_number(os, 0, max_line_number_width);
                std::size_t max_col = 0;
                for (auto col = 0ul; col < col_count; ++col) {
                    if (buffer[col].c != 0) {
                        max_col = col;
                    }
                }

                for (auto i = 0ul; i <= max_col; ++i) {
                    auto const& el = buffer[i];
                    os.changeColor(el.color, el.is_bold) << (el.c == 0 ? ' ' : el.c);
                }
                os << '\n';
            }

            for (auto row = 0ul; row <= std::min(max_line_index, row_count - 1); ++row) {
                std::size_t max_col = 0;
                for (auto col = 0ul; col < col_count; ++col) {
                    if (get_cell(row, col).c != 0) {
                        max_col = col;
                    }
                }

                auto line = std::span(&get_cell(row, 0), max_col + 1);
                add_span_path(line, suggestions_positions, static_cast<std::ptrdiff_t>(row));

                print_line_number(os, 0, max_line_number_width);

                for (auto const& el: line) {
                    char c = (el.c == 0 ? ' ' : el.c);
                    os.changeColor(el.color, el.is_bold) << c;
                }
                os.resetColor();
                os << '\n';
            }   
        }

    }

    auto StreamDiagnosticConsumer::consume(Diagnostic&& diagnostic) -> void {
        using Color = llvm::raw_ostream::Colors;
        // error: expected expression
        //  --> std/std.dark:1:1 
        //   |
        // 1 | auto out = get_stream(diagnostic.level, m_stream);
        //   | ^~~        ^~~~~
        //   | |         /|
        //   | |        | llvm::raw_ostream&
        //   | |       diagnostic
        //   | 'auto' is not allowed in C++98 mode
        // note: to match this '('

        auto& stream = *m_stream;
        if (has_printed) {
            stream << "\n";
        }

        has_printed = true;

        for (auto& collection : diagnostic.collections) {
            auto const max_line_number_width = get_max_line_number_width(collection) + 1;
            if (collection.messages.empty()) continue;
            
            {
                // 1. Show the diagnostic message
                stream.changeColor(get_color(collection.level), true) << to_string(collection.level);
                stream.changeColor(Color::WHITE, true) << ": ";
                stream.resetColor() << collection.formatter.format() << '\n';
            }
            
            {
                // 2. Show the location of the diagnostic
                auto const& message = collection.messages[0];
                if (message.location.can_be_printed()) {
                    stream.changeColor(Color::MAGENTA) << "  --> ";
                    stream.resetColor() << message.location << '\n';
                }
            }


            for (auto& message : collection.messages) {

                auto [fixed_line, offsets] = escaped_string(message.location.line);
                auto old_line = message.location.line;
                message.location.line = fixed_line.borrow();

                Span default_span{};
                {
                    // 3. Fix spans
                    auto len = message.location.length == 0 ? 0 : (message.location.length);
                    len = std::min(len, static_cast<unsigned>(message.location.get_line().size()) - message.location.column_number);
                    
                    default_span = Span( message.location.column_number, message.location.column_number + len );
                    unsigned shift = 0;

                    std::sort(message.suggestions.begin(), message.suggestions.end(), [](auto const& lhs, auto const& rhs) {
                        if (lhs.span.start() == rhs.span.start()) {
                            return (lhs.patch_kind >= rhs.patch_kind);
                        }
                        return (lhs.span.start() < rhs.span.start());
                    });
                    
                    for (auto& suggestions: message.suggestions) {
                        if (suggestions.span.empty()) {
                            suggestions.span = default_span;
                            continue;
                        }

                        if (suggestions.span.is_relative()) {
                            suggestions.span.set_offset(static_cast<std::ptrdiff_t>(message.location.column_number));
                        }

                        if (!offsets.empty()) {
                            auto start = suggestions.span.raw().start();
                            auto offset = std::accumulate(offsets.begin(), offsets.begin() + start, 0u);
                            auto size = suggestions.span.size();
                            auto old_shift = suggestions.span.shift();

                            start += offset;

                            if (offsets[start]) start += offsets[start];

                            for (auto i = start; i < start + size; ++i) {
                                if (offsets[i]) {
                                    size += offsets[start];
                                }
                            }

                            suggestions.span = Span::from_size(start, size).set_shift(old_shift);
                        }

                        if (suggestions.patch_kind == DiagnosticPatchKind::Insert) {
                            auto const size = static_cast<unsigned>(suggestions.patch_content.borrow().size());
                            suggestions.span = Span::from_size(suggestions.span.start(), size);
                            suggestions.span.set_shift(shift);
                            shift += size;
                        } else {
                            suggestions.span.set_shift(shift);
                        }
                    }
                }

                auto unique_suggestion_spans = fix_and_construct_unique_sorted_span(message);
                if (!message.location.line.empty()) {
                    // 4. Show the line of the diagnostic
                    highlight_context(stream, message.location, max_line_number_width, message.suggestions, unique_suggestion_spans, {
                        .span = default_span,
                        .level = collection.level,
                        .ids = {}
                    });
                }

                if (!unique_suggestion_spans.empty()) {
                    auto const last_span = unique_suggestion_spans.back().span;
                    // | ----- Source Line ------ |
                    //                   | ----- Last Span -------------- |
                    //                            | ----- Extra Space ----- |
                    auto const line_size = static_cast<unsigned>(message.location.line.size());
                    auto const [_lhs, rhs] = Span{ 0, line_size }.split_if_intersect(last_span);

                    auto const extra_space = (rhs.empty() ? last_span.size() : rhs.size()) + 10;
                    auto const col_count = std::max(message.location.line.size() + extra_space, 100ul);
                    // 4. Show the diagnostic context
                    print_suggestions_message(stream, max_line_number_width, col_count, unique_suggestion_spans, message.suggestions);
                }
                
                message.location.line = old_line;
            }
            
            {
                // 5. Print all the context
                for (auto const& context : collection.contexts) {
                    stream.changeColor(get_color(context.level), true) << to_string(context.level);
                    stream.changeColor(Color::WHITE, true) << ": ";
                    stream.resetColor() << context.message.borrow() << '\n';
                }
            }
        }
    }

    auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer& {
        static auto* console = new StreamDiagnosticConsumer{ llvm::errs() };
        return *console;
    }

} // namespace dark