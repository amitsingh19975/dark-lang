#ifndef __DARK_ADT_RANGE_HPP__
#define __DARK_ADT_RANGE_HPP__

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>

namespace dark {

    template <std::integral T, std::size_t N>
    struct BasicRange;

    namespace detail {
        template <std::integral T, std::size_t N, T D>
        struct RangeArg {
            using type = T;
            static constexpr std::size_t index{N};

            T value{D};
        };

        template <typename T>
        struct is_range_arg : std::false_type {};

        template <std::integral T, std::size_t N, T D>
        struct is_range_arg<RangeArg<T, N, D>> : std::true_type {};

        template <typename T>
        concept IsRangeArg = is_range_arg<std::decay_t<std::remove_cvref_t<T>>>::value;

        template <std::integral T>
        struct RangeArgPack {
            T start{};
            T end{};
            T step{};
        };

        template <std::integral T, IsRangeArg... Args>
            requires ((sizeof...(Args) <= 3) && (... && std::same_as<typename Args::type, T>))
        [[nodiscard]] constexpr auto make_arg_pack(Args&&... args) noexcept -> RangeArgPack<T> {
            std::array<std::optional<T>, 3> values{std::nullopt};
            ((values[args.index] = args.value), ...);

            RangeArgPack<T> pack;
            if (values[0].has_value()) pack.start = values[0].value();
            else pack.start = T{};

            if (values[1].has_value()) pack.end = values[1].value();
            else pack.end = T{};

            if (values[2].has_value()) pack.step = values[2].value();
            else pack.step = T{1};

            return pack;
        }

    } // namespace detail

    template <std::integral T>
    struct BasicRange<T, 0> {
        using start_t = detail::RangeArg<T, 0, T{}>;
        using end_t = detail::RangeArg<T, 1, T{}>;
        using step_t = detail::RangeArg<T, 2, T{1}>;
        
        struct Iterator;
        
        using value_type = T;
        using difference_type = T;
        using reference = T&;
        using const_reference = T const&;
        using pointer = T*;
        using const_pointer = T const*;
        using size_type = T;
        using iterator = Iterator;
        using const_iterator = Iterator;
        using const_reverse_iterator = std::reverse_iterator<Iterator>;
        using reverse_iterator = std::reverse_iterator<Iterator>;

        static_assert(std::is_unsigned_v<T>, "Range type must be an unsigned integral type.");

        constexpr BasicRange() noexcept = default;
        constexpr BasicRange(BasicRange const&) noexcept = default;
        constexpr BasicRange(BasicRange&&) noexcept = default;
        constexpr BasicRange& operator=(BasicRange const&) noexcept = default;
        constexpr BasicRange& operator=(BasicRange&&) noexcept = default;
        constexpr ~BasicRange() noexcept = default;

        template <detail::IsRangeArg... Args>
            requires ((sizeof...(Args) <= 3) && sizeof...(Args) > 0)
        constexpr BasicRange(Args&&... args) noexcept
            : BasicRange(detail::make_arg_pack<T>(std::forward<Args>(args)...))
        {}

        constexpr BasicRange(T start, T end, T step = T{1}) noexcept
            : m_start(std::min(start, end))
            , m_end(std::max(start, end))
            , m_step(std::max(step, T{1}))
        {
            auto s = static_cast<T>(size());
            if (std::numeric_limits<T>::max() - m_start < s * static_cast<T>(m_step)) {
                m_end = std::numeric_limits<T>::max();
            } else {
                m_end = m_start + s * static_cast<T>(m_step);
            }
        }

        constexpr BasicRange(T start) noexcept
            : BasicRange(start, start + 1, T{1})
        {}

        [[nodiscard]] constexpr auto size() const noexcept -> size_type {
            return static_cast<size_type>((m_end - m_start) / m_step);
        }

        [[nodiscard]] constexpr auto start() const noexcept -> T {
            return m_start;
        }

        [[nodiscard]] constexpr auto last() const noexcept -> T {
            return m_end;
        }

        [[nodiscard]] constexpr auto step() const noexcept -> T {
            return m_step;
        }

        [[nodiscard]] constexpr auto operator[](T index) const noexcept -> T {
            return start() + index * last();
        }

        [[nodiscard]] constexpr auto contains(T value, bool inclusive_end) const noexcept -> bool {
            if (inclusive_end) return (value >= start() && value <= last());
            else return (value >= start() && value < last());
        }

        [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
            return const_iterator(*this, start());
        }

        [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
            return const_iterator(*this, last());
        }

        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator {
            return reverse_iterator(end());
        }

        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator {
            return reverse_iterator(begin());
        }

    private:
        constexpr BasicRange(detail::RangeArgPack<T> pack) noexcept
            : BasicRange(pack.start, pack.end, pack.step)
        {}

    private:
        T m_start{};
        T m_end{};
        T m_step{};
    };

    template <std::integral T, std::size_t N>
    struct BasicRange {
        using start_t = detail::RangeArg<T, 0, T{}>;
        using end_t = detail::RangeArg<T, 1, T{}>;
        
        using value_type = T;
        using difference_type = T;
        using reference = T&;
        using const_reference = T const&;
        using pointer = T*;
        using const_pointer = T const*;
        using size_type = T;
        using iterator = BasicRange<T, 0>::Iterator;
        using const_iterator = iterator const;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = std::reverse_iterator<iterator>;

        constexpr BasicRange() noexcept = default;
        constexpr BasicRange(BasicRange const&) noexcept = default;
        constexpr BasicRange(BasicRange&&) noexcept = default;
        constexpr BasicRange& operator=(BasicRange const&) noexcept = default;
        constexpr BasicRange& operator=(BasicRange&&) noexcept = default;
        constexpr ~BasicRange() noexcept = default;

        template <detail::IsRangeArg... Args>
            requires ((sizeof...(Args) <= 2) && sizeof...(Args) > 0)
        constexpr BasicRange(Args&&... args) noexcept
            : BasicRange(detail::make_arg_pack<T>(std::forward<Args>(args)...))
        {}

        constexpr BasicRange(T start, T end) noexcept
            : m_start(std::min(start, end))
            , m_end(std::max(start, end))
        {
            auto s = static_cast<T>(size());
            if (std::numeric_limits<T>::max() - m_start < s * step()) {
                m_end = std::numeric_limits<T>::max();
            } else {
                m_end = m_start + s * step();
            }
        }

        constexpr BasicRange(T start) noexcept
            : BasicRange(start, start + 1)
        {}

        [[nodiscard]] constexpr auto size() const noexcept -> size_type {
            return static_cast<size_type>(m_end - m_start) / N;
        }

        [[nodiscard]] constexpr auto start() const noexcept -> T {
            return m_start;
        }

        [[nodiscard]] constexpr auto last() const noexcept -> T {
            return m_end;
        }

        [[nodiscard]] constexpr auto step() const noexcept -> T {
            return static_cast<T>(N);
        }

        [[nodiscard]] constexpr auto operator[](T index) const noexcept -> T {
            return start() + index * last();
        }

        [[nodiscard]] constexpr auto contains(T value, bool inclusive_end) const noexcept -> bool {
            if (inclusive_end) return (value >= start() && value <= last());
            else return (value >= start() && value < last());
        }

        [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
            return const_iterator(*this, start());
        }

        [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
            return const_iterator(*this, last());
        }

        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator {
            return reverse_iterator(end());
        }

        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator {
            return reverse_iterator(begin());
        }

        constexpr operator BasicRange<T, 0>() const noexcept {
            return BasicRange<T, 0>(start(), last(), N);
        }

    private:
        constexpr BasicRange(detail::RangeArgPack<T> pack) noexcept
            : BasicRange(pack.start, pack.end)
        {}

    private:
        T m_start{};
        T m_end{};
    };

    template <std::integral T>
    struct BasicRange<T, 0>::Iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = T;
        using pointer = T*;
        using reference = T&;

        constexpr Iterator() noexcept = default;
        constexpr Iterator(Iterator const&) noexcept = default;
        constexpr Iterator(Iterator&&) noexcept = default;
        constexpr Iterator& operator=(Iterator const&) noexcept = default;
        constexpr Iterator& operator=(Iterator&&) noexcept = default;
        constexpr ~Iterator() noexcept = default;

        constexpr Iterator(BasicRange const& range, T current) noexcept
            : m_current(current)
            , m_range(range)
        {}

        [[nodiscard]] constexpr auto operator*() const noexcept -> T {
            return m_current;
        }

        constexpr auto operator++() noexcept -> Iterator& {
            m_current += m_range.step();
            return *this;
        }

        constexpr auto operator++(int) noexcept -> Iterator {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        constexpr auto operator--() noexcept -> Iterator& {
            m_current -= m_range.step();
            return *this;
        }

        constexpr auto operator--(int) noexcept -> Iterator {
            auto copy = *this;
            --(*this);
            return copy;
        }

        constexpr auto operator+=(T n) noexcept -> Iterator& {
            m_current += n * m_range.step();
            return *this;
        }

        constexpr auto operator-=(T n) noexcept -> Iterator& {
            m_current -= n * m_range.step();
            return *this;
        }

        [[nodiscard]] constexpr auto operator+(T n) const noexcept -> Iterator {
            auto copy = *this;
            copy += n;
            return copy;
        }

        [[nodiscard]] constexpr auto operator-(T n) const noexcept -> Iterator {
            auto copy = *this;
            copy -= n;
            return copy;
        }

        [[nodiscard]] constexpr auto operator-(Iterator const& other) const noexcept -> T {
            return (m_current - other.m_current);
        }

        [[nodiscard]] constexpr auto operator<=>(Iterator const& other) const noexcept -> std::strong_ordering {
            return m_current <=> other.m_current;
        }

        [[nodiscard]] constexpr auto operator==(Iterator const& other) const noexcept -> bool {
            return m_current == other.m_current;
        }

        [[nodiscard]] constexpr auto operator!=(Iterator const& other) const noexcept -> bool {
            return !(*this == other);
        }

        [[nodiscard]] constexpr auto operator[](T n) const noexcept -> T {
            return m_current + n * m_range.step();
        }

    private:
        T m_current{};
        BasicRange m_range;
    };

    using Range = BasicRange<std::size_t, 0>;
    using SimpleRange = BasicRange<std::size_t, 1>;
} // namespace dark

#endif // __DARK_ADT_RANGE_HPP__