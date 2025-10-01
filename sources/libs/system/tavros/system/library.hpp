#pragma once

#include <tavros/core/prelude.hpp>

namespace tavros::system
{
    class library : core::noncopyable
    {
    public:
        library();
        library(core::string_view lib_name);
        ~library();

        bool is_open();
        bool open(core::string_view lib_name);
        void close();
        auto get_symbol(const char* symbol_name) -> void*;

    private:
        void* m_handle;
    };
} // namespace tavros::system
