#ifndef __DARK_COMMON_COW_HPP__
#define __DARK_COMMON_COW_HPP__

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
namespace dark {

    struct borrowed_tag {};
    struct owned_tag {};

    template<typename Borrowed, typename Owned>
    struct Cow {
        using borrowed_type = Borrowed;
        using owned_type = Owned;


        static_assert(!std::is_same_v<borrowed_type, owned_type>, "Borrowed and owned types must be different");
        static_assert(std::is_trivially_destructible_v<borrowed_type>, "Borrowed type must be trivially destructible");
        static_assert(std::is_constructible_v<borrowed_type, owned_type> || std::is_convertible_v<owned_type, borrowed_type>, "Owned type must be constructible or convertible to borrowed type");
        static_assert(std::is_constructible_v<owned_type, borrowed_type> || std::is_convertible_v<borrowed_type, owned_type>, "Borrowed type must be constructible or convertible to owned type");

        constexpr explicit Cow(borrowed_type value)
            : m_data(value) 
        {}
        
        constexpr explicit Cow(owned_type value)
            : m_data(std::move(value)) 
        {}

        constexpr explicit Cow() requires std::is_same_v<std::string_view, borrowed_type>
            : Cow(borrowed_type{})
        {}

        template <typename T>
        constexpr Cow(T&& v, owned_tag) requires std::is_constructible_v<owned_type, char const*>
            : m_data(owned_type(std::forward<T>(v))) {}
        
        template <typename T>
        constexpr Cow(T&& v, borrowed_tag) requires std::is_constructible_v<borrowed_type, char const*>
            : m_data(borrowed_type(std::forward<T>(v))) {}
        
        constexpr Cow(Cow const& other) noexcept(std::is_nothrow_copy_constructible_v<borrowed_type>)
            : m_data(other.borrow())
        {}
        constexpr Cow(Cow&& other) noexcept(std::is_nothrow_move_constructible_v<owned_type>) = default;
        constexpr Cow& operator=(Cow const& other) noexcept(std::is_nothrow_copy_assignable_v<owned_type>) {
            if (this == &other) return *this;
            m_data = other.borrow();
            return *this;
        };
        constexpr Cow& operator=(Cow&& other) noexcept(std::is_nothrow_move_assignable_v<owned_type>) = default;
        constexpr ~Cow() noexcept(std::is_nothrow_destructible_v<owned_type>) = default;

        constexpr auto borrow() const noexcept(std::is_nothrow_copy_constructible_v<borrowed_type>) -> borrowed_type const {
            return std::visit([](auto const& value) -> borrowed_type {
                return borrowed_type(value);
            }, m_data);
        }

        constexpr auto own() noexcept(std::is_nothrow_move_constructible_v<owned_type>) -> owned_type {
            return std::visit([]<typename T>(T& value) -> owned_type {
                if constexpr (std::is_same_v<T, owned_type>) {
                    return std::move(value);
                } else {
                    return owned_tag(value);
                }
            }, m_data);
        }

        template <typename ...Args>
            requires std::is_constructible_v<borrowed_type, Args...>
        static constexpr auto make_borrowed(Args&&... args) noexcept(std::is_nothrow_constructible_v<borrowed_type, Args...>) -> Cow {
            return Cow(borrowed_type(std::forward<Args>(args)... ));
        }

        template <typename ...Args>
            requires std::is_constructible_v<owned_type, Args...>
        static constexpr auto make_owned(Args&&... args) noexcept(std::is_nothrow_constructible_v<owned_type, Args...>) -> Cow {
            return Cow(owned_type(std::forward<Args>(args)... ));
        }

        constexpr auto operator==(Cow const& other) const noexcept -> bool {
            return borrow() == other.borrow();
        }

        constexpr auto operator!=(Cow const& other) const noexcept -> bool {
            return borrow() != other.borrow();
        }

    private:
        std::variant<Borrowed, Owned> m_data;
    };

    using CowString = Cow<std::string_view, std::string>;

    inline static auto make_borrowed(std::string_view value) -> CowString {
        return CowString::make_borrowed(value);
    }

    inline static auto make_owned(std::string value) -> CowString {
        return CowString::make_owned(std::move(value));
    }

    namespace literal {
        inline static auto operator""_cow(char const* str, std::size_t len) -> CowString {
            return make_borrowed(std::string_view(str, len));
        }
    }
}

#endif // __DARK_COMMON_COW_HPP__