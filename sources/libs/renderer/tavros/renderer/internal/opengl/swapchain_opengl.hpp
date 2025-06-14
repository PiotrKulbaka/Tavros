#pragma once

#include <tavros/core/memory/memory.hpp>

namespace tavros::renderer
{

    class swapchain;
    struct swapchain_desc;
    class graphics_device_opengl;

    core::shared_ptr<swapchain> create_swapchain_opengl(graphics_device_opengl* device, const swapchain_desc& desc, void* native_handle);

} // namespace tavros::renderer
