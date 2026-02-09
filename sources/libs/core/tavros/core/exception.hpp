#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <stdexcept>

namespace tavros::core
{

    /**
     * @brief Describes the category of a file-related error.
     *
     * Used by @ref file_error to provide structured information
     * about the reason of a file system failure.
     */
    enum class file_error_tag
    {
        not_found,         /// File or directory does not exist at the specified path.
        permission_denied, /// Access to the file is denied due to insufficient permissions.
        open_failed,       /// File could not be opened for reading or writing.
        read_error,        /// An error occurred while reading from the file.
        write_error,       /// An error occurred while writing to the file.
        invalid_path,      /// The provided file path is invalid or malformed.
        invalid_argument,  /// The argument passed is invalid.
        is_directory,      /// The path refers to a directory where a file was expected.
        other,             /// Anything else, less important
    };

    /// Returns a string representation of the tag
    string_view to_string(file_error_tag tag) noexcept;

    /**
     * @brief Exception describing a file system related error.
     *
     * Represents failures occurring during file access or I/O operations.
     * Provides both a human-readable message and a structured error tag,
     * as well as the path associated with the failure.
     */
    class file_error final : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a file error with additional context.
         *
         * @param tag  Classification of the file error.
         * @param path Path associated with the error.
         * @param msg  Error description.
         */
        explicit file_error(file_error_tag tag, core::string_view path, const core::string& msg)
            : std::runtime_error(msg)
            , m_path(core::string(path))
            , m_tag(tag)
        {
        }

        /** Copy constructor. */
        file_error(const file_error&) = default;

        /** Move constructor. */
        file_error(file_error&&) noexcept = default;

        /** Destructor. */
        ~file_error() noexcept override = default;

        /**
         * @brief Returns the error classification tag.
         */
        file_error_tag tag() const noexcept
        {
            return m_tag;
        }

        /**
         * @brief Returns the file path associated with the error.
         */
        core::string_view path() const noexcept
        {
            return m_path;
        }

    private:
        file_error_tag m_tag;
        core::string   m_path;
    };


    /**
     * @brief Describes the category of a data format error.
     *
     * Used by @ref format_error to classify errors related to
     * invalid or unsupported input data.
     */
    enum class format_error_tag
    {
        syntax,        /// Invalid syntax or malformed structure.
        missing_field, /// Required field is missing from the input data.
        invalid_type,  /// Data type does not match the expected type.
        invalid_value, /// Value is outside the allowed or expected range.
        invalid_data,  /// Data is corrupted or incorrect.
        unsupported,   /// Format or version is not supported.
    };

    /// Returns a string representation of the tag
    string_view to_string(format_error_tag tag) noexcept;

    /**
     * @brief Exception describing an input data format error.
     *
     * Represents errors caused by invalid, malformed, or unsupported
     * structured input data (e.g. JSON, XML, configuration files).
     */
    class format_error final : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a format error.
         *
         * @param tag Classification of the format error.
         * @param msg Error description.
         */
        explicit format_error(format_error_tag tag, const core::string& msg)
            : std::runtime_error(msg)
            , m_tag(tag)
        {
        }

        /** Copy constructor. */
        format_error(const format_error&) = default;

        /** Move constructor. */
        format_error(format_error&&) noexcept = default;

        /** Destructor. */
        ~format_error() noexcept override = default;

        /**
         * @brief Returns the error classification tag.
         */
        format_error_tag tag() const noexcept
        {
            return m_tag;
        }

    private:
        format_error_tag m_tag;
    };

} // namespace tavros::core
