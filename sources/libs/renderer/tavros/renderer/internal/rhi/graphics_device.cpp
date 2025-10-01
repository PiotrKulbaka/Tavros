#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

namespace
{
    tavros::core::logger logger("graphics_device");
} // namespace

namespace tavros::renderer::rhi
{

    core::unique_ptr<graphics_device> graphics_device::create(rhi_backend backend)
    {
        switch (backend) {
        case rhi_backend::opengl:
            return core::make_unique<graphics_device_opengl>();
        case rhi_backend::vulkan:
            ::logger.error("Failed to create graphics device: rhi backend `vulkan` not implemented yet");
            return nullptr;
        case rhi_backend::directx12:
            ::logger.error("Failed to create graphics device: rhi backend `directx12` not implemented yet");
            return nullptr;
        case rhi_backend::metal:
            ::logger.error("Failed to create graphics device: rhi backend `metal` not implemented yet");
            return nullptr;
        default:
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::renderer::rhi
