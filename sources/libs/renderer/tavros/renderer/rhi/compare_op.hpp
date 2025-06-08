#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * Defines comparison functions used in operations such as depth testing or sampler comparison.
     * These functions determine whether a pixel or sampled value passes a comparison test.
     */
    enum class compare_op : uint8
    {
        off,           /// Comparison is disabled; used for samplers where depth comparison is not needed
        less,          /// Passes if the incoming value is less than the stored value
        equal,         /// Passes if the incoming value is equal to the stored value
        less_equal,    /// Passes if the incoming value is less than or equal to the stored value
        greater,       /// Passes if the incoming value is greater than the stored value
        greater_equal, /// Passes if the incoming value is greater than or equal to the stored value
        not_equal,     /// Passes if the incoming value is not equal to the stored value
        always,        /// Always passes the comparison test
    };

} // namespace tavros::renderer
