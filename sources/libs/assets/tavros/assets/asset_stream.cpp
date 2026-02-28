#include <tavros/assets/asset_stream.hpp>

#include <tavros/core/debug/assert.hpp>

namespace tavros::assets
{

    core::string asset_stream::read_text()
    {
        TAV_ASSERT(size() >= tell());
        size_t       sz = size() - tell();
        core::string str(sz, '\0');
        auto         rd_sz = read({reinterpret_cast<uint8*>(str.data()), str.size()});
        str.resize(rd_sz);
        return str;
    }

    core::vector<uint8> asset_stream::read_binary()
    {
        TAV_ASSERT(size() >= tell());
        size_t              sz = size() - tell();
        core::vector<uint8> bytes(sz, 0);
        read(bytes);
        return bytes;
    }

} // namespace tavros::assets
