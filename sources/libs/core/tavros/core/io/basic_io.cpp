#include <tavros/core/io/basic_io.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::core
{

    string_view to_string(stream_state state) noexcept
    {
        switch (state) {
        case stream_state::good:
            return "good";
        case stream_state::eos:
            return "eos";
        case stream_state::bad:
            return "bad";
        default:
            TAV_UNREACHABLE();
        }
    }

    string_view to_string(seek_dir dir) noexcept
    {
        switch (dir) {
        case seek_dir::begin:
            return "begin";
        case seek_dir::current:
            return "current";
        case seek_dir::end:
            return "end";
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::core
