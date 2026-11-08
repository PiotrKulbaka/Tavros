#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/check.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/profiler.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/debug/verify.hpp>
#include <tavros/core/debug/debug_only.hpp>

namespace tavros::debug
{
    void set_debug_path(core::string_view path) noexcept;

    core::string_view debug_path() noexcept;

    void save_to_file(core::buffer_view<const uint8> data, core::string_view name = "", core::string_view ext = "") noexcept;
} // namespace tavros::debug

namespace dbg = tavros::debug;
