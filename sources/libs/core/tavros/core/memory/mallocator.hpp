#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/pimpl.hpp>

namespace tavros::core
{

    class mallocator : public allocator
    {
    public:
        mallocator();
        ~mallocator();

        auto allocate(size_t size, const char* tag = nullptr) -> void* override;
        void deallocate(void* ptr) override;
        void clear() override;

    private:
        struct impl;
        pimpl<impl, pointer_size * 20, pointer_size> m_impl;
    };

} // namespace tavros::core
