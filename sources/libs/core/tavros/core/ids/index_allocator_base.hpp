#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    /**
     * @class index_allocator_base
     * @brief Base class for all index allocators.
     *
     * This class defines the fundamental types and constants that all
     * index allocators can rely on. It does not implement allocation or
     * deallocation logic itself.
     *
     * @note  All indices are 0-based.
     */

    class index_allocator_base : noncopyable
    {
    protected:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;

    public:
        using index_t = uint32;
        constexpr static index_t invalid_index = UINT32_MAX;

    public:
        index_allocator_base() noexcept = default;

        ~index_allocator_base() noexcept = default;
    };

    constexpr static index_allocator_base::index_t invalid_index = index_allocator_base::invalid_index;

} // namespace tavros::core
