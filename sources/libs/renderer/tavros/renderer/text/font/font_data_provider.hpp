#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/dynamic_buffer.hpp>

namespace tavros::renderer
{

    /**
     * @brief Interface for providing font binary data.
     *
     * Implementations of this interface are responsible for loading
     * font files from an external source.
     */
    class font_data_provider
    {
    public:
        /** Virtual destructor. */
        virtual ~font_data_provider() noexcept = default;

        /**
         * @brief Loads font data by path.
         *
         * Reads the font identified by the given path and returns its
         * entire binary contents.
         *
         * @param path Path identifying the font resource.
         * @return A buffer containing the raw font data.
         *
         * @throws Exception - the method may throw exceptions.
         */
        virtual core::dynamic_buffer<uint8> load(core::string_view path) = 0;
    };

} // namespace tavros::renderer
