#pragma once

#include <tavros/core/io/stream_writer.hpp>
#include <tavros/core/noncopyable.hpp>
#include <fstream>

namespace tavros::core
{
    /**
     * @brief Binary writer backed by a file.
     *
     * Opens the file in binary mode on construction according to @p mode.
     * @throws file_error if the file cannot be opened.
     */
    class file_writer final : public basic_stream_writer, noncopyable
    {
    public:
        /**
         * @brief Opens the file at @p path for writing.
         *
         * @param path Path to the file.
         * @param mode Controls how the file is opened or created.
         *   - @c truncate       - open or create, clear existing content (default).
         *   - @c append         - open or create, seek to end before each write.
         *   - @c open_or_create - open if exists, create if not, cursor at begin.
         *   - @c create_new     - throws if file already exists.
         *   - @c open_existing  - throws if file does not exist.
         * @throws file_error if the file cannot be opened per the requested mode.
         */
        explicit file_writer(string_view path, file_open_mode mode = file_open_mode::truncate);

        ~file_writer() noexcept override = default;

        /** @brief Writes up to @p size bytes from @p src. Sets state to @c bad on failure. */
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
        mutable std::ofstream m_file;
        size_t                m_size;
    };

} // namespace tavros::core
