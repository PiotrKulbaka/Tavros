#pragma once

#include <tavros/core/io/stream_writer.hpp>
#include <tavros/core/noncopyable.hpp>
#include <fstream>

namespace tavros::core
{
    /**
     * @brief File-backed binary stream writer.
     *
     * Writes binary data to a file using @c std::fstream.
     * Supports seeking, telling, and size queries.
     *
     * Opens the file on construction. Throws @c file_error if the file cannot be opened.
     *
     * @see basic_stream_writer
     */
    class file_writer : public basic_stream_writer, noncopyable
    {
    public:
        /**
         * @brief Opens the file at @p path for writing.
         *
         * @param path Path to the file.
         * @throws file_error if the file cannot be opened.
         */
        explicit file_writer(string_view path);

        ~file_writer() override = default;

        /**
         * @brief Writes @p size bytes from @p src to the file.
         *
         * Returns the number of bytes actually written.
         * Sets state to @c bad and throws @c file_error on I/O failure.
         *
         * @throws file_error on I/O failure.
         */
        size_t write(const uint8* src, size_t size) override;

        /** @brief Returns true for a file backend. */
        [[nodiscard]] bool seekable() const noexcept override;

        /** @brief Seeks to a position in the file. Returns false on failure. */
        bool seek(ssize_t offset, seek_dir dir = seek_dir::begin) noexcept override;

        /** @brief Returns the current position in the file, or -1 on failure. */
        [[nodiscard]] ssize_t tell() const noexcept override;

        /** @brief Returns the total size of the file in bytes. */
        [[nodiscard]] size_t size() const noexcept override;

    private:
        // mutable for tellg function
        mutable std::fstream m_file;
        size_t               m_size;
    };

} // namespace tavros::core
