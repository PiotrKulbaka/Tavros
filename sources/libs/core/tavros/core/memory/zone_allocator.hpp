#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/pimpl.hpp>

namespace tavros::core
{

    class zone_allocator final : public allocator
    {
    public:
        zone_allocator(size_t zone_size);
        ~zone_allocator();

        void* allocate(size_t size, const char* tag = nullptr) override;
        void  deallocate(void* ptr) override;
        void  clear() override;

    private:
        struct impl;
        pimpl<impl, pointer_size * 15, pointer_size> m_impl;
    }; // class zone_allocator

} // namespace tavros::core
