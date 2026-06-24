#pragma once

#include <tavros/tef/node.hpp>
#include <tavros/core/logger/diagnostics.hpp>

#include <tavros/core/traits.hpp>

namespace tavros::tef
{

    /**
     * @brief Warns if the node contains more unnamed values than expected.
     *
     * This helper is intended for scalar-like fields where only a fixed number
     * of unnamed values is allowed after the current node.
     *
     * @param n Node to validate.
     * @param expected_count Expected number of values.
     * @param ds Diagnostics sink.
     */
    inline void warn_if_extra_values(const node* n, uint32 expected_count, core::diagnostics& ds) noexcept
    {
        if (n && n->next() && !n->next()->has_key()) {
            ds.warning(
                "Unexpected extra values at '{}'. Expected exactly {} value{}.",
                n->path(),
                expected_count,
                expected_count == 1 ? "" : "s"
            );
        }
    }

    /**
     * @brief Reads an optional string value from a configuration node.
     *
     * Resolves the specified key relative to the given parent node and attempts
     * to read it as a string. If the key does not exist, the provided default
     * string is assigned to the output value.
     *
     * @tparam StrT String type supporting clear() and append().
     *
     * @param parent Parent node used as the lookup root.
     * @param key Relative path of the value to read.
     * @param default_str Default value assigned when the key is not found.
     * @param out Destination string.
     * @param type_hint Human-readable type description used in diagnostics.
     * @param ds Diagnostics collector.
     *
     * @return true if the value was successfully read or the default value was
     *         applied; false if the value exists but is not a string.
     */
    template<class StrT>
    [[nodiscard]] bool read_string(
        const node*        parent,
        core::string_view  key,
        core::string_view  default_str,
        StrT&              out,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            out.clear();
            out.append(default_str);
            return true;
        }

        if (n->is_string()) {
            auto sv = n->value_or(core::string_view{});
            warn_if_extra_values(n, 1, ds);
            out.clear();
            out.append(sv);
            return true;
        }

        ds.error("Invalid value at '{}'. Expected a valid {}.", n->path(), type_hint);
        return false;
    }

    /**
     * @brief Reads a required string value from a configuration node.
     *
     * Resolves the specified key relative to the given parent node and attempts
     * to read it as a string. If the key does not exist, an error is reported.
     *
     * @tparam StrT String type supporting clear() and append().
     *
     * @param parent Parent node used as the lookup root.
     * @param key Relative path of the value to read.
     * @param out Destination string.
     * @param type_hint Human-readable type description used in diagnostics.
     * @param ds Diagnostics collector.
     *
     * @return true if the value was successfully read; false if the key is
     *         missing or the value is not a string.
     */
    template<class StrT>
    [[nodiscard]] bool read_required_string(
        const node*        parent,
        core::string_view  key,
        StrT&              out,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            out.clear();
            ds.error("Missing required field '{}' at '{}'.", key, parent->path());
            return false;
        }

        if (n->is_string()) {
            auto sv = n->value_or(core::string_view{});
            warn_if_extra_values(n, 1, ds);
            out.clear();
            out.append(sv);
            return true;
        }

        ds.error("Invalid value at '{}'. Expected a valid {}.", n->path(), type_hint);
        return false;
    }

    /**
     * @brief Reads an optional enum value from a child node.
     *
     * The node value is read as a string and converted using the supplied
     * mapper function.
     *
     * If the node does not exist, @p default_val is returned.
     * If the value cannot be converted, an error is reported and
     * `std::nullopt` is returned.
     *
     * @tparam T Enum type.
     * @tparam Mapper Callable with signature:
     *         `std::optional<T>(core::string_view)`.
     *
     * @param parent Parent node.
     * @param key Relative path to the target node.
     * @param default_val Value returned when the node is missing.
     * @param mapper String-to-enum conversion function.
     * @param type_hint Human-readable enum description used in diagnostics.
     * @param ds Diagnostics sink.
     *
     * @return Converted enum value, @p default_val if the node is missing,
     *         or `std::nullopt` if conversion fails.
     */
    template<class T, class Mapper>
    [[nodiscard]] bool read_enum(
        const node*        parent,
        core::string_view  key,
        T                  default_val,
        Mapper             mapper,
        T&                 out,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            out = default_val;
            return true;
        }

        auto sv = n->value_or(core::string_view{});
        if (auto result = mapper(sv)) {
            warn_if_extra_values(n, 1, ds);
            out = *result;
            return true;
        }

        ds.error("Invalid value '{}' at '{}'. Expected a valid {}.", sv, n->path(), type_hint);
        return false;
    }

    /**
     * @brief Reads a required enum value from a child node.
     *
     * The node value is read as a string and converted using the supplied
     * mapper function.
     *
     * If the node does not exist, an error is reported and
     * `std::nullopt` is returned.
     * If the value cannot be converted, an error is reported and
     * `std::nullopt` is returned.
     *
     * @tparam T Enum type.
     * @tparam Mapper Callable with signature:
     *         `std::optional<T>(core::string_view)`.
     *
     * @param parent Parent node.
     * @param key Relative path to the target node.
     * @param mapper String-to-enum conversion function.
     * @param type_hint Human-readable enum description used in diagnostics.
     * @param ds Diagnostics sink.
     *
     * @return Converted enum value or `std::nullopt` if the field is missing
     *         or conversion fails.
     */
    template<class T, class Mapper>
        requires std::invocable<Mapper, core::string_view>
    [[nodiscard]] bool read_required_enum(
        const node*        parent,
        core::string_view  key,
        Mapper             mapper,
        T&                 out,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            ds.error("Missing required field '{}' at '{}'.", key, parent->path());
            return false;
        }

        auto sv = n->value_or(core::string_view{});
        if (auto result = mapper(sv)) {
            warn_if_extra_values(n, 1, ds);
            out = *result;
            return true;
        }

        ds.error("Invalid value '{}' at '{}'. Expected a valid {}.", sv, n->path(), type_hint);
        return false;
    }

    /**
     * @brief Reads an optional scalar value from a child node.
     *
     * The node type is validated using the supplied predicate before
     * the value is read.
     *
     * If the node does not exist, @p default_val is returned.
     * If type validation fails, an error is reported and
     * `std::nullopt` is returned.
     *
     * @tparam T Value type.
     * @tparam TypeCheck Callable with signature:
     *         `bool(const node*)`.
     *
     * @param parent Parent node.
     * @param key Relative path to the target node.
     * @param default_val Value returned when the node is missing.
     * @param is_type Node type validation predicate.
     * @param type_hint Human-readable type description used in diagnostics.
     * @param ds Diagnostics sink.
     *
     * @return Parsed value, @p default_val if the node is missing,
     *         or `std::nullopt` if validation fails.
     */
    template<class T, class TypeCheck>
    [[nodiscard]] bool read_scalar(
        const node*        parent,
        core::string_view  key,
        T                  default_val,
        T&                 out,
        TypeCheck          is_type,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            out = default_val;
            return true;
        }

        if (!is_type(n)) {
            ds.error("Invalid value at '{}'. Expected {}.", n->path(), type_hint);
            return false;
        }

        warn_if_extra_values(n, 1, ds);
        out = n->value_or(default_val);
        return true;
    }

    /**
     * @brief Reads an required scalar value from a child node.
     *
     * The node type is validated using the supplied predicate before
     * the value is read.
     *
     * If the node does not exist, @p default_val is returned.
     * If type validation fails, an error is reported and
     * `std::nullopt` is returned.
     *
     * @tparam T Value type.
     * @tparam TypeCheck Callable with signature:
     *         `bool(const node*)`.
     *
     * @param parent Parent node.
     * @param key Relative path to the target node.
     * @param default_val Value returned when the node is missing.
     * @param is_type Node type validation predicate.
     * @param type_hint Human-readable type description used in diagnostics.
     * @param ds Diagnostics sink.
     *
     * @return Parsed value, @p default_val if the node is missing,
     *         or `std::nullopt` if validation fails.
     */
    template<class T, class TypeCheck>
    [[nodiscard]] bool read_required_scalar(
        const node*        parent,
        core::string_view  key,
        T&                 out,
        TypeCheck          is_type,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        const auto* n = parent->resolve_path(key);
        if (!n) {
            ds.error("Missing required field '{}' value at '{}'.", key, n->path());
            return false;
        }

        if (!is_type(n)) {
            ds.error("Invalid '{}' value at '{}'. Expected {}.", key, n->path(), type_hint);
            return false;
        }

        warn_if_extra_values(n, 1, ds);
        if (auto val = n->value<T>()) {
            out = *val;
            return true;
        }

        ds.error("Invalid '{}' value at '{}'. Expected {}.", key, n->path(), type_hint);

        return false;
    }

    /**
     * @brief Reads an optional scalar value and clamps it to a specified range.
     *
     * The value is read using read_scalar(). If the resulting value falls
     * outside the range [`lo`, `hi`], it is clamped and a warning is reported.
     *
     * If the node does not exist, @p default_val is returned.
     * If type validation fails, an error is reported and
     * `std::nullopt` is returned.
     *
     * @tparam T Value type.
     * @tparam TypeCheck Callable with signature:
     *         `bool(const node*)`.
     *
     * @param parent Parent node.
     * @param key Relative path to the target node.
     * @param default_val Value returned when the node is missing.
     * @param lo Lower bound of the valid range.
     * @param hi Upper bound of the valid range.
     * @param is_type Node type validation predicate.
     * @param type_hint Human-readable type description used in diagnostics.
     * @param ds Diagnostics sink.
     *
     * @return Parsed value clamped to [`lo`, `hi`], @p default_val if the
     *         node is missing, or `std::nullopt` if validation fails.
     */
    template<class T, class TypeCheck>
    [[nodiscard]] bool read_clamped(
        const node*        parent,
        core::string_view  key,
        T                  default_val,
        T                  lo,
        T                  hi,
        T&                 out,
        TypeCheck          is_type,
        core::string_view  type_hint,
        core::diagnostics& ds
    ) noexcept
    {
        T r;
        if (!read_scalar<T>(parent, key, default_val, r, is_type, type_hint, ds)) {
            return false;
        }

        if (r < lo || r > hi) {
            auto clamped = std::clamp(r, lo, hi);
            ds.warning("Value {} at '{}' is out of range [{}, {}]. Clamped to {}.", r, parent->path(), lo, hi, clamped);
            out = clamped;
            return true;
        }

        return true;
    }

} // namespace tavros::tef
