#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

namespace
{
    tavros::core::logger logger("graphics_device");
} // namespace

namespace tavros::renderer::rhi
{

    core::unique_ptr<graphics_device> graphics_device::create(render_backend_type backend)
    {
        switch (backend) {
        case render_backend_type::opengl:
            return core::make_unique<graphics_device_opengl>();
        default:
            break;
        }

        ::logger.fatal("Failed to create graphics device: rhi backend {} not implemented yet", backend);
        TAV_UNREACHABLE();
        return nullptr;
    }

} // namespace tavros::renderer::rhi
