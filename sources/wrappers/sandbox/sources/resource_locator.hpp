#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/noncopyable.hpp>
#include <filesystem>

namespace app
{

    class resource_locator : tavros::core::noncopyable
    {
    public:
        resource_locator(tavros::core::string_view assets_root);
        ~resource_locator() = default;

        // Resolve relative path to full absolute path
        std::filesystem::path resolve_texture(tavros::core::string_view relative_path) const;

    private:
        std::filesystem::path m_assets_root;
    };

} // namespace app
