#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/assets/asset_stream.hpp>
#include <tavros/assets/asset_open_mode.hpp>

#include <fstream>

namespace tavros::assets
{

    class file_stream final : public asset_stream
    {
    public:
        file_stream(core::string_view path, asset_open_mode open_mode);

        ~file_stream() noexcept override;

        size_t read(core::buffer_span<uint8> buffer) override;

        size_t write(core::buffer_view<uint8> buffer) override;

        bool seek(ssize_t offset, seek_dir dir = seek_dir::begin) noexcept override;

        ssize_t tell() const noexcept override;

        size_t size() const noexcept override;

        bool eos() const noexcept override;

    private:
        mutable std::fstream m_file;
        core::string         m_path;
        size_t               m_size;
        asset_open_mode      m_open_mode;
    };

} // namespace tavros::assets