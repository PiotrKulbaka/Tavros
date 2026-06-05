#pragma once

#include <tavros/core/ids/l2_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>
#include <tavros/core/ids/l4_bitmap_index_allocator.hpp>

namespace tavros::core
{

    // clang-format off
    template<size_t N>
    using bitmap_index_allocator =
        std::conditional_t<(N <= l2_bitmap_index_allocator::k_capacity),
            l2_bitmap_index_allocator,
            std::conditional_t<(N <= l3_bitmap_index_allocator::k_capacity),
                l3_bitmap_index_allocator,
                l4_bitmap_index_allocator
            >
        >;
    // clang-format on

} // namespace tavros::core
