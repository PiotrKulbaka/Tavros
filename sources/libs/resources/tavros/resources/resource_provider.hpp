#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/resources/resource.hpp>

namespace tavros::resources
{

    /**
     * @brief Abstract interface for accessing and managing resources.
     *
     * A resource provider defines how resources (such as files, memory assets,
     * or virtual paths) are accessed for reading and writing. Concrete implementations
     * can provide access from the filesystem, memory archives, network sources, etc.
     */
    class resource_provider
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~resource_provider() = default;

        /**
         * @brief Checks if the provider allows reading from the specified path.
         * @param path The path to the resource.
         * @return true if the resource is readable, false otherwise.
         */
        virtual [[nodiscard]] bool available_for_read(core::string_view path) const = 0;

        /**
         * @brief Checks if the provider allows writing to the specified path.
         * @param path The path to the resource.
         * @return true if the resource is writable, false otherwise.
         */
        virtual [[nodiscard]] bool available_for_write(core::string_view path) const = 0;

        /**
         * @brief Checks whether a resource exists at the specified path.
         * @param path The path to the resource.
         * @return true if the resource exists, false otherwise.
         */
        virtual [[nodiscard]] bool exists(core::string_view path) const = 0;

        /**
         * @brief Opens the specified resource for reading or writing.
         * @param path The path to the resource.
         * @return A shared pointer to the opened resource, or nullptr if opening failed.
         */
        virtual [[nodiscard]] core::shared_ptr<resource> open(core::string_view path) = 0;
    };

} // namespace tavros::resources
