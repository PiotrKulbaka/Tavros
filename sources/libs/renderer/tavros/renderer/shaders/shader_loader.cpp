#include <tavros/renderer/shaders/shader_loader.hpp>

#include <tavros/core/containers/set.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace
{

    tavros::core::string_view to_string(tavros::renderer::shader_language lang)
    {
        using enum tavros::renderer::shader_language;
        switch (lang) {
        case glsl_430:
            return "430";
        case glsl_460:
            return "460";
        case glsl_330:
            return "330";
        default:
            TAV_UNREACHABLE();
            break;
        }
    }

} // namespace

namespace tavros::renderer
{

    shader_loader::shader_loader(core::unique_ptr<shader_source_provider> sp) noexcept
        : m_shaders_provider(std::move(sp))
    {
    }

    core::string shader_loader::load(core::string_view path, const shader_load_args& args)
    {
        load_r(path);
        return preprocess(path, args);
    }


    void shader_loader::clean() noexcept
    {
        m_files.clear();
    }

    void shader_loader::load_r(core::string_view path)
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

            auto src = m_shaders_provider->load(cur_path);
            auto it = m_files.try_emplace(core::string(cur_path), std::move(src), cur_path);

            for (size_t i = 0; i < it.first->second.includes_count(); ++i) {
                auto candidate_path = it.first->second.include_path(i);
                if (!m_files.contains(candidate_path)) {
                    stack.push_back(candidate_path);
                }
            }
        }
    }

    core::string shader_loader::make_intro(const shader_load_args& args) const
    {
        constexpr auto hash_ver_sv = core::string_view("#version ");
        constexpr auto core_sv = core::string_view(" core\n");
        constexpr auto define_sv = core::string_view("#define ");
        constexpr auto define_end_sv = core::string_view("\n");

        size_t defines_number = args.defines.size() == 0 ? 0 : std::count(args.defines.begin(), args.defines.end(), ';') + 1;
        size_t required_capacity = hash_ver_sv.size() + 3 + core_sv.size() + (define_sv.size() + define_end_sv.size()) * defines_number + args.defines.size();

        core::string result;
        result.reserve(required_capacity);

        result.append(hash_ver_sv);
        result.append(to_string(args.lang));
        result.append(core_sv);

        // Add defines
        const auto* fwd = args.defines.data();
        const auto* beg = args.defines.data();
        const auto* end = args.defines.data() + args.defines.size();
        while (fwd < end) {
            while (fwd < end && *fwd != ';') {
                ++fwd;
            }

            result.append(define_sv);
            result.append(core::string_view(beg, fwd));
            result.append(define_end_sv);

            beg = ++fwd;
        }

        return result;
    }

    core::string shader_loader::preprocess(core::string_view path, const shader_load_args& args) const
    {
        struct part_t
        {
            int32             line_number = 0;
            core::string_view path;
            core::string_view text;
        };

        core::vector<part_t> parts;
        size_t               total_size = 0;

        std::set<core::string, core::string_string_view_comparator> included;

        auto append_parts = [&](auto&& self, core::string_view cur_path) -> void {
            auto it = m_files.find(cur_path);
            TAV_ASSERT(it != m_files.end());

            const auto& file = it->second;

            auto text_count = file.text_parts_count();
            auto includes_count = file.includes_count();

            for (size_t i = 0; i < includes_count; ++i) {
                auto part = file.text_part(i);
                total_size += cur_path.size() + part.text.size();

                parts.push_back({part.start_line_number, cur_path, part.text});

                auto candidate = file.include_path(i);
                if (!included.contains(candidate)) {
                    included.insert(core::string(candidate));
                    self(self, file.include_path(i));
                }
            }

            auto part = file.text_part(text_count - 1);
            total_size += cur_path.size() + part.text.size();
            parts.push_back({part.start_line_number, cur_path, part.text});
        };

        append_parts(append_parts, path);

        constexpr auto   n_hash_line_sp = core::string_view("\n#line ");
        constexpr auto   sp_dq = core::string_view(" \"");
        constexpr auto   dq_n = core::string_view("\"\n");
        constexpr size_t max_number_size = 10;
        constexpr size_t additional_size = n_hash_line_sp.size() + sp_dq.size() + dq_n.size() + max_number_size;
        total_size += additional_size * parts.size();


        auto intro = make_intro(args);

        core::string result;
        result.reserve(total_size + intro.size());

        result.append(intro);

        static char ascii_number[32] = {0};

        for (const auto& p : parts) {
            itoa(p.line_number, ascii_number, 10);

            result.append(n_hash_line_sp);
            result.append(ascii_number);
            result.append(sp_dq);
            result.append(p.path);
            result.append(dq_n);
            result.append(p.text);
        }

        return result;
    }

} // namespace tavros::renderer
