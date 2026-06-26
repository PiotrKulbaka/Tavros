#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/shaders/shader_loader.hpp>

#include <tavros/core/resource/resource.hpp>
#include <tavros/renderer/material/material_desc.hpp>

#include <tavros/renderer/texture/texture.hpp>

namespace tavros::renderer
{

    /**
     * @brief GPU material resource.
     *
     * A material owns a compiled shader program and the corresponding graphics
     * pipeline object created from a @ref material_desc. It provides access to
     * the underlying pipeline and shader reflection data required for resource
     * binding and material instance creation.
     */
    class material
        : public core::basic_resource<material>,
          core::noncopyable
    {
    public:
        /**
         * @brief Creates a material from the specified description.
         *
         * Compiles or loads the required shader, creates the graphics pipeline,
         * and initializes all GPU resources needed by the material.
         *
         * @param gdevice Graphics device used to create GPU resources.
         * @param desc Material description used to configure the pipeline.
         * @param sl Shader loader used to load and compile shader programs.
         * @param msaa Number of MSAA samples used when creating the pipeline.
         * @param ds_format Depth-stencil format that the pipeline will be compatible with.
         */
        material(rhi::graphics_device* gdevice, const material_desc& desc, shader_loader& sl, uint32 msaa, rhi::pixel_format ds_format);

        /** @brief Moves a material. */
        material(material&&) noexcept;

        /** @brief Destroys the material and releases all owned GPU resources. */
        ~material() noexcept;

        /**
         * @brief Returns the graphics pipeline associated with this material.
         *
         * @return Handle to the graphics pipeline.
         */
        rhi::pipeline_handle gpu_pipeline() const noexcept;

        /**
         * @brief Returns shader reflection information for the material's shader.
         *
         * The returned pointer remains valid for the lifetime of the material.
         *
         * @return Pointer to the shader reflection interface, or nullptr if
         *         reflection data is unavailable.
         */
        const rhi::shader_reflect* shader_reflect() const noexcept;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        rhi::pipeline_handle  m_pipeline;
        rhi::shader_handle    m_shader;
    };

    /**
     * @brief Reference-counted handle to a @ref material resource.
     */
    using material_ref = core::basic_resource_ref<material>;

} // namespace tavros::renderer