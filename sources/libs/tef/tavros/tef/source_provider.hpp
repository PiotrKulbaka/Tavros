#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::tef
{

    /**
     * @brief Interface for providing tef file source code.
     *
     * Implementations of this interface are responsible for loading
     * tef source text from an external source.
     */
    class tef_provider
    {
    public:
        /** Virtual destructor. */
        virtual ~tef_provider() noexcept = default;

        virtual core::string load(core::string_view path) = 0;
    };

} // namespace tavros::tef
