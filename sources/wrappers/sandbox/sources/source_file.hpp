#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    class source_file : core::noncopyable
    {
    public:
        explicit source_file(core::string source, core::string_view path);

        source_file(source_file&&) noexcept = default;

        ~source_file() noexcept = default;

        source_file& operator=(source_file&&) noexcept = default;

        size_t text_parts_count() const noexcept;

        core::string_view text_part(size_t index) const noexcept;

        int32 line_number_for_text_part(size_t index) const noexcept;

        size_t includes_count() const noexcept;

        core::string_view include_path(size_t index) const noexcept;

        core::string_view source() const noexcept;

    private:
        bool scan(core::string_view path);

    private:
        struct include_info
        {
            int32  line_number = 0;
            size_t replace_begin = 0;
            size_t replace_size = 0;
            size_t path_begin = 0;
            size_t path_size = 0;
        };

        core::string m_source;

        core::vector<include_info> m_includes;
    };

} // namespace tavros::renderer