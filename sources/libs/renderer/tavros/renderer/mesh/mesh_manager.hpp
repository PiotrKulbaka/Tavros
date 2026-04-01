#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/assets/asset_manager.hpp>

namespace tavros::renderer
{

    struct texture_tag : tavros::core::handle_type_registration<0x2002>
    {
    };
    using texture_handle = tavros::core::handle_base<texture_tag>;

    struct mesh_tag : core::handle_type_registration<0x2002>
    {
    };
    using mesh_handle = core::handle_base<mesh_tag>;

    struct gpu_mesh_view : core::resource_base
    {
        rhi::bind_buffer_info    vertex_buffer;
        rhi::buffer_handle       index_buffer;
        uint32                   vertex_count = 0;
        uint32                   index_offset = 0;
        uint32                   index_count = 0;
        rhi::index_buffer_format index_format = rhi::index_buffer_format::u16;
    };


    class mesh_manager
    {
    public:
        using registry_type = core::resource_registry<gpu_mesh_view, mesh_tag>;
        using const_iterator = registry_type::const_iterator;

        struct mesh_upload
        {
            core::buffer_view<uint8> vertices;
            core::buffer_view<uint8> indices;
            rhi::index_buffer_format index_format = rhi::index_buffer_format::u16;
        };

    public:
        explicit mesh_manager(core::shared_ptr<assets::asset_manager> am) noexcept;
        ~mesh_manager() noexcept;

        void init(rhi::graphics_device* gdevice) noexcept;
        void shutdown() noexcept;

        mesh_handle load(core::string_view key, const mesh_upload& request);

        [[nodiscard]] const gpu_mesh_view* get(mesh_handle handle) const noexcept;

        [[nodiscard]] bool has_pending() const noexcept;

        void flush_pending(gpu_stage_buffer& stage, rhi::command_queue& cmd);

        void acquire(mesh_handle handle) noexcept;

        void release(mesh_handle handle);

        void clear() noexcept;

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_registry.begin();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return m_registry.cbegin();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_registry.end();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return m_registry.cend();
        }

    private:
        core::shared_ptr<assets::asset_manager> m_am;
        rhi::graphics_device*                   m_gdevice = nullptr;
        registry_type                           m_registry;

        rhi::buffer_handle m_vertex_buffer;
        rhi::buffer_handle m_index_buffer;

        uint32 m_vertex_cursor = 0; // следующая свободная вершина
        uint32 m_index_cursor = 0;  // следующий свободный индекс


        struct pending_upload
        {
            mesh_handle handle;
            size_t      stage_vertex_offset = 0; // offset в staging buffer для вершин
            size_t      stage_index_offset = 0;  // offset в staging buffer для индексов
            // core::vector<geometry_vertex>   vertices; // копия до flush
            // core::vector<uint32>            indices;
        };

        core::vector<pending_upload> m_pending;
    };

} // namespace tavros::renderer
