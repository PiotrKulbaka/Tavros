#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/tef/lexer.hpp>
#include <tavros/tef/registry.hpp>
#include <tavros/tef/token.hpp>

namespace tavros::tef
{

    /**
     * @brief Result of a single file parse operation.
     *
     * On success, @p ok is true and the parsed nodes have been appended
     * to the document node passed to parser::parse(). On failure, @p ok
     * is false and @p error contains the first error encountered.
     */
    struct parse_result
    {
        bool                       success = false;
        core::vector<core::string> includes;
        core::string               error_str;
    };

    class parser : core::nonconstructable
    {
    public:
        static [[nodiscard]] parse_result parse(core::string_view source, node& doc);
    };

} // namespace tavros::tef
