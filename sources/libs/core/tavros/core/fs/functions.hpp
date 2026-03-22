#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/core/string_view.hpp>

#include <filesystem>
#include <stdexcept>
#include <cctype>

namespace tavros::filesystem
{
    inline constexpr size_t k_fixed_path_size = 256;
    using fixed_path = core::fixed_string<k_fixed_path_size>;

    namespace detail
    {
        /**
         * @brief Returns C-string path, using buffer if needed.
         *
         * Avoids allocation if input is already null-terminated.
         * Throws if path exceeds fixed buffer size.
         */
        inline const char* cstr_path(core::string_view path, fixed_path& buffer)
        {
            if (path.size() >= k_fixed_path_size) {
                throw std::length_error("tavros::filesystem: path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
            }
            if (path.data()[path.size()] == '\0') {
                return path.data();
            }
            buffer = fixed_path(path);
            return buffer.data();
        }
    } // namespace detail

    /** @brief Returns true if the path refers to an existing file system object. */
    [[nodiscard]] inline bool exists(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::exists(detail::cstr_path(path, buf));
    }

    /** @brief Returns true if the path refers to a regular file. */
    [[nodiscard]] inline bool is_file(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::is_regular_file(detail::cstr_path(path, buf));
    }

    /** @brief Returns true if the path refers to a directory. */
    [[nodiscard]] inline bool is_directory(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::is_directory(detail::cstr_path(path, buf));
    }

    /** @brief Returns true if the path refers to an empty file or directory. */
    [[nodiscard]] inline bool is_empty(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::is_empty(detail::cstr_path(path, buf));
    }

    /** @brief Returns the size of a file in bytes. */
    [[nodiscard]] inline uint64 file_size(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::file_size(detail::cstr_path(path, buf));
    }

    /** @brief Returns the absolute path. */
    [[nodiscard]] inline fixed_path absolute(core::string_view path)
    {
        fixed_path buf;

        auto result = std::filesystem::absolute(detail::cstr_path(path, buf))
                          .lexically_normal()
                          .generic_string();

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: absolute path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns a relative path from base to path. */
    [[nodiscard]] inline fixed_path relative(core::string_view path, core::string_view base)
    {
        fixed_path path_buf;
        fixed_path base_buf;

        auto result = std::filesystem::relative(detail::cstr_path(path, path_buf), detail::cstr_path(base, base_buf))
                          .lexically_normal()
                          .generic_string();

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: relative path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns the current working directory. */
    [[nodiscard]] inline fixed_path current_path()
    {
        auto result = std::filesystem::current_path()
                          .lexically_normal()
                          .generic_string();

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: current path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns a path suitable for temporary files. */
    [[nodiscard]] inline fixed_path temp_directory_path()
    {
        auto result = std::filesystem::temp_directory_path()
                          .lexically_normal()
                          .generic_string();

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: temp path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Creates a single directory. Returns true if created. */
    inline bool create_directory(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::create_directory(detail::cstr_path(path, buf));
    }

    /** @brief Creates directories recursively. Returns true if any created. */
    inline bool create_directories(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::create_directories(detail::cstr_path(path, buf));
    }

    /** @brief Removes a file or empty directory.Returns true if removed. */
    inline bool remove(core::string_view path)
    {
        fixed_path buf;
        return std::filesystem::remove(detail::cstr_path(path, buf));
    }

    /** @brief Removes a file or directory recursively.Returns number of removed entries. */
    inline uint64 remove_all(core::string_view path)
    {
        fixed_path buf;
        return static_cast<uint64>(std::filesystem::remove_all(detail::cstr_path(path, buf)));
    }

    /** @brief Renames or moves a file or directory. */
    inline void rename(core::string_view from, core::string_view to)
    {
        fixed_path from_buf;
        fixed_path to_buf;
        std::filesystem::rename(
            detail::cstr_path(from, from_buf),
            detail::cstr_path(to, to_buf)
        );
    }

    /** @brief Copies a file. Optionally overwrites the destination. */
    inline bool copy_file(core::string_view from, core::string_view to, bool overwrite = false)
    {
        fixed_path from_buf;
        fixed_path to_buf;
        auto       options = overwrite
                               ? std::filesystem::copy_options::overwrite_existing
                               : std::filesystem::copy_options::none;
        return std::filesystem::copy_file(
            detail::cstr_path(from, from_buf),
            detail::cstr_path(to, to_buf),
            options
        );
    }

    /** @brief Returns the filename component(everything after the last / or \). */
    [[nodiscard]] inline core::string_view filename(core::string_view path) noexcept
    {
        auto pos = path.find_last_of('/');
        if (pos == core::string_view::npos) {
            pos = path.find_last_of('\\'); // fallback
            if (pos == core::string_view::npos) {
                return path;
            }
        }
        return path.substr(pos + 1);
    }

    /**
     * @brief Returns the extension component(including the dot, e.g. ".txt").
     * Returns empty if no extension or filename starts with a dot and has no other dot.
     */
    [[nodiscard]] inline core::string_view extension(core::string_view path) noexcept
    {
        auto name = filename(path);
        if (name.empty() || name == "." || name == "..") {
            return {};
        }
        // skip leading dot (hidden files: ".gitignore" has no extension)
        auto pos = name.rfind('.');
        if (pos == core::string_view::npos || pos == 0) {
            return {};
        }
        return name.substr(pos);
    }

    /** @brief Returns the stem component(filename without extension). */
    [[nodiscard]] inline core::string_view stem(core::string_view path) noexcept
    {
        auto name = filename(path);
        auto ext = extension(path);
        return name.substr(0, name.size() - ext.size());
    }

    /**
     8 @brief Returns the parent path(everything before the last / or \).
     * Returns empty for single-component paths with no separator.
     */
    [[nodiscard]] inline core::string_view parent_path(core::string_view path) noexcept
    {
        if (path.empty()) {
            return {};
        }

        // strip trailing slashes
        auto last = path.size() - 1;
        while (last > 0 && (path[last] == '/' || path[last] == '\\')) {
            --last;
        }

        // preserve root slash: "/foo" -> "/"
        if (last == 0) {
            if (path[0] == '/' || path[0] == '\\') {
                return path.substr(0, 1);
            }
            return {};
        }

        auto pos = path.find_last_of('/', last);
        if (pos == core::string_view::npos) {
            pos = path.find_last_of('\\', last);
            if (pos == core::string_view::npos) {
                return {};
            }
        }

        // strip slashes
        while (pos > 0 && (path[pos] == '/' || path[pos] == '\\')) {
            --pos;
        }

        // preserve root slash: "/foo" -> "/"
        if (pos == 0 && (path[pos] == '/' || path[pos] == '\\')) {
            return path.substr(0, 1);
        }
        return path.substr(0, pos + 1);
    }

    /** @brief Returns true if the path is absolute. */
    [[nodiscard]] inline bool is_absolute(core::string_view path) noexcept
    {
        if (path.empty()) {
            return false;
        }
        // Unix: /foo
        if (path[0] == '/' || path[0] == '\\') {
            return true;
        }
        // Windows (or URL): C:/foo (resource://foo)
        auto        sz = path.size();
        const auto* p = path.data();
        while (sz > 0 && std::isalnum(static_cast<int>(*p))) {
            --sz;
            ++p;
        }

        if (sz != path.size() && sz > 1 && (p[0] == ':') && (p[1] == '/' || p[1] == '\\')) {
            return true;
        }

        return false;
    }

    /** @brief Returns true if the path is relative. */
    [[nodiscard]] inline bool is_relative(core::string_view path) noexcept
    {
        return !is_absolute(path);
    }

    /** @brief Returns true if the path has a non-empty filename component. */
    [[nodiscard]] inline bool has_filename(core::string_view path) noexcept
    {
        return !filename(path).empty();
    }

    /** @brief Returns true if the path has a non-empty stem component. */
    [[nodiscard]] inline bool has_stem(core::string_view path) noexcept
    {
        return !stem(path).empty();
    }

    /** @brief Returns true if the path has an extension. */
    [[nodiscard]] inline bool has_extension(core::string_view path) noexcept
    {
        return !extension(path).empty();
    }

    /** @brief Returns true if the path has a parent path. */
    [[nodiscard]] inline bool has_parent_path(core::string_view path) noexcept
    {
        return !parent_path(path).empty();
    }

    /** @brief Returns a normalized copy of @p path, resolving @c . and @c .., replacing @c \\ with @c / . */
    [[nodiscard]] inline core::string normalize_path(const core::string& path)
    {
        auto result = std::filesystem::path(path.data())
                          .lexically_normal()
                          .generic_string();
        return result;
    }

    /** @brief Returns a normalized copy of @p path, resolving @c . and @c .., replacing @c \\ with @c / . */
    template<size_t N>
    [[nodiscard]] inline core::fixed_string<N> normalize_path(const core::fixed_string<N>& path)
    {
        auto result = std::filesystem::path(path.data())
                          .lexically_normal()
                          .generic_string();
        return core::fixed_string<N>(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns a normalized copy of @p path, resolving @c . and @c .., replacing @c \\ with @c / . */
    template<size_t N>
    inline void normalize_path(core::string_view path, core::fixed_string<N>& out)
    {
        auto result = std::filesystem::path(path.data())
                          .lexically_normal()
                          .generic_string();
        out = core::fixed_string<N>(core::string_view(result.data(), result.size()));
    }

    /** @brief Normalizes @p path in place, resolving @c . and @c .., replacing @c \\ with @c / . */
    inline void normalize_path_inplace(core::string& path)
    {
        path = std::move(std::filesystem::path(path.data()).lexically_normal().generic_string());
    }

    /** @brief Normalizes @p path in place, resolving @c . and @c .., replacing @c \\ with @c / . */
    template<size_t N>
    inline void normalize_path_inplace(core::fixed_string<N>& path)
    {
        auto result = std::filesystem::path(path.data())
                          .lexically_normal()
                          .generic_string();
        path = result;
    }

} // namespace tavros::filesystem
