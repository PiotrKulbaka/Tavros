#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/resources/seek_dir.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::resources
{

    /**
     * @brief Abstract interface for reading data from a resource.
     *
     * This class provides a generic interface for reading bytes from a resource,
     * such as a file, memory buffer, or custom storage. It is non-copyable.
     */
    class resource_reader : core::noncopyable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~resource_reader() = default;

        /**
         * @brief Checks if the resource is currently open.
         * @return true if the resource is open, false otherwise.
         */
        virtual [[nodiscard]] bool is_open() const noexcept = 0;

        /**
         * @brief Reads data from the resource into the provided buffer.
         * @param buffer The buffer to store the read data.
         * @return The number of bytes actually read.
         */
        virtual size_t read(core::buffer_span<uint8> buffer) = 0;

        /**
         * @brief Moves the read position within the resource.
         * @param offset The offset to seek to.
         * @param dir The reference point for the offset (default is beginning of resource).
         * @return true if the seek was successful, false otherwise.
         */
        virtual bool seek(size_t offset, seek_dir dir = seek_dir::begin) = 0;

        /**
         * @brief Returns the current read position within the resource.
         * @return The current offset in bytes.
         */
        virtual [[nodiscard]] size_t tell() const = 0;

        /**
         * @brief Returns the total size of the resource in bytes.
         * @return Size of the resource.
         */
        virtual [[nodiscard]] size_t size() const = 0;

        /**
         * @brief Checks if the end of the resource has been reached.
         * @return true if end-of-stream is reached, false otherwise.
         */
        virtual [[nodiscard]] bool eos() const = 0;

        /**
         * @brief Checks if the resource is in a good state for reading.
         * @return true if the reader is valid and can perform operations, false otherwise.
         */
        virtual [[nodiscard]] bool good() const = 0;

        /**
         * @brief Reads the content of the file as a text string.
         *
         * @return core::string The contents of the file as a string.
         *
         * @throws file_error If there is a problem accessing or reading the file.
         * @throws std::ios_base::failure If an error occurs during stream operations
         *         (as thrown by std::ifstream).
         */
        virtual [[nodiscard]] core::string read_as_text() const = 0;

        /**
         * @brief Reads the content of the file as a binary data.
         *
         * @return core::vector<uint8> The contents of the file as a binary.
         *
         * @throws file_error If there is a problem accessing or reading the file.
         * @throws std::ios_base::failure If an error occurs during stream operations
         *         (as thrown by std::ifstream).
         */
        virtual [[nodiscard]] core::vector<uint8> read_as_binary() const = 0;
    };

} // namespace tavros::resources
