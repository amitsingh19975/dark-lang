#ifndef __DARK_SOURCE_SOURCE_BUFFER_HPP__
#define __DARK_SOURCE_SOURCE_BUFFER_HPP__

#include "diagnostics/diagnostic_consumer.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <optional>
#include <string>
#include <memory>
namespace dark {

    struct SourceBuffer {

        SourceBuffer() = delete;
        SourceBuffer(SourceBuffer const&) = delete;
        constexpr SourceBuffer(SourceBuffer&&) noexcept = default;
        constexpr SourceBuffer& operator=(SourceBuffer const&) = delete;
        SourceBuffer& operator=(SourceBuffer&&) noexcept = default;
        ~SourceBuffer() = default;

        static auto make_from_file(
            llvm::vfs::FileSystem& fs,
            llvm::StringRef filename,
            DiagnosticConsumer& consumer
        ) -> std::optional<SourceBuffer>;

        static auto make_from_stdin(
            DiagnosticConsumer& consumer
        ) -> std::optional<SourceBuffer>;

        constexpr auto get_filename() const noexcept -> std::string_view {
            return m_filename;
        }

        constexpr auto get_source() const noexcept -> llvm::StringRef {
            return m_source->getBuffer();
        }

        [[nodiscard]] constexpr auto is_regular_file() const noexcept -> bool {
            return m_is_regular_file;
        }

    private:
        static auto make_from_memory_buffer(
            llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer,
            llvm::StringRef filename,
            bool is_regular_file,
            DiagnosticConsumer& consumer
        ) -> std::optional<SourceBuffer>;
        
        explicit SourceBuffer(std::string filename, std::unique_ptr<llvm::MemoryBuffer> source, bool is_regular_file)
            : m_filename(std::move(filename))
            , m_source(std::move(source))
            , m_is_regular_file(is_regular_file)
        {}

    private:
        std::string m_filename;
        std::unique_ptr<llvm::MemoryBuffer> m_source;
        bool m_is_regular_file;
    };

} // namespace dark

#endif // __DARK_SOURCE_SOURCE_BUFFER_HPP__