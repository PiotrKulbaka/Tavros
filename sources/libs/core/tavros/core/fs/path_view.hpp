#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/filesystem/path.hpp>

namespace tavros::core
{

    class path_view
    {
    public:
        using string_view_type = std::string_view;

        path_view() = default;

        path_view(const std::filesystem::path& p)
            : m_str_view(p.native())
        {
        }

        path_view(const std::string& str)
            : m_str_view(str)
        {
        }

        path_view(const char* str)
            : m_str_view(str)
        {
        }

        string_view string() const noexcept
        {
            return m_str_view;
        }

    private:
        string_view m_str_view;
    };

} // namespace tavros::core
