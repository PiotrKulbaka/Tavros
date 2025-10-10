#pragma once

#include <tavros/resources/resource_writer.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/io/ofstream.hpp>

namespace tavros::resources
{

    class file_writer : public resource_writer
    {
    public:
        explicit file_writer();
        ~file_writer() override;

        void open(core::string_view path, bool append = false);
        void close();

        bool   is_open() const override;
        size_t write(core::buffer_view<uint8> buffer) override;
        bool   seek(size_t offset, seek_dir dir = seek_dir::begin) override;
        size_t tell() const override;
        size_t size() const override;
        void   flush() override;
        bool   good() const override;

    private:
        mutable core::ofstream m_file;
        size_t                 m_size = 0;
    };

} // namespace tavros::resources