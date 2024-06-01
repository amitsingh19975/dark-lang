#ifndef __DARK_COMMON_BIG_NUM_HPP__
#define __DARK_COMMON_BIG_NUM_HPP__

#include <algorithm>
#include <cstdlib>
#include <gmp.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <gmpxx.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dark {

    enum class BigNumKind {
        UnsignedInteger,
        SignedInteger,
        Float,
        Real
    };

    template <BigNumKind kind>
    struct BasicBigNum;

    namespace detail {
        template <BigNumKind kind>
        struct BigNumberTraits;

        template <>
        struct BigNumberTraits<BigNumKind::UnsignedInteger> {
            using base_type = mpz_class;
            using value_type = unsigned long int;
            using size_type = std::size_t;

            constexpr static auto get_number_of_bits(const base_type& value) noexcept -> size_type {
                return mpz_sizeinbase(value.get_mpz_t(), 2);
            }

            template <std::integral T>
            static void assign(base_type& c, T l) {
                static_assert(std::is_unsigned_v<T>, "Unsigned integer required");
                mpz_set_ui(c.get_mpz_t(), static_cast<value_type>(l));
            }

            static decltype(auto) get_internal(base_type const& value) noexcept {
                return value.get_mpz_t();
            }

            static decltype(auto) get_internal(base_type& value) noexcept {
                return value.get_mpz_t();
            }

            static void add(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_add(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void add(base_type& c, base_type const& a, T b) noexcept {
                mpz_add_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void sub(base_type& c, base_type const& a, base_type const& b) noexcept {
                if (a < b) mpz_set_ui(c.get_mpz_t(), 0);
                else mpz_sub(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                if (a.fits_ulong_p() && a.get_ui() < b) mpz_set_ui(c.get_mpz_t(), 0);
                else mpz_sub_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void mul(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_mul(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                mpz_mul_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void div(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_tdiv_q(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void div(base_type& c, base_type const& a, T b) noexcept {
                mpz_tdiv_q_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void mod(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_mod(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void mod(base_type& c, base_type const& a, T b) noexcept {
                mpz_mod_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void shift_left(base_type& c, base_type const& a, size_type b) noexcept {
                auto size = mpz_size(a.get_mpz_t());
                if (size == 0) {
                    mpz_set_ui(c.get_mpz_t(), 0);
                    return;
                }
                auto new_size = static_cast<long int>(size + 1);
                auto sign = mpz_sgn(a.get_mpz_t());
                auto o = mpz_limbs_modify(c.get_mpz_t(), new_size);
                auto i = mpz_limbs_read(a.get_mpz_t());
                mpn_lshift(o, i, size, b);
                mpz_limbs_finish(c.get_mpz_t(), sign * new_size);
            }

            static void shift_right(base_type& c, base_type const& a, size_type b) noexcept {
                auto size = mpz_size(a.get_mpz_t());
                if (size == 0) {
                    mpz_set_ui(c.get_mpz_t(), 0);
                    return;
                }
                auto new_size = static_cast<long int>(size);
                auto sign = mpz_sgn(a.get_mpz_t());
                auto o = mpz_limbs_modify(c.get_mpz_t(), new_size);
                auto i = mpz_limbs_read(a.get_mpz_t());
                mpn_rshift(o, i, size, b);
                mpz_limbs_finish(c.get_mpz_t(), sign * new_size);
            }

            static int sign([[maybe_unused]] base_type const& a) noexcept {
                return 1;
            }
        };

        template <>
        struct BigNumberTraits<BigNumKind::SignedInteger> {
            using base_type = mpz_class;
            using value_type = signed long int;
            using size_type = std::size_t;

            constexpr static auto get_number_of_bits(const base_type& value) noexcept -> size_type{
                return mpz_sizeinbase(value.get_mpz_t(), 2) + 1;
            }

            template <std::integral T>
            static void assign(base_type& c, T l) {
                static_assert(std::is_signed_v<T>, "Signed integer required");
                if (l >= 0) mpz_set_ui(c.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(l));
                else mpz_set_si(c.get_mpz_t(), static_cast<value_type>(l));
            }

            static decltype(auto) get_internal(base_type const& value) noexcept {
                return value.get_mpz_t();
            }

            static decltype(auto) get_internal(base_type& value) noexcept {
                return value.get_mpz_t();
            }

            static void add(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_add(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
            static void add(base_type& c, base_type const& a, T b) noexcept {
                if (b >= 0) mpz_add_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b));
                else mpz_sub_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(-b));
            }

            static void sub(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_sub(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                if (b >= 0) mpz_sub_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b));
                else mpz_add_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(-b));
            }

            static void mul(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_mul(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                if (b >= 0) mpz_mul_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b));
                else mpz_mul_si(c.get_mpz_t(), a.get_mpz_t(), static_cast<value_type>(b));
            }

            static void div(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_tdiv_q(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
            static void div(base_type& c, base_type const& a, T b) noexcept {
                mpz_tdiv_q_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b));
                if (b < 0) neg(c, c);
            }

            static void mod(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpz_mod(c.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
            }

            template <std::integral T>
                requires (std::is_unsigned_v<T>)
            static void mod(base_type& c, base_type const& a, T b) noexcept {
                mpz_mod_ui(c.get_mpz_t(), a.get_mpz_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b));
            }
            
            static void neg(base_type& c, base_type const& a) noexcept {
                mpz_neg(c.get_mpz_t(), a.get_mpz_t());
            }

            static void shift_left(base_type& c, base_type const& a, size_type b) noexcept {
                auto size = mpz_size(a.get_mpz_t());
                if (size == 0) {
                    mpz_set_ui(c.get_mpz_t(), 0);
                    return;
                }
                
                auto new_size = static_cast<long int>(size + 1);
                auto sign = mpz_sgn(a.get_mpz_t());
                auto o = mpz_limbs_modify(c.get_mpz_t(), new_size);
                auto i = mpz_limbs_read(a.get_mpz_t());
                mpn_lshift(o, i, size, b);
                mpz_limbs_finish(c.get_mpz_t(), sign * new_size);
            }

            static void shift_right(base_type& c, base_type const& a, size_type b) noexcept {
                auto size = mpz_size(a.get_mpz_t());
                if (size == 0) {
                    mpz_set_ui(c.get_mpz_t(), 0);
                    return;
                }
                
                auto new_size = static_cast<long int>(size);
                auto sign = mpz_sgn(a.get_mpz_t());
                auto o = mpz_limbs_modify(c.get_mpz_t(), new_size);
                auto i = mpz_limbs_read(a.get_mpz_t());
                mpn_rshift(o, i, size, b);
                mpz_limbs_finish(c.get_mpz_t(), sign * new_size);
            }

            static int sign(base_type const& a) noexcept {
                return mpz_sgn(a.get_mpz_t());
            }
        };

        template <>
        struct BigNumberTraits<BigNumKind::Float> {
            using base_type = mpf_class;
            using value_type = double;
            using size_type = std::size_t;
            using exp_type = mp_exp_t;

            constexpr static auto get_number_of_bits(const base_type& value) noexcept -> size_type{
                return value.get_prec();
            }

            template <std::integral T>
            static void assign(base_type& c, T l) {
                if constexpr (std::is_signed_v<T>) {
                    auto val = static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(l);
                    if (val < 0) {
                        mpf_set_si(c.get_mpf_t(), val);
                        return;
                    }
                }
                
                mpf_set_ui(c.get_mpf_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(l));
            }

            template <std::floating_point T>
            static void assign(base_type& c, T d) { 
                mpf_set_d(c.get_mpf_t(), static_cast<value_type>(d));
            }

            static decltype(auto) get_internal(base_type const& value) noexcept {
                return value.get_mpf_t();
            }

            static decltype(auto) get_internal(base_type& value) noexcept {
                return value.get_mpf_t();
            }

            static void add(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpf_add(c.get_mpf_t(), a.get_mpf_t(), b.get_mpf_t());
            }

            template <std::floating_point T>
            static void add(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpf_class(static_cast<double>(b));
                mpf_add(c.get_mpf_t(), a.get_mpf_t(), temp.get_mpf_t());
            }
            
            template <std::integral T>
            static void add(base_type& c, base_type const& a, T b) noexcept {
                auto other = static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(b < 0 ? -b : b);
                if constexpr (std::is_signed_v<T>) {
                    if (b < 0) sub(c, a, other);
                    else mpf_add_ui(c.get_mpf_t(), a.get_mpf_t(), other);
                } else {
                    mpf_add_ui(c.get_mpf_t(), a.get_mpf_t(), other);
                }
            }

            static void sub(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpf_sub(c.get_mpf_t(), a.get_mpf_t(), b.get_mpf_t());
            }

            template <std::floating_point T>
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpf_class(static_cast<double>(b));
                mpf_sub(c.get_mpf_t(), a.get_mpf_t(), temp.get_mpf_t());
            }

            template <std::integral T>
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                auto other = static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(std::abs(b));
                if constexpr (std::is_signed_v<T>) {
                    if (b < 0) add(c, a, other);
                    else mpf_sub_ui(c.get_mpf_t(), a.get_mpf_t(), other);
                } else {
                    mpf_sub_ui(c.get_mpf_t(), a.get_mpf_t(), other);
                }
            }

            static void mul(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpf_mul(c.get_mpf_t(), a.get_mpf_t(), b.get_mpf_t());
            }

            template <std::floating_point T>
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpf_class(static_cast<double>(b));
                mul(c, a, temp);
            }

            template <std::integral T>
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                auto other = static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(std::abs(b));
                if constexpr (std::is_signed_v<T>) {
                    if (b < 0) neg(c, c);
                }
                mpf_mul_ui(c.get_mpf_t(), a.get_mpf_t(), other);
            }

            static void div(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpf_div(c.get_mpf_t(), a.get_mpf_t(), b.get_mpf_t());
            }

            template <std::floating_point T>
            static void div(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpf_class(static_cast<double>(b));
                div(c, a, temp);
            }

            template <std::integral T>
            static void div(base_type& c, base_type const& a, T b) noexcept {
                auto other = static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(std::abs(b));
                if constexpr (std::is_signed_v<T>) {
                    if (b < 0) neg(c, c);
                }
                mpf_div_ui(c.get_mpf_t(), a.get_mpf_t(), other);
            }

            static void neg(base_type& c, base_type const& a) noexcept {
                mpf_neg(c.get_mpf_t(), a.get_mpf_t());
            }

            static int sign(base_type const& a) noexcept {
                return mpf_sgn(a.get_mpf_t());
            }
        };

        template <>
        struct BigNumberTraits<BigNumKind::Real> {
            using base_type = mpq_class;
            using value_type = double;
            using size_type = std::size_t;

            constexpr static auto get_number_of_bits(const base_type& value) noexcept -> size_type {
                return mpz_sizeinbase(value.get_num_mpz_t(), 2) + mpz_sizeinbase(value.get_den_mpz_t(), 2);
            }

            template <std::integral T>
            static void assign(base_type& c, T l) {
                if constexpr (std::is_signed_v<T>) {
                    auto val = static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(l);
                    if (val < 0) {
                        mpq_set_si(c.get_mpq_t(), val, 1);
                        return;
                    }
                }
                
                mpq_set_ui(c.get_mpq_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(l), 1);
            }

            template <std::floating_point T>
            static void assign(base_type& c, T d) { 
                mpq_set_d(c.get_mpq_t(), static_cast<value_type>(d));
            }

            template <std::integral T, typename U>
            static void assign(base_type& c, T num, U den) {
                auto factor = den < 0 ? -1 : 1;
                auto d = static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(std::abs(den));
                if constexpr (std::is_signed_v<T>) {
                    auto val = static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(num) * factor;
                    if (val < 0) {
                        mpq_set_si(c.get_mpq_t(), val, d);
                        return;
                    }
                }
                if (factor == -1) {
                    mpq_set_si(c.get_mpq_t(), static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(num) * factor, d);
                } else {
                    mpq_set_ui(c.get_mpq_t(), static_cast<BigNumberTraits<BigNumKind::UnsignedInteger>::value_type>(num), d);
                }
            }

            static decltype(auto) get_internal(base_type const& value) noexcept {
                return value.get_mpq_t();
            }

            static decltype(auto) get_internal(base_type& value) noexcept {
                return value.get_mpq_t();
            }

            static void add(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpq_add(c.get_mpq_t(), a.get_mpq_t(), b.get_mpq_t());
            }

            template <std::floating_point T>
            static void add(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<double>(b));
                mpq_add(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            template <std::integral T>
            static void add(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast< BigNumberTraits<BigNumKind::SignedInteger>::value_type >(b), 1ul);
                mpq_add(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            static void sub(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpq_sub(c.get_mpq_t(), a.get_mpq_t(), b.get_mpq_t());
            }

            template <std::floating_point T>
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<double>(b));
                mpq_sub(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            template <std::integral T>
            static void sub(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(b), 1ul);
                mpq_sub(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            static void mul(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpq_mul(c.get_mpq_t(), a.get_mpq_t(), b.get_mpq_t());
            }

            template <std::floating_point T>
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<double>(b));
                mpq_mul(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            template <std::integral T>
            static void mul(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(b), 1ul);
                mpq_mul(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            static void div(base_type& c, base_type const& a, base_type const& b) noexcept {
                mpq_div(c.get_mpq_t(), a.get_mpq_t(), b.get_mpq_t());
            }

            template <std::floating_point T>
            static void div(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<double>(b));
                mpq_div(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            template <std::integral T>
            static void div(base_type& c, base_type const& a, T b) noexcept {
                auto temp = mpq_class(static_cast<BigNumberTraits<BigNumKind::SignedInteger>::value_type>(b), 1ul);
                mpq_div(c.get_mpq_t(), a.get_mpq_t(), temp.get_mpq_t());
            }

            static void neg(base_type& c, base_type const& a) noexcept {
                mpq_neg(c.get_mpq_t(), a.get_mpq_t());
            }

            static int sign(base_type const& a) noexcept {
                return mpq_sgn(a.get_mpq_t());
            }
        };

        template <BigNumKind K>
        concept IsBigIntegerKind = (K == BigNumKind::UnsignedInteger || K == BigNumKind::SignedInteger);

        template <BigNumKind K>
        concept IsBigRealKind = (K == BigNumKind::Real);

        template <BigNumKind K>
        concept IsBigFloatKind = (K == BigNumKind::Float);

        template <BigNumKind K, typename T>
        constexpr auto can_apply_op(T&&) -> std::false_type;
        
        template <BigNumKind K, typename T>
            requires (std::is_arithmetic_v<T>)
        constexpr auto can_apply_op(T&&) -> std::true_type;
        
        template <BigNumKind K>
        constexpr auto can_apply_op(BasicBigNum<K>) -> std::true_type;
        
        template <BigNumKind K, typename T>
        concept CanApplyOp = decltype(can_apply_op<K>(std::declval<T>()))::value;


        template <BigNumKind kind>
        constexpr auto validate_number(llvm::StringRef value) {
            value = value.trim();
            if constexpr (kind == BigNumKind::UnsignedInteger) {
                if (value.contains('-')) {
                    throw std::invalid_argument("Unsigned integer cannot be negative");
                } else if (value.contains('.')) {
                    throw std::invalid_argument("Unsigned integer cannot be real");
                }
            } else if constexpr (kind == BigNumKind::SignedInteger) {
                if (value.contains('.')) {
                    throw std::invalid_argument("Signed integer cannot be real");
                }
            }
        }

        template <typename T>
        struct is_big_num : std::false_type {};

        template <BigNumKind kind>
        struct is_big_num<BasicBigNum<kind>> : std::true_type {};

        template <typename T>
        concept IsBigNum = is_big_num<std::decay_t<std::remove_cvref_t<T>>>::value;
    } // namespace detail

    template <BigNumKind K>
    struct BasicBigNum {
        static constexpr auto kind = K;
        using trait = detail::BigNumberTraits<kind>;
        using base_type = typename trait::base_type;
        using value_type = typename trait::value_type;
        using size_type = std::size_t;

        BasicBigNum() = default;
        BasicBigNum(const BasicBigNum&) = default;
        BasicBigNum(BasicBigNum&&) noexcept = default;
        BasicBigNum& operator=(const BasicBigNum&) = default;
        BasicBigNum& operator=(BasicBigNum&&) noexcept = default;
        ~BasicBigNum() = default;

        BasicBigNum(std::string value, unsigned base = 0) {
            detail::validate_number<kind>(value);
            m_value.set_str(value, static_cast<int>(base));
        }

        BasicBigNum(char const* value, unsigned base = 0)
            : BasicBigNum(std::string_view(value), base)
        {}
        
        template <BigNumKind O>
            requires (std::same_as<base_type, typename BasicBigNum<O>::base_type>)
        BasicBigNum(BasicBigNum<O> other)
            : m_value(std::move(other.m_value))
        {}

        BasicBigNum(std::string_view value, unsigned base = 0) {
            detail::validate_number<kind>(value);
            m_value.set_str(value.data(), static_cast<int>(base));
        }

        template <std::integral T>
            requires (std::is_unsigned_v<T>)
        BasicBigNum(T val) requires (kind == BigNumKind::UnsignedInteger) {
            trait::assign(m_value, val);
        }
        
        template <BigNumKind N, BigNumKind D>
            requires ((N == BigNumKind::UnsignedInteger || N == BigNumKind::SignedInteger) && (D == BigNumKind::UnsignedInteger))
        BasicBigNum(
            BasicBigNum<N> const& num,
            BasicBigNum<D> const& den
        ) requires (kind == BigNumKind::Real)
            : m_value(num.m_value, den.m_value)
        {
        }
        
        template <std::integral N, std::integral D>
        BasicBigNum(
            N num,
            D den
        ) requires (kind == BigNumKind::Real)
        {
            trait::assign(m_value, num, den);
        }

        template <std::integral T>
            requires (std::is_signed_v<T>)
        BasicBigNum(T val) requires (kind == BigNumKind::SignedInteger) {
            trait::assign(m_value, val);
        }

        template <typename T>
            requires (std::floating_point<T> || std::integral<T>)
        BasicBigNum(T val) requires (!detail::IsBigIntegerKind<kind>) {
            trait::assign(m_value, val);
        }

        friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const BasicBigNum& num) {
            os << num.m_value.get_str();
            return os;
        }

        friend constexpr auto operator<(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value < rhs.m_value;
        }

        friend constexpr auto operator>(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value > rhs.m_value;
        }

        friend constexpr auto operator<=(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value <= rhs.m_value;
        }

        friend constexpr auto operator>=(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value >= rhs.m_value;
        }

        friend constexpr auto operator==(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value == rhs.m_value;
        }

        friend constexpr auto operator!=(const BasicBigNum& lhs, const BasicBigNum& rhs) noexcept -> bool {
            return lhs.m_value != rhs.m_value;
        }

        auto to_sign_extended_str(unsigned bits = 0, bool prefix = true) -> std::string {
            auto res = std::string{};
            if (m_value == 0) {
                if (bits != 0) {
                    res.insert(0, bits, '0');
                } else {
                    res = "0";
                }
            } else {
                res = m_value.get_str(2);

                bool has_sign = res[0] == '-';
                if (has_sign) {
                    res.erase(0, 1);
                }
                if (bits != 0) {
                    char sign = has_sign ? '1' : '0';
                    auto remaining = bits < res.size() ? 0 : bits - res.size();
                    res.insert(0, remaining, sign);
                }
            }
            if (prefix) {
                res.insert(0, "0b");
            }
            return res;
        }
        auto to_str(unsigned radix = 10, bool prefix = true) const -> std::string requires (kind != BigNumKind::Float){
            auto res = std::string{};
            res = m_value.get_str(static_cast<int>(radix));
            if (prefix) {
                auto insert_pos = res[0] == '-' ? 1ul : 0ul;
                switch (radix) {
                    case 2:
                        res.insert(insert_pos, "0b");
                        break;
                    case 8:
                        res.insert(insert_pos, "0o");
                        break;
                    case 16:
                        res.insert(insert_pos, "0x");
                        break;
                    default:
                        break;
                }
            }
            return res;
        }

        auto to_str(unsigned radix = 10, bool prefix = true) const -> std::string requires (kind == BigNumKind::Float) {
            auto res = std::string{};
            auto temp_radix = static_cast<int>(radix);
            auto exponent = typename trait::exp_type{};
            res = m_value.get_str(exponent, temp_radix);
            
            auto insert_pos = res[0] == '-' ? 1ul : 0ul;
            if (exponent != 0) {
                auto pos = insert_pos + static_cast<std::size_t>(exponent);

                while(pos > res.size()) {
                    res.append("0");
                }

                if (pos < res.size()) {
                    res.insert(pos, ".");
                } else {
                    res.append(".0");
                }
            }

            if (res.empty()) {
                res = "0.0";
            }

            if (prefix) {
                switch (radix) {
                    case 2:
                        res.insert(insert_pos, "0b");
                        break;
                    case 8:
                        res.insert(insert_pos, "0o");
                        break;
                    case 16:
                        res.insert(insert_pos, "0x");
                        break;
                    default:
                        break;
                }
            }
            return res;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator+(BasicBigNum const& lhs, T const& rhs) -> BasicBigNum {
            auto temp = BasicBigNum();
            if constexpr (detail::IsBigNum<T>) {
                trait::add(temp.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::add(temp.m_value, lhs.m_value, rhs);
            }
            return temp;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator+=(BasicBigNum& lhs, T const& rhs) -> BasicBigNum& {
            if constexpr (detail::IsBigNum<T>) {
                trait::add(lhs.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::add(lhs.m_value, lhs.m_value, rhs);
            }
            return lhs;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator-(BasicBigNum const& lhs, T const& rhs) -> BasicBigNum {
            auto temp = BasicBigNum();
            if constexpr (detail::IsBigNum<T>) {
                trait::sub(temp.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::sub(temp.m_value, lhs.m_value, rhs);
            }
            return temp;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator-=(BasicBigNum& lhs, T const& rhs) -> BasicBigNum& {
            if constexpr (detail::IsBigNum<T>) {
                trait::sub(lhs.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::sub(lhs.m_value, lhs.m_value, rhs);
            }
            return lhs;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator*(BasicBigNum const& lhs, T const& rhs) -> BasicBigNum {
            auto temp = BasicBigNum();
            if constexpr (detail::IsBigNum<T>) {
                trait::mul(temp.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::mul(temp.m_value, lhs.m_value, rhs);
            }
            return temp;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator*=(BasicBigNum& lhs, T const& rhs) -> BasicBigNum& {
            if constexpr (detail::IsBigNum<T>) {
                trait::mul(lhs.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::mul(lhs.m_value, lhs.m_value, rhs);
            }
            return lhs;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator/(BasicBigNum const& lhs, T const& rhs) -> BasicBigNum {
            auto temp = BasicBigNum();
            if constexpr (detail::IsBigNum<T>) {
                trait::div(temp.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::div(temp.m_value, lhs.m_value, rhs);
            }
            return temp;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator/=(BasicBigNum& lhs, T const& rhs) -> BasicBigNum& {
            if constexpr (detail::IsBigNum<T>) {
                trait::div(lhs.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::div(lhs.m_value, lhs.m_value, rhs);
            }
            return lhs;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator%(BasicBigNum const& lhs, T const& rhs) -> BasicBigNum requires (kind != BigNumKind::Float) {
            auto temp = BasicBigNum();
            if constexpr (detail::IsBigNum<T>) {
                trait::mod(temp.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::mod(temp.m_value, lhs.m_value, rhs);
            }
            return temp;
        }

        template <typename T>
            requires (detail::CanApplyOp<kind, T>)
        friend auto operator%=(BasicBigNum& lhs, T const& rhs) -> BasicBigNum& requires (kind != BigNumKind::Float) {
            if constexpr (detail::IsBigNum<T>) {
                trait::mod(lhs.m_value, lhs.m_value, rhs.m_value);
            } else {
                trait::mod(lhs.m_value, lhs.m_value, rhs);
            }
            return lhs;
        }

        auto operator-() const -> BasicBigNum requires (kind != BigNumKind::UnsignedInteger){
            auto temp = BasicBigNum();
            trait::neg(temp.m_value, m_value);
            return temp;
        }

        friend auto operator<<(BasicBigNum const& lhs, size_type shift) -> BasicBigNum requires (detail::IsBigIntegerKind<kind>) {
            auto temp = BasicBigNum();
            trait::shift_left(temp.m_value, lhs.m_value, shift);
            return temp;
        }

        friend auto operator<<=(BasicBigNum& lhs, size_type shift) -> BasicBigNum& requires (detail::IsBigIntegerKind<kind>) {
            trait::shift_left(lhs.m_value, lhs.m_value, shift);
            return lhs;
        }

        friend auto operator>>(BasicBigNum const& lhs, size_type shift) -> BasicBigNum requires (detail::IsBigIntegerKind<kind>) {
            auto temp = BasicBigNum();
            trait::shift_right(temp.m_value, lhs.m_value, shift);
            return temp;
        }

        friend auto operator>>=(BasicBigNum& lhs, size_type shift) -> BasicBigNum& requires (detail::IsBigIntegerKind<kind>) {
            trait::shift_right(lhs.m_value, lhs.m_value, shift);
            return lhs;
        }

        constexpr auto get_number_of_bits() const noexcept -> size_type {
            return trait::get_number_of_bits(m_value);
        }

        constexpr operator bool() const noexcept {
            return m_value != 0;
        }

        constexpr auto is_negative() const noexcept -> bool {
            return trait::sign(m_value) < 0;
        }

        [[nodiscard]] constexpr auto is_signed() const noexcept -> bool {
            return kind == BigNumKind::SignedInteger;
        }
        
        [[nodiscard]] constexpr auto is_unsigned() const noexcept -> bool {
            return kind == BigNumKind::UnsignedInteger;
        }

        [[nodiscard]] constexpr auto is_real() const noexcept -> bool {
            return kind == BigNumKind::Real;
        }

        [[nodiscard]] constexpr auto is_float() const noexcept -> bool {
            return kind == BigNumKind::Float;
        }

        [[nodiscard]] constexpr auto get_rep_t() const noexcept {
            return trait::get_internal(m_value);
        }

        [[nodiscard]] constexpr auto get_rep_t() noexcept {
            return trait::get_internal(m_value);
        }

        [[nodiscard]] constexpr auto abs() const noexcept -> BasicBigNum {
            using std::swap;
            auto temp = BasicBigNum();
            temp.m_value = std::move(::abs(m_value));
            return temp;
        }

        [[nodiscard]] auto numerator() const -> BasicBigNum<BigNumKind::SignedInteger> requires (kind == BigNumKind::Real) {
            auto temp = BasicBigNum<BigNumKind::SignedInteger>();
            temp.m_value, std::move(m_value.get_num());
            return temp;
        }
        
        [[nodiscard]] auto denominator() const -> BasicBigNum<BigNumKind::UnsignedInteger> requires (kind == BigNumKind::Real) {
            using std::swap;
            auto temp = BasicBigNum<BigNumKind::UnsignedInteger>();
            temp.m_value = std::move(m_value.get_den());
            return temp;
        }

    private:
        template <BigNumKind>
        friend struct BasicBigNum;
    private:
        base_type m_value;
    };

    using UnsignedBigNum = BasicBigNum<BigNumKind::UnsignedInteger>;
    using SignedBigNum = BasicBigNum<BigNumKind::SignedInteger>;
    using BigFloatNum = BasicBigNum<BigNumKind::Float>;
    using BigRealNum = BasicBigNum<BigNumKind::Real>;

    namespace detail {

        template <IsBigNum T>
        static constexpr BigNumKind big_num_kind_v = std::decay_t<std::remove_cvref_t<T>>::kind;

    } // namespace detail

    template <BigNumKind TK, detail::IsBigNum F>
        requires (detail::big_num_kind_v<F> == TK)
    [[nodiscard]] auto cast(F&& from) -> F&& {
        return std::forward<F>(from);
    }

    template <BigNumKind TK, BigNumKind FK>
        requires (TK != FK)
    [[nodiscard]] auto cast(BasicBigNum<FK> const& from) -> BasicBigNum<TK> {
        if constexpr (FK == BigNumKind::UnsignedInteger) {
            if constexpr (TK == BigNumKind::SignedInteger) {
                auto val = from; 
                auto temp = SignedBigNum(std::move(val));
                return temp;
            } else if constexpr (TK == BigNumKind::Float) {
                auto temp = BigFloatNum();
                mpf_set_z(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::Real) {
                auto temp = BigRealNum();
                mpq_set_z(temp.get_rep_t(), from.get_rep_t());
                return temp;
            }
        } else if constexpr (FK == BigNumKind::SignedInteger) {
            if constexpr (TK == BigNumKind::UnsignedInteger) {
                auto temp = UnsignedBigNum(std::move(from.abs()));
                return temp;
            } else if constexpr (TK == BigNumKind::Float) {
                auto temp = BigFloatNum();
                mpf_set_z(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::Real) {
                auto temp = BigRealNum();
                mpq_set_z(temp.get_rep_t(), from.get_rep_t());
                return temp;
            }
        } else if constexpr (FK == BigNumKind::Float) {
            if constexpr (TK == BigNumKind::UnsignedInteger) {
                auto temp = UnsignedBigNum();
                mpz_set_f(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::SignedInteger) {
                auto temp = SignedBigNum();
                mpz_set_f(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::Real) {
                auto temp = BigRealNum();
                mpq_set_f(temp.get_rep_t(), from.get_rep_t());
                return temp;
            }
        } else if constexpr (FK == BigNumKind::Real) {
            if constexpr (TK == BigNumKind::UnsignedInteger) {
                auto temp = UnsignedBigNum();
                mpz_set_q(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::SignedInteger) {
                auto temp = SignedBigNum();
                mpz_set_q(temp.get_rep_t(), from.get_rep_t());
                return temp;
            } else if constexpr (TK == BigNumKind::Float) {
                auto temp = BigFloatNum();
                mpf_set_q(temp.get_rep_t(), from.get_rep_t());
                return temp;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] decltype(auto) to_unsigned_num(detail::IsBigNum auto&& from) {
        return cast<BigNumKind::UnsignedInteger>(std::forward<decltype(from)>(from));
    }

    [[nodiscard]] decltype(auto) to_signed_num(detail::IsBigNum auto&& from) {
        return cast<BigNumKind::SignedInteger>(std::forward<decltype(from)>(from));
    }

    [[nodiscard]] decltype(auto) to_float_num(detail::IsBigNum auto&& from) {
        return cast<BigNumKind::Float>(std::forward<decltype(from)>(from));
    }

    [[nodiscard]] decltype(auto) to_real_num(detail::IsBigNum auto&& from) {
        return cast<BigNumKind::Real>(std::forward<decltype(from)>(from));
    }

    // namespace detail {
        
    //     #if defined(__SIZEOF_INT128__)
    //         using big_unsigned_native_num = __int128;
    //         using big_signed_native_num = __int128;
    //     #else
    //         using big_unsigned_native_num = std::size_t;
    //         using big_signed_native_num = std::ptrdiff_t;
    //     #endif

    // } // namespace detail

} // namespace dark

#endif // __DARK_COMMON_BIG_NUM_HPP__