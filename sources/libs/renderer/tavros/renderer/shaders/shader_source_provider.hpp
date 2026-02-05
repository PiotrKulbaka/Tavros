#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::renderer
{

    /**
     * @brief Interface for providing shader source code.
     *
     * Implementations of this interface are responsible for loading
     * shader source text from an external source.
     */
    class shader_source_provider
    {
    public:
        /** Virtual destructor. */
        virtual ~shader_source_provider() noexcept = default;

        /**
         * @brief Loads shader source text by path.
         *
         * @param path Path identifying the shader source.
         * @return Full shader source text.
         *
         * @throws Exception - the method may throw exceptions.
         */
        virtual core::string load(core::string_view path) = 0;
    };

} // namespace tavros::renderer
