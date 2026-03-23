#include <tavros/core/io/basic_io.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::core
{

    string_view to_string(file_open_mode mode) noexcept
    {
        switch (mode) {
        case file_open_mode::open_existing:
            return "open_existing";
        case file_open_mode::create_new:
            return "create_new";
        case file_open_mode::open_or_create:
            return "open_or_create";
        case file_open_mode::truncate:
            return "truncate";
        case file_open_mode::append:
            return "append";
        default:
            TAV_UNREACHABLE();
        }
    }

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
