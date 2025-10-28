#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/resources/resource_reader.hpp>
#include <tavros/resources/resource_writer.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::resources
{

    /**
     * @brief Abstract representation of an open resource.
     *
     * A resource acts as a unified interface for accessing data sources such as files,
     * memory buffers, or virtual assets. It provides access to both reading and writing
     * operations through dedicated reader and writer interfaces.
     */
    class resource : core::noncopyable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~resource() = default;

        /**
         * @brief Returns a reader interface for the resource.
         * @return Pointer to a resource_reader, or nullptr if reading is not supported.
         */
        virtual [[nodiscard]] resource_reader* reader() = 0;

        /**
         * @brief Returns a writer interface for the resource.
         * @return Pointer to a resource_writer, or nullptr if writing is not supported.
         */
        virtual [[nodiscard]] resource_writer* writer() = 0;

        /**
         * @brief Returns the logical or physical path of the resource.
         * @return The resource path as a string view.
         */
        virtual [[nodiscard]] core::string_view path() const = 0;

        /**
         * @brief Checks whether the resource is currently open.
         * @return true if the resource is open, false otherwise.
         */
        virtual [[nodiscard]] bool is_open() const = 0;

        /**
         * @brief Closes the resource and releases associated handles or buffers.
         */
        virtual void close() = 0;
    };

} // namespace tavros::resources
