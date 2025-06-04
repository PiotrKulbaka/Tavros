#include <tavros/core/memory/zone_allocator.hpp>

#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/debug/assert.hpp>

#include <new>
#include <cstdlib>
#include <cstring>

using namespace tavros::core;

namespace
{
    struct alignas(32) zone_chunk
    {
        zone_chunk(size_t chunk_size)
            : next(nullptr)
            , prev(nullptr)
            , size(chunk_size - sizeof(zone_chunk))
            , free(true)
        {
            TAV_ASSERT(chunk_size == align(chunk_size));
            TAV_ASSERT(chunk_size > sizeof(zone_chunk));
        }

        ~zone_chunk()
        {
            extract();
        }

        void insert_this_after(zone_chunk* chunk)
        {
            TAV_ASSERT(!next);
            TAV_ASSERT(!prev);
            TAV_ASSERT(chunk);

            next = chunk->next;
            prev = chunk;
            chunk->next = this;
            if (next) {
                next->prev = this;
            }

            TAV_ASSERT(next && next->prev == this || !next);
            TAV_ASSERT(prev->next == this);
        }

        void extract()
        {
            if (prev) {
                prev->next = next;
            }
            if (next) {
                next->prev = prev;
            }
            TAV_ASSERT(prev && prev->next == next || !prev);
            TAV_ASSERT(next && next->prev == prev || !next);

            next = prev = nullptr;
        }

        size_t chunk_size()
        {
            return size + sizeof(zone_chunk);
        }

        uint8_t* memptr()
        {
            return reinterpret_cast<uint8_t*>(this) + sizeof(zone_chunk);
        }

        static size_t align(size_t val)
        {
            constexpr auto align_bits = sizeof(zone_chunk) - 1;
            return (val + align_bits) & ~align_bits;
        }

        static uint8_t* align(uint8_t* ptr)
        {
            return reinterpret_cast<uint8_t*>(align(reinterpret_cast<size_t>(ptr)));
        }

        zone_chunk* next; // next chunk
        zone_chunk* prev; // prev chunk
        size_t      size; // size of free memory to the right of the chunk
        bool        free; // is current chunk free
        uint32_t    tag;
    }; // struct zone_chunk
} // namespace


struct zone_allocator::impl
{
    impl(size_t zone_size)
    {
        TAV_ASSERT(zone_size >= 1024);
        TAV_ASSERT(zone_size == zone_chunk::align(zone_size));

        this->zone_size = zone_size;
        alloced_size = zone_size + sizeof(zone_chunk);
        mptr = static_cast<uint8_t*>(std::malloc(alloced_size));
        clear();
    }

    ~impl()
    {
        base->~zone_chunk();
        std::memset(mptr, 0, alloced_size);
        std::free(mptr);
    }

    zone_chunk* find_nearest(size_t size)
    {
        auto* chunk = pivot;
        while (chunk && (!chunk->free || chunk->size < size)) {
            chunk = chunk->next;
        }
        if (chunk) {
            return chunk;
        }

        chunk = base;
        while (chunk != pivot && (!chunk->free || chunk->size < size)) {
            chunk = chunk->next;
        }
        return chunk;
    }

    zone_chunk* allocate_chunk(size_t size)
    {
        size = zone_chunk::align(size);

        auto* chunk = find_nearest(size);
        if (!chunk) {
            return nullptr;
        }
        TAV_ASSERT(chunk->free);

        auto extra_size = chunk->size - size;
        if (extra_size > sizeof(zone_chunk)) {
            auto* mem = chunk->memptr();

            TAV_ASSERT(extra_size == zone_chunk::align(extra_size));
            TAV_ASSERT(mem == zone_chunk::align(mem));

            // add a free chunk to the right
            chunk->size = size;
            auto* extra_chunk = new (mem + chunk->size) zone_chunk(extra_size);
            extra_chunk->insert_this_after(chunk);
            pivot = extra_chunk;
        }

        chunk->free = false;
        chunk->tag = 0xdaf321ca;
        std::memset(chunk->memptr(), 0, chunk->size);
        return chunk;
    }

    void free_chunk(zone_chunk* chunk)
    {
        TAV_ASSERT(reinterpret_cast<uint8_t*>(chunk) >= reinterpret_cast<uint8_t*>(base));
        TAV_ASSERT(reinterpret_cast<uint8_t*>(chunk) < (reinterpret_cast<uint8_t*>(base) + zone_size));
        TAV_ASSERT(!chunk->free);
        TAV_ASSERT(chunk->tag == 0xdaf321ca);

        chunk->free = true;
        auto* next = chunk->next;
        if (next && next->free) {
            auto* nextnext = next->next;
            // remove the next chunk
            chunk->size += next->chunk_size();
            next->~zone_chunk();
        }

        auto* prev = chunk->prev;
        if (prev && prev->free) {
            next = chunk->next;
            prev->size += chunk->chunk_size();
            chunk->~zone_chunk();
            if (pivot == chunk) {
                pivot = prev;
            }
            chunk = prev;
        }

        pivot = chunk;
        std::memset(chunk->memptr(), 0, chunk->size);
    }

    void clear()
    {
        base = new (zone_chunk::align(mptr)) zone_chunk(zone_size);
        pivot = base;
    }

    uint8_t*                   mptr;
    size_t                     alloced_size;
    size_t                     zone_size;
    zone_chunk*                base; // the first chunk
    zone_chunk*                pivot;
    unordered_map<void*, bool> allocation_info;
}; // struct zone_allocator::impl


zone_allocator::zone_allocator(size_t zone_size)
    : m_impl(zone_size)
{
    TAV_ASSERT(zone_size == zone_chunk::align(zone_size));
}

zone_allocator::~zone_allocator()
{
    clear();
}

void* zone_allocator::allocate(size_t size, const char* tag)
{
    auto* chunk = m_impl->allocate_chunk(size);
    TAV_ASSERT(chunk);

    auto* p = static_cast<void*>(chunk->memptr());
    auto& map = m_impl->allocation_info;
    if (auto it = map.find(p); it != map.end()) {
        if (it->second) {
            TAV_ASSERT(false);
        } else {
            it->second = true;
        }
    } else {
        map[p] = true;
    }

    return chunk->memptr();
}

void zone_allocator::deallocate(void* ptr)
{
    auto* chunk = reinterpret_cast<zone_chunk*>(static_cast<uint8_t*>(ptr) - sizeof(zone_chunk));

    auto& map = m_impl->allocation_info;
    if (auto it = map.find(ptr); it != map.end()) {
        if (it->second) {
            it->second = false;
        } else {
            TAV_ASSERT(false);
        }
    } else {
        TAV_ASSERT(false);
    }

    m_impl->free_chunk(chunk);
}

void zone_allocator::clear()
{
    m_impl->clear();
}
