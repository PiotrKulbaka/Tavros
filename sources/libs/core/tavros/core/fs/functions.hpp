#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/fixed_string.hpp>

#include <filesystem>
#include <stdexcept>

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

        /** @brief Replaces backslashes with forward slashes in-place. */
        inline void normalize_slashes(std::string& s)
        {
            std::replace(s.begin(), s.end(), '\\', '/');
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

        auto result = std::filesystem::absolute(detail::cstr_path(path, buf)).string();
        detail::normalize_slashes(result);

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

        auto result = std::filesystem::relative(detail::cstr_path(path, path_buf), detail::cstr_path(base, base_buf)).string();
        detail::normalize_slashes(result);

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: relative path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns the current working directory. */
    [[nodiscard]] inline fixed_path current_path()
    {
        auto result = std::filesystem::current_path().string();
        detail::normalize_slashes(result);

        if (result.size() >= k_fixed_path_size) {
            throw std::length_error("tavros::filesystem: current path exceeds " + std::to_string(k_fixed_path_size - 1) + " characters");
        }

        return fixed_path(core::string_view(result.data(), result.size()));
    }

    /** @brief Returns a path suitable for temporary files. */
    [[nodiscard]] inline fixed_path temp_directory_path()
    {
        auto result = std::filesystem::temp_directory_path().string();
        detail::normalize_slashes(result);

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

} // namespace tavros::filesystem
