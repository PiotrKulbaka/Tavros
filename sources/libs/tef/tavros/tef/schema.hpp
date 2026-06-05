#pragma once

#include <tavros/tef/node.hpp>
#include <tavros/tef/conv.hpp>
#include <tavros/core/logger/diagnostics.hpp>

#include <functional>
#include <optional>
#include <vector>
#include <string_view>

namespace tavros::tef
{

    /**
     * @brief Defines the mapping between a C++ type and its node representation.
     *
     * Specialize this struct for each type that needs to participate in TEF serialization.
     *
     * @tparam T Type to map.
     */
    template<class T>
    struct schema
    {
        /**
         * @brief Converts a value into its IR node representation.
         *
         * Traverses the fields of @p in and writes them as child nodes of @p n.
         * Errors encountered during conversion are appended to @p ds.
         *
         * @tparam T        Type to serialize. Must have a @ref schema specialization.
         *
         * @param n         Target node to populate. Must not be null.
         * @param in        Source value to read from.
         * @param ds        Diagnostics collector. Appended to, not cleared.
         */
        static void serialize(node* n, const T& in, core::diagnostics& ds) noexcept;

        /**
         * @brief Populates a value from its IR node representation.
         *
         * Reads child nodes of @p n and writes the result into @p out.
         * Missing required fields and type mismatches are reported via @p ds.
         *
         * @invariant If @p ds contains no errors after the call, @p out is fully valid.
         *
         * @tparam T        Type to deserialize. Must have a @ref schema specialization.
         *
         * @param n         Source node to read from. Must not be null.
         * @param out       Target value to populate.
         * @param ds        Diagnostics collector. Appended to, not cleared.
         */
        static void deserialize(const node* n, T& out, core::diagnostics& ds) noexcept;
    };

} // namespace tavros::tef
