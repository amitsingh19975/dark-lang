#ifndef __DARK_COMMON_OSTREAM_HPP__
#define __DARK_COMMON_OSTREAM_HPP__

#include "./cow.hpp"
#include "./format.hpp"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <ostream>

namespace dark {
    template <typename DeriveT>
    struct Printable {
        using child_type = DeriveT;

        LLVM_DUMP_METHOD auto dump() const -> void 
            requires detail::ImplementsPrint<child_type>
        {
            self()->Print(llvm::errs());
        }

        constexpr auto self() const -> child_type const& {
            return static_cast<child_type const&>(*this);
        }

        friend auto operator<<(llvm::raw_ostream& os, const child_type& printable) -> llvm::raw_ostream& 
            requires detail::ImplementsPrint<child_type>
        {
            printable.print(os);
            return os;
        }
        
        friend auto operator<<(std::ostream& os, const child_type& printable) -> std::ostream& 
            requires detail::ImplementsPrint<child_type>
        {
            auto rs = llvm::raw_os_ostream(os);
            printable.print(rs);
            return os;
        }
        
        auto to_cow_string() const -> CowString requires detail::ImplementsPrint<child_type> {
            return self()->to_cow_string();
        }
    };


    inline auto print_to_string(detail::ImplementsLLVMOStream auto const& printable) -> std::string {
        std::string str;
        llvm::raw_string_ostream rs(str);
        rs << printable;
        return rs.str();
    }

} // namespace dark

// Taken from Carbon Language
// https://github.com/carbon-language/carbon-lang/blob/d28c63df012adf1d0adc86bb3fee363d9c8a724a/common/ostream.h#L66
namespace llvm {

// Injects an `operator<<` overload into the `llvm` namespace which detects LLVM
// types with `raw_ostream` overloads and uses that to map to a `std::ostream`
// overload. This allows LLVM types to be printed to `std::ostream` via their
// `raw_ostream` operator overloads, which is needed both for logging and
// testing.
//
// To make this overload be unusually low priority, it is designed to take even
// the `std::ostream` parameter as a template, and SFINAE disable itself unless
// that template parameter matches `std::ostream`. This ensures that an
// *explicit* operator will be preferred when provided. Some LLVM types may have
// this, and so we want to prioritize accordingly.
//
// It would be slightly cleaner for LLVM itself to provide this overload in
// `raw_os_ostream.h` so that we wouldn't need to inject into LLVM's namespace,
// but supporting `std::ostream` isn't a priority for LLVM so we handle it
// locally instead.
template <typename StreamT, typename ClassT,
          typename = std::enable_if_t<
              std::is_base_of_v<std::ostream, std::decay_t<StreamT>>>,
          typename = std::enable_if_t<
              !std::is_same_v<std::decay_t<ClassT>, raw_ostream>>>
auto operator<<(StreamT& standard_out, const ClassT& value) -> StreamT& {
  raw_os_ostream(standard_out) << value;
  return standard_out;
}

}  // namespace llvm

#endif // __DARK_COMMON_OSTREAM_HPP__