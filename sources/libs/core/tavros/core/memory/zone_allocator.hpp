#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/pimpl.hpp>
#include <tavros/core/defines.hpp>

namespace tavros::core
{

    class zone_allocator final : public allocator
    {
    public:
        zone_allocator(size_t zone_size);
        ~zone_allocator() override;

        void* allocate(size_t size, size_t align, const char* tag = nullptr) override;
        void* reallocate(void* ptr, size_t new_size, size_t align, const char* tag = nullptr) override;
        void  deallocate(void* ptr) override;
        void  clear() override;

    private:
        struct impl;
        static constexpr size_t              impl_size = TAV_DEBUG ? pointer_size * 15 : pointer_size * 13;
        pimpl<impl, impl_size, pointer_size> m_impl;
    }; // class zone_allocator

} // namespace tavros::core
