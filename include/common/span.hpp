#ifndef __DARK_COMMON_SPAN_HPP__
#define __DARK_COMMON_SPAN_HPP__

#include <cstddef>
#include <llvm/Support/raw_ostream.h>
#include <ostream>

namespace dark {

    namespace detail {
        constexpr auto min(unsigned a, unsigned b) noexcept {
            return a < b ? a : b;
        }
        constexpr auto max(unsigned a, unsigned b) noexcept {
            return a < b ? b : a;
        }
    }
    
    struct Span {
        constexpr Span() = default;
        constexpr Span(Span const&) = default;
        constexpr Span(Span&&) = default;
        constexpr Span& operator=(Span const&) = default;
        constexpr Span& operator=(Span&&) = default;
        constexpr ~Span() = default;

        constexpr Span(unsigned start, unsigned end, bool is_relative = false)
            : m_start(start)
            , m_size(std::max(start, end) - start)
            , m_is_relative(is_relative)
        {}

        constexpr static Span from_size(unsigned start, unsigned size) noexcept {
            return Span{start, start + size};
        }

        [[nodiscard]] constexpr unsigned start() const noexcept { return m_start + m_shift; }
        [[nodiscard]] constexpr unsigned end() const noexcept { return start() + size(); }
        [[nodiscard]] constexpr unsigned size() const noexcept { return m_size; }
        [[nodiscard]] constexpr auto set_size(unsigned size) const noexcept -> Span {
            auto self = *this;
            self.m_size = size;
            return self;
        }
        [[nodiscard]] constexpr auto set_start(unsigned start) const noexcept -> Span {
            auto self = *this;
            self.m_start = start;
            return self;
        }
        [[nodiscard]] constexpr auto set_end(unsigned end) const noexcept -> Span {
            auto self = *this;
            self.m_size = end - m_start;
            return self;
        }

        constexpr unsigned operator[](unsigned index) const noexcept { return m_start + index; }
        constexpr void set_offset(std::ptrdiff_t offset) noexcept {
            auto const temp_start = static_cast<std::ptrdiff_t>(m_start + m_shift);
            auto const new_start = static_cast<unsigned>(std::max(temp_start + offset, 0l));
            *this = Span::from_size(new_start, m_size);
        }

        constexpr Span subspan(unsigned start, unsigned end) const noexcept {
            auto const new_start = detail::min(m_start + start, this->end());
            auto const new_end = detail::min(m_start + end, this->end());
            if (new_start >= new_end) {
                return Span{0, 0};
            }
            return Span{new_start, new_end};
        }

        constexpr bool empty() const noexcept { return m_size == 0; }
        constexpr bool is_valid() const noexcept { return !(empty() && (start() == 0)); }

        constexpr auto contains(Span const& other, bool inclusive = true) const noexcept -> bool {
            if (inclusive) {
                return start() <= other.start() && end() >= other.end();
            }
            return start() < other.start() && end() > other.end();
        }

        constexpr auto outside_of(Span const& other) const noexcept -> bool {
            return start() >= other.end() || end() <= other.start();
        }

        // Assumption: Self is less than other
        constexpr auto split_if_intersect(Span const& other) const noexcept -> std::pair<Span, Span> {
            if (empty() && (start() == other.start())) {
                return { other, Span{} };
            }

            if (other.empty() && end() == other.end()) {
                return { *this, Span{} };
            }

            if (outside_of(other)) {
                return { *this, other };
            }

            if (contains(other)) {
                return { *this, Span{} };
            }

            if (other.contains(*this)) {
                return { other, Span{} };
            }

            // Case 1:
            // |-----|
            //    |-----|

            if (start() <= other.start()) {
                return {
                    *this,
                    Span(end(), other.end())
                };
            }

            // Case 2:
            //    |-----|
            // |-----|
            return {
                Span(other.start(), start()),
                Span(start(), end())
            };
        }

        friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, Span const& span) {
            return os << "Span(" << span.start() << ", " << span.end() <<", " << span.size() << ")";
        }

        friend std::ostream& operator<<(std::ostream& os, Span const& span) {
            return os << "Span(" << span.start() << ", " << span.end() <<", " << span.size() << ")";
        }

        constexpr auto to_relative() const noexcept -> Span {
            auto temp = *this;
            temp.m_is_relative = true;
            return temp;
        }

        constexpr auto to_absolute() const noexcept -> Span {
            auto temp = *this;
            temp.m_is_relative = false;
            return temp;
        }

        constexpr auto is_relative() const noexcept -> bool {
            return m_is_relative;
        }

        constexpr auto shift() const noexcept -> unsigned {
            return m_shift;
        }

        [[nodiscard]] constexpr auto set_shift(unsigned shift) const noexcept -> Span {
            auto self = *this;
            self.m_shift = shift;
            return self;
        }
        
        constexpr auto set_shift(unsigned shift) noexcept -> Span& {
            m_shift = shift;
            return *this;
        }

        [[nodiscard]] constexpr auto raw() const noexcept -> Span {
            return Span{m_start, m_start + m_size};
        }
    private:
        unsigned m_start{0};
        unsigned m_size{0};
        unsigned m_shift{0};
        bool m_is_relative{false};
    };
}

#endif // __DARK_COMMON_SPAN_HPP__