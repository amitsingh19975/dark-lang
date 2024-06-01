#ifndef __DARK_COMMON_ENUM_HPP__
#define __DARK_COMMON_ENUM_HPP__

#include <llvm/ADT/StringRef.h>
#include "common/cow.hpp"
#include "ostream.hpp"
#include <type_traits>

namespace dark::detail {

    template <typename DerivedType, std::default_initializable EnumT, const llvm::StringLiteral Names[]>
    struct EnumBase: public Printable<DerivedType> {
        using RawEnumType = EnumT;

        using EnumType = DerivedType;
        using UnderlyingType = std::underlying_type_t<RawEnumType>;
        
        constexpr operator RawEnumType() const noexcept { return m_value; }

        constexpr ~EnumBase() = default;

        [[nodiscard]] constexpr auto name() const -> llvm::StringRef {
            return Names[static_cast<std::size_t>(as_int())];
        }

        auto print(llvm::raw_ostream& os) const -> void {
            os << name();
        }

        auto to_cow_string() const -> CowString {
            return make_borrowed(name());
        }
        
        static constexpr auto Make(RawEnumType value) -> EnumType {
            EnumType result;
            result.m_value = value;
            return result;
        }

        constexpr auto operator==(const EnumBase& other) const -> bool {
            return m_value == other.m_value;
        }
    protected:
        constexpr EnumBase() = default;
        constexpr EnumBase(EnumBase const& value) = default;
        constexpr EnumBase(EnumBase&& value) = default;
        constexpr EnumBase& operator=(EnumBase const& value) = default;
        constexpr EnumBase& operator=(EnumBase&& value) = default;

        constexpr auto operator<=>(const EnumBase&) const = default;
        constexpr auto as_int() const -> UnderlyingType { return static_cast<UnderlyingType>(m_value); }
        constexpr auto from_int(UnderlyingType value) -> EnumType { return EnumBase(static_cast<RawEnumType>(value)); }
        
    private:
        RawEnumType m_value;
    };

} // namespace dark

#define DARK_DEFINE_RAW_ENUM_CLASS_NO_NAMES(EnumClassName, UnderlyingType)\
    namespace detail {\
        enum class EnumClassName##RawEnum: UnderlyingType;\
    }\
    enum class detail::EnumClassName##RawEnum: UnderlyingType

#define DARK_RAW_ENUM_VALUE(EnumClassName, Name) detail::EnumClassName##RawEnum::Name
#define DARK_CAST_RAW_ENUM(EnumClassName, INT) static_cast<detail::EnumClassName##RawEnum>(INT)

#define DARK_DEFINE_RAW_ENUM_CLASS(EnumClassName, UnderlyingType)\
    namespace detail {\
        extern const llvm::StringLiteral EnumClassName##Names[];\
    }\
    DARK_DEFINE_RAW_ENUM_CLASS_NO_NAMES(EnumClassName, UnderlyingType)

#define DARK_RAW_ENUM_ENUMERATOR(NAME) NAME,
#define DARK_RAW_ENUM_ENUMERATOR_WITH_ID(NAME, ID) NAME = ID,

#define DARK_ENUM_BASE(EnumClassName) \
    DARK_ENUM_BASE_CRTP(EnumClassName, EnumClassName, EnumClassName)

#define DARK_ENUM_BASE_CRTP(EnumClassName, LocalTypeNameForEnumClass, \
                              EnumClassNameForNames)                    \
  ::dark::detail::EnumBase<LocalTypeNameForEnumClass, detail::EnumClassName##RawEnum, detail::EnumClassNameForNames##Names>

#define DARK_ENUM_CONSTANT_DECL(Name) static const EnumType Name;

#define DARK_ENUM_CONSTANT_DEFINITION(EnumClassName, Name) \
  constexpr EnumClassName EnumClassName::Name =              \
      EnumClassName::Make(RawEnumType::Name);

#define DARK_INLINE_ENUM_CONSTANT_DEFINITION(Name)     \
  static constexpr const typename Base::EnumType& Name = \
      Base(Base::RawEnumType::Name);
    
#define DARK_DEFINE_ENUM_CLASS_NAMES(EnumClassName) \
  constexpr llvm::StringLiteral detail::EnumClassName##Names[]

#define DARK_ENUM_CLASS_NAME_STRING(Name) #Name,

#endif // __DARK_COMMON_ENUM_HPP__