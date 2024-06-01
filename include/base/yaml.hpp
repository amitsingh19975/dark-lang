#ifndef __DARK_BASE_YAML_HPP__
#define __DARK_BASE_YAML_HPP__

#include "common/assert.hpp"
#include "common/ostream.hpp"
#include <llvm/ADT/STLFunctionalExtras.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/raw_ostream.h>

namespace dark::yaml {

    template <typename T>
    static inline auto print(llvm::raw_ostream& out, T const& yml) -> void {
        (void)out;
        (void)yml;
    }

    template <typename T>
    struct Printable: public ::dark::Printable<T> {
        using base_type = ::dark::Printable<T>;

        auto print(llvm::raw_ostream& out) const -> void {
            llvm::yaml::Output y_os(out, /*Ctxt=*/nullptr, /*WrapColumn=*/80);
            y_os << base_type::self()->OutputYaml();
        }
    };

    struct OutputScalar {
        template <typename T>
        explicit OutputScalar(T const& value)
            : m_output([&value](llvm::raw_ostream& out) {
                if constexpr (detail::ImplementsPrint<T>) {
                    value.print(out);
                } else {
                    out << value;
                }
            })
        {}

        explicit OutputScalar(llvm::function_ref<void(llvm::raw_ostream&)> output)
            : m_output(output)
        {}

        auto output(llvm::raw_ostream& out) const -> void {
            m_output(out);
        }
    private:
        llvm::function_ref<void(llvm::raw_ostream&)> m_output;
    };

    struct OutputMapping {
        struct Map {
            constexpr Map(llvm::yaml::IO& io) noexcept
                : m_io(io)
            {}

            template <typename T>
            auto put(llvm::StringRef key, T value) -> void {
                m_io.mapRequired(key.data(), value);
            }
        private:
            llvm::yaml::IO& m_io;
        };

        explicit OutputMapping(llvm::function_ref<void(Map)> output)
            : m_output(output)
        {}

        auto output(llvm::yaml::IO& io) const -> void { m_output(Map(io)); }

    private:
        llvm::function_ref<void(OutputMapping::Map)> m_output;
    };

} // namespace dark::yaml

namespace llvm::yaml {
    template <>
    struct ScalarTraits<::dark::yaml::OutputScalar> {
        static auto output(
            ::dark::yaml::OutputScalar const& value, 
            [[maybe_unused]] void* ctxt, 
            raw_ostream& out
        ) -> void {
            value.output(out);
        }

        static auto input(
            [[maybe_unused]] StringRef scalar, 
            [[maybe_unused]] void* ctx, 
            [[maybe_unused]] ::dark::yaml::OutputScalar& value
        ) -> StringRef {
            dark_assert(false, "OutputScalar is not meant to be read from yaml");
            return "";
        }

        static auto mustQuote(
            [[maybe_unused]] llvm::StringRef value
        ) -> QuotingType { return QuotingType::None; }
    };

    static_assert(has_ScalarTraits<::dark::yaml::OutputScalar>::value, "OutputScalar must have ScalarTraits");

    template <>
    struct MappingTraits<::dark::yaml::OutputMapping> {
        static auto mapping(IO& io, ::dark::yaml::OutputMapping& value) -> void {
            value.output(io);
        }
    };

    static_assert(has_MappingTraits<::dark::yaml::OutputMapping, EmptyContext>::value, "OutputMapping must have MappingTraits");

} // namespace llvm::yaml

#endif // __DARK_BASE_YAML_HPP__