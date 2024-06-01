#ifndef __DARK_BASE_INDEX_BASE_HPP__
#define __DARK_BASE_INDEX_BASE_HPP__

#include "common/assert.hpp"
#include "common/ostream.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace dark {

    struct IdBase: public Printable<IdBase> {
        using inner_type = std::int32_t;
        static constexpr inner_type invalid {-1};
        
        constexpr IdBase() noexcept = default;
        constexpr explicit IdBase(inner_type id) noexcept
            : index(id) 
        {
        }

        template <typename T>
            requires (!std::numeric_limits<T>::is_signed)
        constexpr explicit IdBase(T id) noexcept
        {
            dark_assert(id >= 0 && id <= static_cast<T>(std::numeric_limits<inner_type>::max()), "Invalid id");
            index = static_cast<inner_type>(id);
        }
        
        constexpr IdBase(const IdBase&) noexcept = default;
        constexpr IdBase(IdBase&&) noexcept = default;
        constexpr IdBase& operator=(const IdBase&) noexcept = default;
        constexpr IdBase& operator=(IdBase&&) noexcept = default;
        constexpr ~IdBase() noexcept = default;

        constexpr auto is_valid() const noexcept -> bool { return index != invalid; }
        constexpr operator bool() const noexcept { return is_valid(); }

        auto print(llvm::raw_ostream& os) const -> void {
            is_valid() ? (os << index) : (os << "<invalid>");
        }

        constexpr auto as_unsigned() const noexcept -> unsigned {
            return static_cast<unsigned>(index);
        }

        constexpr operator size_t() const noexcept {
            dark_assert((this->index) >= 0, "Invalid index");
            return static_cast<size_t>(index);
        }

        inner_type index{invalid};
    };

    struct IndexBase: public IdBase {
        using IdBase::IdBase;
    };

    namespace detail {
        template <typename T>
        concept IsIndexBase = std::derived_from<std::decay_t<std::remove_cvref_t<T>>, IdBase> 
            || std::same_as<IdBase, std::decay_t<std::remove_cvref_t<T>>>
            || std::convertible_to<std::decay_t<std::remove_cvref_t<T>>, IdBase>;
    } // namespace detail
}

template <dark::detail::IsIndexBase LHS, dark::detail::IsIndexBase RHS>
constexpr auto operator==(LHS const& lhs, RHS const& rhs) noexcept -> bool {
    return lhs.index == rhs.index;
}

template <dark::detail::IsIndexBase LHS, dark::detail::IsIndexBase RHS>
constexpr auto operator<=>(LHS const& lhs, RHS const& rhs) noexcept -> std::strong_ordering {
    return lhs.index <=> rhs.index;
}

namespace dark {

    template <detail::IsIndexBase Index>
    struct IndexMapInfo {
        static inline auto get_empty_key() -> Index {
            return Index(llvm::DenseMapInfo<int32_t>::getEmptyKey());
        }

        static inline auto get_tombstone_key() -> Index {
            return Index(llvm::DenseMapInfo<int32_t>::getTombstoneKey());
        }

        static auto get_hash_value(const Index& val) -> unsigned {
            return llvm::DenseMapInfo<int32_t>::getHashValue(val.index);
        }

        static auto is_equal(const Index& lhs, const Index& rhs) -> bool {
            return lhs == rhs;
        }
    };

} // namespace dark

#endif // __DARK_BASE_INDEX_BASE_HPP__