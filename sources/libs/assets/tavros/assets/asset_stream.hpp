#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/string.hpp>

namespace tavros::assets
{

    /**
     * @brief Represents the reference point for seeking within a stream.
     */
    enum class seek_dir
    {
        /// Seek relative to the beginning of the stream.
        begin,

        /// Seek relative to the current position in the stream.
        current,

        /// Seek relative to the end of the stream.
        end,
    };

    /**
     * @brief Abstract interface for reading and writing data from/to a stream.
     *
     * This class provides a generic interface for byte-level I/O from various
     * storage backends, such as files, memory buffers, or custom providers.
     * It is non-copyable and follows RAII semantics.
     */
    class asset_stream : core::noncopyable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~asset_stream() noexcept = default;

        /**
         * @brief Reads data from the stream into the provided buffer.
         *
         * @param buffer The buffer to store the read data.
         * @return The number of bytes actually read.
         * @throws core::file_error If a read operation fails.
         */
        virtual size_t read(core::buffer_span<uint8> buffer) = 0;

        /**
         * @brief Writes data from the provided buffer into the stream.
         *
         * @param buffer The data to write.
         * @return The number of bytes actually written.
         * @throws core::file_error If a write operation fails.
         */
        virtual size_t write(core::buffer_view<uint8> buffer) = 0;

        /**
         * @brief Moves the read/write position within the stream.
         *
         * @param offset The offset to seek to.
         * @param dir The reference point for the offset (default is beginning of stream).
         * @return true if the seek was successful, false otherwise.
         */
        virtual bool seek(ssize_t offset, seek_dir dir = seek_dir::begin) noexcept = 0;

        /**
         * @brief Returns the current position within the stream.
         *
         * @return The current offset in bytes, or -1 on error.
         */
        [[nodiscard]] virtual ssize_t tell() const noexcept = 0;

        /**
         * @brief Returns the total size of the stream in bytes.
         *
         * @return Size of the stream.
         */
        [[nodiscard]] virtual size_t size() const noexcept = 0;

        /**
         * @brief Checks whether the end of the stream has been reached.
         *
         * @return true if end-of-stream is reached, false otherwise.
         */
        [[nodiscard]] virtual bool eos() const noexcept = 0;

        /**
         * @brief Reads all remaining bytes from the current position as text.
         *
         * Reads all bytes from the current position until the end of the source
         * and returns them as UTF-8 text.
         *
         * @return A string containing the asset's contents.
         *
         * @throws core::file_error If the read operation fails.
         */
        core::string read_text();

        /**
         * @brief Reads all remaining bytes from the current read position.
         *
         * Reads all bytes from the current position until the end of the source
         * and returns them as a contiguous byte buffer.
         *
         * The read cursor will be moved to the end after this call.
         *
         * @return A buffer containing the remaining raw bytes.
         *
         * @throws core::file_error If the read operation fails.
         */
        core::dynamic_buffer<uint8> read_binary();
    };

} // namespace tavros::assets
