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

        bool   is_open() const noexcept override;
        size_t read(core::buffer_span<uint8> buffer) override;
        bool   seek(size_t offset, seek_dir dir = seek_dir::begin) override;
        size_t tell() const override;
        size_t size() const override;
        bool   eos() const override;
        bool   good() const override;

        core::string read_as_text() const override;

        core::vector<uint8> read_as_binary() const override;

    private:
        mutable core::ifstream m_file;
        core::string_view      m_path;
        size_t                 m_size = 0;
    };

} // namespace tavros::resources