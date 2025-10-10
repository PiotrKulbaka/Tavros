#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/resources/seek_dir.hpp>
#include <tavros/core/memory/buffer.hpp>

namespace tavros::resources
{

    class resource_reader : core::noncopyable
    {
    public:
        virtual ~resource_reader() = default;
        virtual [[nodiscard]] bool   is_open() const = 0;
        virtual size_t               read(core::buffer_span<uint8> buffer) = 0;
        virtual bool                 seek(size_t offset, seek_dir dir = seek_dir::begin) = 0;
        virtual [[nodiscard]] size_t tell() const = 0;
        virtual [[nodiscard]] size_t size() const = 0;
        virtual [[nodiscard]] bool   eos() const = 0;
        virtual [[nodiscard]] bool   good() const = 0;
    };

} // namespace tavros::resources