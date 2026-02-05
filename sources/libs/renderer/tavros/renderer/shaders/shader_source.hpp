#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * @brief Represents a shader source with parsed text parts and include directives.
     *
     * This class owns the full shader source text and provides access to
     * individual text segments and include directives extracted from it.
     *
     * The object is non-copyable but movable. Parsing of include directives
     * is performed during construction.
     */
    class shader_source : core::noncopyable
    {
    public:
        /**
         * @brief Describes a continuous text region of the source.
         *
         * The text refers to a subrange of the original source and does not
         * own the underlying data.
         * The line number corresponds to the first line of this region
         * in the original source file.
         */
        struct part_type
        {
            int32             start_line_number = 0; /// Line number where this text part starts.
            core::string_view text;                  /// View into the original source text.
        };

    public:
        /**
         * @brief Constructs a shader source and parses include directives.
         *
         * @param source Full shader source text.
         * @param path   Path associated with this source (used for logging).
         *
         * @throws core::format_error
         *         If the shader source contains invalid syntax or malformed
         *         include directives.
         */
        explicit shader_source(core::string source, core::string_view path);

        /** Destructor. */
        ~shader_source() noexcept = default;

        /**
         * @brief Returns the number of text parts in the source.
         *
         * Text parts are continuous regions of the source that are located
         * between include directives.
         */
        size_t text_parts_count() const noexcept;

        /**
         * @brief Returns a text part with its source line information.
         *
         * A text part represents a continuous region of the original source
         * located between include directives.
         *
         * @param index Index of the text part.
         * @return Text view and the line number at which this part starts
         *         in the original source.
         */
        part_type text_part(size_t index) const noexcept;

        /**
         * @brief Returns the number of include directives found in the source.
         */
        size_t includes_count() const noexcept;

        /**
         * @brief Returns the include path for the specified include directive.
         *
         * @param index Index of the include directive.
         * @return View into the original source text representing the include path.
         */
        core::string_view include_path(size_t index) const noexcept;

        /**
         * @brief Returns the full original shader source.
         */
        core::string_view source() const noexcept;

    private:
        /**
         * @brief Scans the source and extracts include directives.
         *
         * @param path Path used for logging.
         * @return True if scanning succeeded, false otherwise.
         */
        bool scan(core::string_view path) noexcept;

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
