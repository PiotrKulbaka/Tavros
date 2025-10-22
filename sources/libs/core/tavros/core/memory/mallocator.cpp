#include <tavros/core/memory/mallocator.hpp>

#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/logger/logger.hpp>

#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include <tavros/core/math/bitops.hpp>


namespace
{
    tavros::core::logger logger("mallocator");
}

namespace tavros::core
{

    mallocator::mallocator() = default;

    mallocator::~mallocator()
    {
        if (!m_allocations.empty()) {
            ::logger.warning("Called an allocator destructor with active allocations {} - possible memory leak", m_allocations.size());
        }

        clear();
    }

    void* mallocator::allocate(size_t size, size_t align, const char* tag)
    {
        if (!math::is_power_of_two(align)) {
            ::logger.error("Invalid alignment ({}) provided to allocate; must be a power of two", align);
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        void* ptr = _aligned_malloc(size, align);

        if (!ptr) {
            ::logger.error("Failed to allocate {} bytes (align {}), tag: {}", size, align, (tag ? tag : "(not provided)"));
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        // Sometimes the allocator may allocate already released memory pointer
        if (auto it = m_released.find(ptr); it != m_released.end()) {
            // So just rease released info
            m_released.erase(it);
        }
        // And add to the allocations
        m_allocations[ptr] = allocation_info{size, align, tag};

        memset(ptr, 0, size);

        return ptr;
    }

    void* mallocator::reallocate(void* ptr, size_t new_size, size_t align, const char* tag)
    {
        if (!math::is_power_of_two(align)) {
            ::logger.error("Invalid alignment ({}) provided to reallocate; must be a power of two", align);
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        // handle realloc semantics explicitly
        if (ptr == nullptr) {
            return allocate(new_size, align, tag);
        }

        if (new_size == 0) {
            deallocate(ptr);
            return nullptr;
        }

        if (auto it = m_allocations.find(ptr); it == m_allocations.end()) {
            ::logger.error("Attempt to reallocate an unknown pointer {}", fmt::ptr(ptr));
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        void* new_ptr = _aligned_realloc(ptr, new_size, align);
        if (!new_ptr) {
            ::logger.error("Failed to reallocate {} bytes (align {}), tag: {}", new_size, align, (tag ? tag : "(not provided)"));
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        // Update track data
        if (new_ptr == ptr) {
            // Just update old data
            auto old_info = m_allocations[new_ptr];
            m_allocations[new_ptr] = allocation_info{new_size, align, tag ? tag : old_info.tag};
        } else {
            // First of all check the released memory
            // Sometimes the allocator may allocate already released memory pointer
            if (auto it = m_released.find(new_ptr); it != m_released.end()) {
                // So just rease released info
                m_released.erase(it);
            }

            // And add new data
            m_allocations[new_ptr] = allocation_info{new_size, align, tag};

            // Migrate old data
            if (auto it = m_allocations.find(ptr); it != m_allocations.end()) {
                auto info = it->second;
                m_allocations.erase(it);
                m_released[ptr] = info;
            } else {
                // Something went wrong
                TAV_UNREACHABLE();
            }
        }

        return new_ptr;
    }

    void mallocator::deallocate(void* ptr)
    {
        if (!ptr) {
            return;
        }

        if (auto it = m_allocations.find(ptr); it != m_allocations.end()) {
            auto info = it->second;
            m_allocations.erase(it);
            m_released[ptr] = info;
            _aligned_free(ptr);
        } else if (auto it_released = m_released.find(ptr); it_released != m_released.end()) {
            ::logger.error("Double free detected for pointer {}", fmt::ptr(ptr));
            TAV_DEBUG_BREAK();
        } else {
            ::logger.error("Attempt to free an unknown pointer {}", fmt::ptr(ptr));
            TAV_DEBUG_BREAK();
        }
    }

    void mallocator::clear()
    {
        for (auto& [ptr, info] : m_allocations) {
            _aligned_free(ptr);
            m_released[ptr] = info;
        }
        m_allocations.clear();
    }

} // namespace tavros::core
