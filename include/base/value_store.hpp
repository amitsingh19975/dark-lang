#ifndef __DARK_BASE_VALUE_STORE_HPP__
#define __DARK_BASE_VALUE_STORE_HPP__

#include "base/index_base.hpp"
#include "base/yaml.hpp"
#include "common/assert.hpp"
#include "common/cow.hpp"
#include "common/ostream.hpp"
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/ADT/APFloat.h>
#include <string>
#include <string_view>
#include <type_traits>

namespace dark {

    struct IntId: public IdBase, public Printable<IntId> {
        using value_type = llvm::APInt;
        static const IntId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "int";
            IdBase::print(os);
        }
    };

    constexpr IntId IntId::invalid = IntId{};
    static_assert(detail::ImplementsPrint<IntId>, "IntId must implement print method");

    struct Real: public Printable<Real> {
        auto print(llvm::raw_ostream& os) const -> void {
            mantissa.print(os, /*isSigned=*/false);
            os << "*" << (is_decimal ? "10" : "2") << "^" << exponent;
        }

        llvm::APInt mantissa;
        llvm::APInt exponent;
        bool is_decimal;
    };

    struct RealId: public IdBase, public Printable<RealId> {
        // mantissa, exponent; this is here to conform to the 'IsValueStoreValue' concept
        using value_type = Real;
        static const RealId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "real";
            IdBase::print(os);
        }
    };

    static_assert(detail::ImplementsPrint<RealId>, "FloatId must implement print method");


    struct FloatId: public IdBase, public Printable<FloatId> {
        using value_type = llvm::APFloat;
        static const FloatId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "float";
            IdBase::print(os);
        }
    };

    constexpr FloatId FloatId::invalid = FloatId{};
    static_assert(detail::ImplementsPrint<FloatId>, "FloatId must implement print method");


    struct StringId: public IdBase, public Printable<StringId> {
        using value_type = std::string;
        static const StringId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "string";
            IdBase::print(os);
        }
    };

    constexpr StringId StringId::invalid = StringId{};
    static_assert(detail::ImplementsPrint<StringId>, "StringId must implement print method");

    struct IdentifierId: public IdBase, public Printable<IdentifierId> {
        using value_type = std::string;
        static const IdentifierId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "identifier";
            IdBase::print(os);
        }
    };

    constexpr IdentifierId IdentifierId::invalid = IdentifierId{};
    static_assert(detail::ImplementsPrint<IdentifierId>, "IdentifierId must implement print method");


    struct StringLiteralId: public IdBase, public Printable<StringLiteralId> {
        using value_type = std::string;
        static const StringLiteralId invalid;

        using IdBase::IdBase;

        auto print(llvm::raw_ostream& os) const -> void {
            os << "string_literal";
            IdBase::print(os);
        }
    };


    namespace detail {
        struct ValueStoreNotPrintable{};

        template <typename T>
        concept IsValueStoreValue = IsIndexBase<T> && requires {
            typename T::value_type;
        };
    }

    template <detail::IsValueStoreValue IdT>
    struct ValueStore:
        std::conditional_t<
            detail::ImplementsPrint<IdT>,
            yaml::Printable<ValueStore<IdT>>, 
            detail::ValueStoreNotPrintable
        >
    {
        using value_type = typename IdT::value_type;

        auto add(value_type value) -> IdT {
            auto id = IdT{static_cast<IdT::inner_type>(m_values.size())};
            dark_assert(id >= 0, "overflow detected");
            m_values.push_back(value);
            return id;
        }

        auto add_default() -> IdT {
            return add(value_type{});
        }

        constexpr auto get(IdT id) noexcept -> value_type& {
            dark_assert(id.as_unsigned() < m_values.size(), "invalid id");
            return m_values[id];
        }

        constexpr auto get(IdT id) const noexcept -> value_type const& {
            dark_assert(id.as_unsigned() < m_values.size(), "invalid id");
            return m_values[id];
        }

        constexpr auto size() const noexcept -> std::size_t {
            return m_values.size();
        }

        constexpr auto array_ref() const noexcept -> llvm::ArrayRef<value_type> {
            return m_values;
        }

        auto reserve(std::size_t size) -> void {
            m_values.reserve(size);
        }

        auto clear() -> void {
            m_values.clear();
        }

        auto output_yaml() const -> yaml::OutputMapping {
            return yaml::OutputMapping{[this](yaml::OutputMapping::Map map) {
                for (auto i = 0u; i < m_values.size(); ++i) {
                    auto id = IdT(i);
                    map.put(print_to_string(i), yaml::OutputScalar(get(id)));
                }
            }};
        }
    private:
        llvm::SmallVector<value_type, 0> m_values;
    };

    template<>
    struct ValueStore<StringId>: public yaml::Printable<ValueStore<StringId>> {
        auto add(CowString value) -> StringId {
            auto id = StringId{m_values.size()};
            dark_assert(id.index >= 0, "overflow detected");

            auto [it, inserted] = m_map.insert({ value.borrow(), id });
            if (inserted) m_values.push_back(value);
            return it->second;
        }

        auto add_borrowed(std::string_view value) -> StringId {
            return add(CowString::make_borrowed(value));
        }
        
        constexpr auto get(StringId id) noexcept -> std::string_view {
            dark_assert(id.as_unsigned() < m_values.size(), "invalid id");
            return m_values[id].borrow();
        }

        constexpr auto get(StringId id) const noexcept -> std::string_view const {
            dark_assert(id.as_unsigned() < m_values.size(), "invalid id");
            return m_values[id].borrow();
        }

        constexpr auto find(std::string_view value) const noexcept -> StringId {
            auto it = m_map.find(value);
            if (it == m_map.end()) return StringId::invalid;
            return it->second;
        }

        constexpr auto size() const noexcept -> std::size_t {
            return m_values.size();
        }

        auto array_ref() const noexcept -> llvm::ArrayRef<CowString> {
            return m_values;
        }

        auto reserve(std::size_t size) -> void {
            m_values.reserve(size);
        }

        auto clear() -> void {
            m_values.clear();
            m_map.clear();
        }

        auto output_yaml() const -> yaml::OutputMapping {
            return yaml::OutputMapping{[this](yaml::OutputMapping::Map map) {
                for (auto i = 0u; i < m_values.size(); ++i) {
                    auto id = StringId(i);
                    map.put(print_to_string(id), yaml::OutputScalar(get(id)));
                }
            }};
        }
    private:
        llvm::DenseMap<llvm::StringRef, StringId> m_map;
        llvm::SmallVector<CowString, 0> m_values;
    };

    template <detail::IsValueStoreValue T>
    struct StringStoreWrapper: public Printable<StringStoreWrapper<T>> {

        constexpr explicit StringStoreWrapper(ValueStore<StringId>* store)
            : m_store(store)
        {}
        constexpr StringStoreWrapper(StringStoreWrapper<T> const&) = default;
        constexpr StringStoreWrapper(StringStoreWrapper<T>&&) = default;
        constexpr auto operator=(StringStoreWrapper<T> const&) -> StringStoreWrapper<T>& = default;
        constexpr auto operator=(StringStoreWrapper<T>&&) -> StringStoreWrapper<T>& = default;
        constexpr ~StringStoreWrapper() noexcept = default;

        auto add(CowString value) -> T {
            return m_store->add(std::move(value));
        }

        auto add_borrowed(std::string_view value) -> T {
            return T(m_store->add_borrowed(value).index);
        }

        constexpr auto get(T id) noexcept -> std::string_view {
            return m_store->get(StringId(id.index));
        }

        constexpr auto get(T id) const noexcept -> std::string_view const {
            return m_store->get(StringId(id.index));
        }

        constexpr auto size() const noexcept -> std::size_t {
            return m_store->size();
        }

        constexpr auto find(std::string_view value) const noexcept -> T {
            return T(m_store->find(value).index);
        }

        auto print(llvm::raw_ostream& os) const -> void {
            os << *m_store;
        }

    private:
        ValueStore<StringId>* m_store;
    };

    struct SharedValueStores: public yaml::Printable<SharedValueStores> {
        SharedValueStores() = default;
        SharedValueStores(SharedValueStores const&) = delete;
        SharedValueStores(SharedValueStores&&) noexcept = default;
        auto operator=(SharedValueStores const&) -> SharedValueStores& = delete;
        auto operator=(SharedValueStores&&) noexcept -> SharedValueStores& = default;
        ~SharedValueStores() = default;

        constexpr auto identifier() noexcept -> StringStoreWrapper<IdentifierId>& {
            return m_identifier_wrapper;
        }

        constexpr auto identifier() const noexcept -> StringStoreWrapper<IdentifierId> const& {
            return m_identifier_wrapper;
        }

        constexpr auto string_literal() noexcept -> StringStoreWrapper<StringLiteralId>& {
            return m_string_literal_wrapper;
        }

        constexpr auto string_literal() const noexcept -> StringStoreWrapper<StringLiteralId> const& {
            return m_string_literal_wrapper;
        }

        constexpr auto ints() noexcept -> ValueStore<IntId>& {
            return m_ints;
        }

        constexpr auto ints() const noexcept -> ValueStore<IntId> const& {
            return m_ints;
        }

        auto reals() noexcept -> ValueStore<RealId>& {
            return m_real;
        }

        auto reals() const noexcept -> ValueStore<RealId> const& {
            return m_real;
        }

        constexpr auto strings() noexcept -> ValueStore<StringId>& {
            return m_strings;
        }

        constexpr auto strings() const noexcept -> ValueStore<StringId> const& {
            return m_strings;
        }

        auto output_yaml(std::optional<llvm::StringRef> filename = std::nullopt) const -> yaml::OutputMapping {
            return yaml::OutputMapping{[this, filename](yaml::OutputMapping::Map map) {
                if (filename) {
                    map.put("filename", *filename);
                }
                map.put("shared_values", yaml::OutputMapping([this](yaml::OutputMapping::Map map) {
                    map.put("ints", m_ints.output_yaml());
                    map.put("floats", m_real.output_yaml());
                    map.put("strings", m_strings.output_yaml());
                }));
            }};
        }

    private:
        ValueStore<IntId> m_ints;
        ValueStore<RealId> m_real;
        ValueStore<StringId> m_strings;

        StringStoreWrapper<IdentifierId> m_identifier_wrapper{ &m_strings };
        StringStoreWrapper<StringLiteralId> m_string_literal_wrapper{ &m_strings };
    };

} // namespace dark

#endif // __DARK_BASE_VALUE_STORE_HPP__