#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/containers/hierarchy.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/debug/verify.hpp>
#include <tavros/core/traits.hpp>

#include <tavros/tef/conv.hpp>

#include <variant>
#include <optional>

namespace tavros::tef
{

    class registry;

    /**
     * @brief A single node in a TEFF document tree.
     *
     * Nodes are owned exclusively by a @ref registry and must not outlive it.
     * Creation is only possible through @ref registry or another node's mutation
     * methods (append, append_object).
     *
     * A node is either:
     *   - an **object** — has children, no scalar value
     *   - a **scalar** — holds a typed value (string, integer, float, bool)
     *
     * Each node has an optional string key. Nodes without a key are
     * continuations of a value sequence initiated by the preceding keyed
     * sibling. [See TEFF spec 4.2, 5.2]
     *
     * @note Nodes are non-copyable and non-movable. Always access them
     *       through pointers or references returned by @ref registry and @ref node
     *       mutation methods.
     */
    class node : protected core::hierarchy<node>, core::noncopyable
    {
    private:
        friend class registry;
        friend class core::hierarchy<node>;

    public:
        using iterator = core::hierarchy<node>::iteration_range;
        using const_iterator = const core::hierarchy<node>::iteration_range;

        /**
         * @brief Discriminator for the type of data stored in a node.
         *
         * This enum defines both container and scalar node types used in TEFF.
         *
         * Notes:
         * - @ref document represents a top-level container and does not have a parent.
         * - @ref object is a nested container.
         * - Other types represent scalar values.
         */
        enum class node_type : uint8
        {
            /**
             * @brief Top-level document node.
             *
             * Acts as the root of a parsed file. It owns a hierarchy of child nodes.
             * A document node: has no parent; cannot be nested inside other nodes
             * may contain objects and scalar values.
             */
            document,

            /// Object node containing child nodes.
            object,

            /// UTF-8 string value.
            string,

            /// 64-bit floating-point value.
            integer,

            /// 64-bit floating-point value.
            floating_point,

            /// Boolean value.
            boolean,
        };

    public:
        // ----------------------------------------------------------------
        // Type
        // ----------------------------------------------------------------

        /**
         * @brief Returns the value type of this node.
         */
        [[nodiscard]] node_type type() const noexcept
        {
            return m_type;
        }

        /**
         * @brief Returns true if this node is an root (container for children).
         */
        [[nodiscard]] bool is_document() const noexcept
        {
            return m_type == node_type::document;
        }

        /**
         * @brief Returns true if this node is an object (container for children).
         */
        [[nodiscard]] bool is_object() const noexcept
        {
            return m_type == node_type::object;
        }

        /**
         * @brief Returns true if this node is a container.
         */
        [[nodiscard]] bool is_container() const noexcept
        {
            return m_type == node_type::object || m_type == node_type::document;
        }

        /**
         * @brief Returns true if this node holds a string value.
         */
        [[nodiscard]] bool is_string() const noexcept
        {
            return m_type == node_type::string;
        }

        /**
         * @brief Returns true if this node holds an integer value.
         */
        [[nodiscard]] bool is_integer() const noexcept
        {
            return m_type == node_type::integer;
        }

        /**
         * @brief Returns true if this node holds a floating - point value.
         */
        [[nodiscard]] bool is_floating_point() const noexcept
        {
            return m_type == node_type::floating_point;
        }

        /**
         * @brief Returns true if this node holds an integer or floating - point value.
         */
        [[nodiscard]] bool is_number() const noexcept
        {
            return m_type == node_type::integer || m_type == node_type::floating_point;
        }

        /**
         * @brief Returns true if this node holds a boolean value.
         */
        [[nodiscard]] bool is_boolean() const noexcept
        {
            return m_type == node_type::boolean;
        }

        /**
         * @brief Returns true if this node holds any scalar value(i.e.is not an object).
         */
        [[nodiscard]] bool is_scalar() const noexcept
        {
            return !is_container();
        }

    public:
        // ----------------------------------------------------------------
        // Key
        // ----------------------------------------------------------------

        /**
         * @brief Returns true if this node has a non - empty key.
         */
        [[nodiscard]] bool has_key() const noexcept
        {
            return !m_key.empty();
        }

        /**
         * @brief Returns the key of this node as a string view.
         *
         * Returns an empty string_view if the node has no key.
         * The returned view is valid for the lifetime of the owning @ref registry.
         */
        [[nodiscard]] core::string_view key() const noexcept
        {
            return m_key;
        }

    public:
        // ----------------------------------------------------------------
        // Value
        // ----------------------------------------------------------------

        /**
         * @brief Returns the node value as type @p T, with numeric cross-conversion.
         *
         * Conversion rules:
         *   - @c bool     : only from boolean nodes. Never converted from numeric types.
         *   - integral    : from integer nodes (exact); from floating-point nodes (truncating cast).
         *   - float/double: from floating-point nodes (exact); from integer nodes (widening cast).
         *   - string_view / string : from string nodes only.
         *
         * Returns @c std::nullopt if the node type is incompatible with @p T,
         * or if the node is an object.
         *
         * @tparam T Target type. Supported: bool, integral types, floating-point
         *           types, core::string, core::string_view.
         */
        template<typename T>
        [[nodiscard]] std::optional<T> value() const noexcept
        {
            if constexpr (std::is_same_v<T, bool>) {
                if (is_boolean()) {
                    return static_cast<bool>(std::get<int64>(m_value));
                }
            } else if constexpr (std::is_same_v<T, core::string_view> || std::is_same_v<T, core::string>) {
                if (is_string()) {
                    return std::get<core::string>(m_value);
                }
            } else if constexpr (std::is_integral_v<T>) {
                if (is_integer()) {
                    return static_cast<T>(std::get<int64>(m_value));
                } else if (is_floating_point()) {
                    return static_cast<T>(std::get<double>(m_value));
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                if (is_floating_point()) {
                    return static_cast<T>(std::get<double>(m_value));
                } else if (is_integer()) {
                    return static_cast<T>(std::get<int64>(m_value));
                }
            } else {
                static_assert(false, "Unsupported type");
            }
            return std::nullopt;
        }

        /**
         * @brief Returns the node value as type @p T, or @p fallback if unavailable.
         *
         * @tparam T      Target type. See value() for supported types and conversion rules.
         * @param fallback Value returned when the node type is incompatible.
         */
        template<typename T>
        [[nodiscard]] T value_or(T fallback) const noexcept
        {
            if (auto val = value<T>()) {
                return *val;
            }
            return fallback;
        }

        /**
         * @brief Converts this node (and its keyless siblings) to type @p T
         *        using a registered @ref conv specialization.
         *
         * The conversion starts at this node and may advance through subsequent
         * keyless siblings to read multi-value types.
         *
         * Returns @c std::nullopt if the conversion fails.
         *
         * @tparam T Target type. Must have a corresponding @ref conv specialization.
         */
        template<typename T>
        [[nodiscard]] std::optional<T> as() const noexcept
        {
            return conv<T>{}(this);
        }

        /**
         * @brief Converts this node to type @p T, or returns @p fallback on failure.
         *
         * @tparam T      Target type. Must have a corresponding @ref conv specialization.
         * @param fallback Value returned when conversion fails.
         */
        template<typename T>
        [[nodiscard]] T as_or(T fallback) const noexcept
        {
            auto result = as<T>();
            return result.has_value() ? *result : fallback;
        }

    public:
        // ----------------------------------------------------------------
        // Tree navigation
        // ----------------------------------------------------------------

        /**
         * @brief Returns the parent node, or nullptr if this is a root node.
         */
        [[nodiscard]] node* parent() noexcept
        {
            return hierarchy::parent();
        }

        /// @copydoc parent()
        [[nodiscard]] const node* parent() const noexcept
        {
            return hierarchy::parent();
        }

        /**
         * @brief Returns the next sibling node, or nullptr if this is the last sibling.
         */
        [[nodiscard]] node* next() noexcept
        {
            return hierarchy::next();
        }

        /// @copydoc next()
        [[nodiscard]] const node* next() const noexcept
        {
            return hierarchy::next();
        }

        /**
         * @brief Returns the previous sibling node, or nullptr if this is the first sibling.
         */
        [[nodiscard]] node* prev() noexcept
        {
            return hierarchy::prev();
        }

        /// @copydoc prev()
        [[nodiscard]] const node* prev() const noexcept
        {
            return hierarchy::prev();
        }

        /**
         * @brief Returns the first child node, or nullptr if this node has no children.
         */
        [[nodiscard]] node* first_child() noexcept
        {
            return hierarchy::first_child();
        }

        /// @copydoc first_child()
        [[nodiscard]] const node* first_child() const noexcept
        {
            return hierarchy::first_child();
        }

        /**
         * @brief Returns the last child node, or nullptr if this node has no children.
         */
        [[nodiscard]] node* last_child() noexcept
        {
            return hierarchy::last_child();
        }

        /// @copydoc last_child()
        [[nodiscard]] const node* last_child() const noexcept
        {
            return hierarchy::last_child();
        }

        /**
         * @brief Returns true if this node has at least one child.
         */
        [[nodiscard]] bool has_children() const noexcept
        {
            return hierarchy::has_children();
        }

        /**
         * @brief Returns a range for iterating direct children.
         */
        [[nodiscard]] auto children() noexcept
        {
            return hierarchy::children();
        }

        /**
         * @brief Returns a range for iterating direct children.
         */
        [[nodiscard]] auto children() const noexcept
        {
            return hierarchy::children();
        }

        /**
         * @brief Finds the first direct child with the given key.
         *
         * Only immediate children are searched. For deep lookup use at_path().
         *
         * @param key Key to search for.
         * @return Pointer to the matching child, or nullptr if not found.
         */
        node* child(core::string_view key) noexcept;

        /// @copydoc child(core::string_view)
        const node* child(core::string_view key) const noexcept;

        /**
         * @brief Navigates a dot-separated path starting from this node.
         *
         * Each segment names a direct child of the node selected by the
         * preceding segments. Returns nullptr if any segment is not found
         * or if an intermediate node is not an object.
         *
         * @param path Dot-separated key path, e.g. @c "resolution.width".
         * @return Pointer to the target node, or nullptr if not found.
         */
        node* at_path(core::string_view path) noexcept;

        /// @copydoc at_path(core::string_view)
        const node* at_path(core::string_view path) const noexcept;

    public:
        // ----------------------------------------------------------------
        // Tree - mutation (nodes are allocated by owning registry)
        // ----------------------------------------------------------------

        /**
         * @brief Appends a new object child node.
         *
         * This node must be an object; a failed assertion is triggered otherwise.
         * The returned pointer is valid for the lifetime of the owning @ref registry.
         *
         * @param key Key for the new child node.
         * @return Pointer to the newly created object node.
         * @throws std::bad_alloc if the node pool is exhausted.
         */
        node* append_object(core::string_view key)
        {
            TAV_VERIFY(is_container());
            auto* n = make_obj_node(m_owner, key);
            insert_last_child(n);
            return n;
        }

        /**
         * @brief Appends a new scalar child node.
         *
         * This node must be an object; a failed assertion is triggered otherwise.
         * The returned pointer is valid for the lifetime of the owning @ref registry.
         *
         * Supported value types: bool, integral types, floating-point types,
         * core::string, core::string_view, and other string-like types.
         *
         * To append a keyless continuation node (value sequence), pass an
         * empty string_view as @p key.
         *
         * @tparam T  Value type. Must be one of the supported scalar types.
         * @param key Key for the new child node. Pass @c {} for a keyless node.
         * @param val Scalar value to store.
         * @return Pointer to the newly created node.
         * @throws std::bad_alloc if the node pool is exhausted.
         */
        template<class T>
        node* append(core::string_view key, T val)
        {
            TAV_VERIFY(is_container());
            using U = std::remove_cvref_t<T>;

            node* n = nullptr;
            if constexpr (core::is_string_like_v<U>) {
                core::string_view sv = core::string_view(val);
                n = node::make_str_node(m_owner, key, sv);
            } else if constexpr (std::is_same_v<U, bool>) {
                n = node::make_bool_node(m_owner, key, static_cast<bool>(val));
            } else if constexpr (std::is_integral_v<U>) {
                n = node::make_int_node(m_owner, key, static_cast<int64>(val));
            } else if constexpr (std::is_floating_point_v<U>) {
                n = node::make_flt_node(m_owner, key, static_cast<double>(val));
            } else {
                static_assert(!sizeof(T), "Unsupported type for node::append");
            }

            insert_last_child(n);
            return n;
        }

        /**
         * @brief Removes and destroys all direct children of this node.
         *
         * All pointers and references to the removed nodes become invalid.
         * This node must be an object.
         */
        void clear_children() noexcept;

    private:
        using value_variant = std::variant<std::nullptr_t /* for object */, int64, double, core::string>;

        static [[nodiscard]] node* make_obj_node(registry* owner, core::string_view key);
        static [[nodiscard]] node* make_str_node(registry* owner, core::string_view key, core::string_view val);
        static [[nodiscard]] node* make_int_node(registry* owner, core::string_view key, int64 val);
        static [[nodiscard]] node* make_flt_node(registry* owner, core::string_view key, double val);
        static [[nodiscard]] node* make_bool_node(registry* owner, core::string_view key, bool val);

        node(registry* owner, core::string_view key, node_type type, value_variant value);
        ~node() noexcept = default;

    private:
        registry*     m_owner;
        node_type     m_type;
        core::string  m_key;
        value_variant m_value;
    };

} // namespace tavros::tef
