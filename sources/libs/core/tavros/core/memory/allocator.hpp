#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    class allocator
    {
    public:
        virtual auto allocate(size_t size, const char* tag = nullptr) -> void* = 0;
        virtual void deallocate(void* ptr) = 0;
        virtual void clear() = 0;
    };

} // namespace tavros::core
