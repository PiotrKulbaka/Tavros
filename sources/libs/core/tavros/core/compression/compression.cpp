#include <tavros/core/compression/compression.hpp>

#include <tavros/core/logger/logger.hpp>
#include <zlib/zlib.h>

namespace
{
    tavros::core::logger logger("compression");
}

namespace tavros::core
{

    bool uncompress_data(buffer_view<uint8> compressed_data, buffer_span<uint8> output)
    {
        uLongf dest_len = output.size();
        auto   res = uncompress(output.data(), &dest_len, compressed_data.data(), compressed_data.size());

        switch (res) {
        case Z_OK:
            return true;

        case Z_MEM_ERROR:
            ::logger.error("Failed to uncompress: not enough memory");
            return false;

        case Z_BUF_ERROR:
            ::logger.error("Failed to uncompress: output buffer too small");
            return false;

        case Z_DATA_ERROR:
            ::logger.error("Failed to uncompress: corrupted data");
            return false;

        default:
            ::logger.fatal("Unknown zlib error {}", res);
            return false;
        }
    }

} // namespace tavros::core