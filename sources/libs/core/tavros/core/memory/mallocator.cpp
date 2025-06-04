#include <tavros/core/memory/mallocator.hpp>

#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/logger/logger.hpp>

#include <cstdlib>
#include <cstring>

#include <unordered_map>

using namespace tavros::core;

namespace
{
    tavros::core::logger s_logger("mallocator");
}

struct mallocator::impl
{
    struct allocation_info
    {
        size_t      size;
        const char* tag;
    };

    void* allocate(size_t size, const char* tag = nullptr)
    {
        auto* p = std::malloc(size);

        if (!p) {
            s_logger.error("Allocator cannot allocate memory `%s`.", (tag ? tag : ""));
            TAV_ASSERT(false);
            return nullptr;
        }

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
                s_logger.error("Attempting to free memory twice.");
                TAV_ASSERT(false);
            } else {
                s_logger.error("Attempting to free previously unallocated memory with this allocator.");
                TAV_ASSERT(false);
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

    unordered_map<void*, allocation_info> allocations;
    unordered_map<void*, allocation_info> released;
}; // struct mallocator::impl


mallocator::mallocator() = default;

mallocator::~mallocator()
{
    clear();
}

void* mallocator::allocate(size_t size, const char* tag)
{
    return m_impl->allocate(size, tag);
}

void mallocator::deallocate(void* ptr)
{
    m_impl->deallocate(ptr);
}

void mallocator::clear()
{
    m_impl->clear();
}
