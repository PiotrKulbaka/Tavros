#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::text
{

    /**
     * @brief Interface for providing font data.
     *
     * Implementations of this interface are responsible for loading
     * font data from an external source.
     */
    class font_data_provider
    {
    public:
        /** Virtual destructor. */
        virtual ~font_data_provider() noexcept = default;

        /**
         * @brief Loads font data by path.
         *
         * @param path Path identifying the font file.
         * @return Full font data.
         *
         * @throws Exception - the method may throw exceptions.
         */
        virtual core::vector<uint8> load(core::string_view path) = 0;
    };

} // namespace tavros::text
