#include "source/source_buffer.hpp"
#include "base/index_base.hpp"
#include "diagnostics/basic_diagnostic.hpp"
#include "diagnostics/diagnostic_emitter.hpp"
#include "diagnostics/dianostic_converter.hpp"
#include <cstddef>
#include <limits>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/ErrorOr.h>

namespace dark {

    struct FilenameConverter: DiagnosticConverter<std::string_view> {
        auto convert_loc(std::string_view filename, context_fn_t context_fn) const -> DiagnosticLocation override {
            return { .filename = filename };
        }
    };

    auto SourceBuffer::make_from_stdin(DiagnosticConsumer &consumer) -> std::optional<SourceBuffer> {
        return make_from_memory_buffer(
            llvm::MemoryBuffer::getSTDIN(),
            "<stdin>",
            false,
            consumer
        );
    }

    auto SourceBuffer::make_from_file(
            llvm::vfs::FileSystem& fs,
            llvm::StringRef filename,
            DiagnosticConsumer& consumer
        ) -> std::optional<SourceBuffer> {
        
        auto converter = FilenameConverter{};
        auto emitter = DiagnosticEmitter(converter, consumer);

        auto file = fs.openFileForRead(filename);
        if (file.getError()) {
            DARK_DIAGNOSTIC(ErrorOpeningFile, Error, "Error opening file for read: {0}", std::string);
            emitter.emit(filename, ErrorOpeningFile, file.getError().message());
            return std::nullopt;
        }

        auto status = (*file)->status();
        if (!status) {
            DARK_DIAGNOSTIC(ErrorStattingFile, Error, "Error statting file: {0}", std::string);
            emitter.emit(filename, ErrorStattingFile, status.getError().message());
            return std::nullopt;
        }

        auto is_regular_file = status->isRegularFile();
        auto size = status->getSize();
        
        return make_from_memory_buffer(
            (*file)->getBuffer(filename, size, /*RequiresNullTerminator=*/false),
            filename,
            is_regular_file,
            consumer
        );
    }

    auto SourceBuffer::make_from_memory_buffer(
        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer,
        llvm::StringRef filename,
        bool is_regular_file,
        DiagnosticConsumer& consumer
    ) -> std::optional<SourceBuffer> {
        auto converter = FilenameConverter{};
        auto emitter = DiagnosticEmitter(converter, consumer);

        if (buffer.getError()) {
            DARK_DIAGNOSTIC(ErrorReadingFile, Error, "Error reading file: {0}", std::string);
            emitter.emit(filename, ErrorReadingFile, buffer.getError().message());
            return std::nullopt;
        }

        if (buffer.get()->getBufferSize() >= std::numeric_limits<IdBase::inner_type>::max()) {
            DARK_DIAGNOSTIC(FileTooLarge, Error, "File is over the 2GiB input limit; size is {0} bytes.", std::size_t);
            emitter.emit(filename, FileTooLarge, buffer.get()->getBufferSize());
            return std::nullopt;
        }

        return SourceBuffer(
            filename.str(),
            std::move(buffer.get()),
            is_regular_file
        );
    }

} // namespace dark