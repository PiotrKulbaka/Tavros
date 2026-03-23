#pragma once

#include <tavros/core/io/stream_reader.hpp>
#include <tavros/core/noncopyable.hpp>
#include <fstream>

namespace tavros::core
{
    /**
     * @brief Binary reader backed by a file.
     *
     * Opens the file in binary mode on construction.
     * @throws file_error if the file cannot be opened.
     */
    class file_reader final : public basic_stream_reader, noncopyable
    {
    public:
        /**
         * @brief Opens the file at @p path for reading.
         *
         * @param path Path to the file.
         * @throws file_error if the file cannot be opened.
         */
        explicit file_reader(string_view path);

        ~file_reader() noexcept override = default;

        /** @brief Reads up to @p size bytes into @p dst. Sets state to @c eos or @c bad on failure. */
        size_t read(uint8* dst, size_t size) override;

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
        mutable std::ifstream m_file;
        size_t                m_size;
    };

} // namespace tavros::core