#pragma once

#include <tavros/core/containers/map.hpp>
#include <tavros/core/utils/string_string_view_comparator.hpp>
#include <tavros/renderer/shaders/shader_source.hpp>
#include <tavros/renderer/shaders/shader_source_provider.hpp>
#include <tavros/core/memory/memory.hpp>

namespace tavros::renderer
{

    /**
     * @brief Supported shader languages / GLSL versions (core profile).
     */
    enum class shader_language
    {
        glsl_430, /// GLSL version 4.30
        glsl_460, /// GLSL version 4.60
        glsl_330, /// GLSL version 3.30
    };

    /**
     * @brief Arguments for loading a shader.
     *
     * Contains settings that influence shader preprocessing and compilation.
     */
    struct shader_load_args
    {
        /**
         * Target shader language / GLSL version. Defaults to glsl_460.
         */
        shader_language lang = shader_language::glsl_460;

        /**
         * @brief Preprocessor definitions to inject into the shader.
         *
         * Multiple definitions are separated by semicolons (';').
         * Example: "NUM_LIGHTS 4;USE_SHADOWS;DEBUG"
         */
        core::string_view defines;
    };


    /**
     * @brief Loads and preprocesses shader sources.
     *
     * Resolves shader source files using the provided shader_source_provider,
     * processes include directives and returns the final preprocessed source.
     */
    class shader_loader
    {
    public:
        /**
         * @brief Constructs a shader loader.
         *
         * @param sp Shader source provider used to load shader sources.
         */
        shader_loader(core::unique_ptr<shader_source_provider> sp) noexcept;

        /** Move constructor. */
        shader_loader(shader_loader&&) noexcept = default;

        /** Destructor. */
        ~shader_loader() noexcept = default;

        /**
         * @brief Loads and preprocesses a shader source.
         *
         * Resolves the shader source using the provided shader_source_provider,
         * processes include directives, injects shader directives (version,
         * defines, precision, extensions) and returns the final preprocessed source.
         *
         * @param path Path identifying the shader source.
         * @param args Shader loading options.
         * @return Preprocessed shader source text.
         *
         * @throws Exception
         *         If the underlying shader_source_provider::load() throws.
         * @throws core::format_error
         *         If the shader source has an invalid format.
         */
        core::string load(core::string_view path, const shader_load_args& args);

        /**
         * @brief Clears the internal cache of loaded shader sources.
         *
         * After calling this, all previously loaded shader files are removed
         * from the cache. Future calls to `load()` will reload sources from
         * the shader_source_provider.
         */
        void clean() noexcept;

    private:
        void load_r(core::string_view path);

        core::string make_intro(const shader_load_args& args) const;

        core::string preprocess(core::string_view path, const shader_load_args& args) const;

    private:
        core::unique_ptr<shader_source_provider> m_shaders_provider;

        core::map<core::string, shader_source, core::string_string_view_comparator> m_files;
    };

} // namespace tavros::renderer
