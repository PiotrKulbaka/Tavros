#include <tavros/core/memory/mallocator.hpp>

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <unordered_map>

using namespace tavros::core;

struct mallocator::impl
{
    struct allocation_info
    {
        size_t      size;
        const char* tag;
    };

    void* allocate(size_t size, const char* tag)
    {
        auto* p = std::malloc(size);
        allocations[p] = allocation_info{
            .size = size,
            .tag = tag
        };

        std::memset(p, 0, size);

        return p;
    }

    void deallocate(void* ptr)
    {
        if (auto it = allocations.find(ptr); it != allocations.end()) {
            auto info = it->second;
            allocations.erase(it);
            released[ptr] = info;
            std::memset(ptr, 0, info.size);
            std::free(ptr);
        } else {
            if (auto it = released.find(ptr); it != released.end()) {
                // TODO: write info to log instead assert
                // Attempt to free memory twice
                assert(false);
            } else {
                // TODO: write info to log instead assert
                assert(false);
            }
        }
    }

    void clear()
    {
        for (auto& it : allocations) {
            auto* p = it.first;
            std::memset(p, 0, it.second.size);
            std::free(p);
        }
        allocations.clear();
        released.clear();
    }

    std::unordered_map<void*, allocation_info> allocations;
    std::unordered_map<void*, allocation_info> released;
}; // struct mallocator::impl


/* mallocator::mallocator */
mallocator::mallocator() = default;

/* zone_allocator::~zone_allocator */
mallocator::~mallocator()
{
    clear();
}

/* mallocator::allocate */
auto mallocator::allocate(size_t size, const char* tag) -> void*
{
    return m_impl->allocate(size, tag);
}

/* mallocator::deallocate */
void mallocator::deallocate(void* ptr)
{
    m_impl->deallocate(ptr);
}

/* mallocator::clear */
void mallocator::clear()
{
    m_impl->clear();
}
