#include "shader_preprocessor.hpp"

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <exception>

#include <set>

namespace
{
    static tavros::core::logger logger("shader_preprocessor");

} // namespace

namespace tavros::renderer
{


    shader_preprocessor::shader_preprocessor(core::shared_ptr<resources::resource_manager> rm) noexcept
        : m_resource_manager(std::move(rm))
    {
    }

    core::string shader_preprocessor::load_shader_source(core::string_view path)
    {
        load_r(path);

        struct part_t
        {
            int32 line_number = 0;
            core::string_view path;
            core::string_view text;
        };

        core::vector<part_t> parts;
        size_t total_size = 0;
        std::set<core::string, core::string_string_view_comparator> included;

        auto append_parts = [&](auto&& self, core::string_view cur_path) -> void {
            auto it = m_files.find(cur_path);
            TAV_ASSERT(it != m_files.end());

            const auto& file = it->second;

            auto text_count     = file.text_parts_count();
            auto includes_count = file.includes_count();

            for (size_t i = 0; i < includes_count; ++i) {
                auto part = file.text_part(i);
                total_size += part.size();
                total_size += cur_path.size();
                parts.push_back({file.line_number_for_text_part(i), cur_path, part});

                auto candidate = file.include_path(i);
                if (!included.contains(candidate)) {
                    included.insert(core::string(candidate));
                    self(self, file.include_path(i));
                }
            }

            auto part = file.text_part(text_count - 1);
            total_size += part.size();
            total_size += cur_path.size();
            parts.push_back({file.line_number_for_text_part(text_count - 1), cur_path, part});
        };

        append_parts(append_parts, path);

        total_size += 18 * parts.size();

        core::string result;
        result.reserve(total_size);

        
        static char buf[64] = {0};

        for (const auto& p : parts) {
            itoa(p.line_number, buf, 10);

            result.append("\n#line ");
            result.append(buf);
            result.append(" \"");
            result.append(p.path);
            result.append("\"\n");
            result.append(p.text);
        }

        return result;
    }

    void shader_preprocessor::load_r(core::string_view path)
    {
        if (m_files.contains(path)) {
            return;
        }

        core::vector<core::string_view> stack;
        stack.reserve(128);
        stack.push_back(path);

        while (!stack.empty()) {
            auto cur_path = stack.back();
            stack.pop_back();

            auto it = m_files.try_emplace(core::string(cur_path), load_file(cur_path), cur_path);

            for (size_t i = 0; i < it.first->second.includes_count(); ++i) {
                auto candidate_path = it.first->second.include_path(i);
                if (!m_files.contains(candidate_path)) {
                    stack.push_back(candidate_path);
                }
            }
        }
    }

    core::string shader_preprocessor::load_file(core::string_view path)
    {
        auto res = m_resource_manager->open(path, resources::resource_access::read_only);
        if (!res) {
            throw std::runtime_error("Failed to open shader file.");
        }

        auto* reader = res->reader();
        if (!reader) {
            throw std::runtime_error("Failed to open reader.");
        }

        auto [success, data] = reader->read_content();
        if (!success) {
            throw std::runtime_error("Failed to read shader file.");
        }

        return data;
    }

} // namespace tavros::renderer