#pragma once

#include <tavros/resources/resource_reader.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/io/ifstream.hpp>

namespace tavros::resources
{

    class file_reader : public resource_reader
    {
    public:
        file_reader();
        ~file_reader() override;

        void open(core::string_view path);
        void close();

        bool   is_open() const override;
        size_t read(core::buffer_span<uint8> buffer) override;
        bool   seek(size_t offset, seek_dir dir = seek_dir::begin) override;
        size_t tell() const override;
        size_t size() const override;
        bool   eos() const override;
        bool   good() const override;

    private:
        mutable core::ifstream m_file;
        size_t                 m_size = 0;
    };

} // namespace tavros::resources