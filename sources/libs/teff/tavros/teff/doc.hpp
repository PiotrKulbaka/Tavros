#pragma once

#include <tavros/core/memory/pool_allocator.hpp>
#include <tavros/core/memory/memory.hpp>

#include <tavros/teff/node.hpp>

namespace tavros::teff
{

    /**
     * @brief Owns and allocates all nodes of a single TEFF document.
     *
     * A doc represents the in-memory tree of a parsed TEFF file.
     * All nodes are allocated from an internal pool and are valid for
     * the lifetime of the doc. Pointers to nodes must not be used after
     * the doc is destroyed, moved, or clear() is called.
     *
     * The root of the document is an implicit flat sequence of top-level
     * nodes, accessible via first_child() / children() / at_path().
     */
    class doc final : core::noncopyable
    {
    private:
        friend class node;

    public:
        /// @brief Number of nodes the internal pool can hold.
        static constexpr size_t k_pool_capacity = 1024 * 32;

        /// @brief Maximum supported nesting depth for free_nodes traversal.
        static constexpr size_t k_max_nesting_level = 60;

        using pool_type = core::pool_allocator<node, k_pool_capacity>;

    public:
        /**
         * @brief Constructs an empty document.
         */
        doc();

        /**
         * @brief Destroys the document and releases all allocated nodes.
         */
        ~doc() noexcept;

        /**
         * @brief Move constructor.
         */
        doc(doc&& other) noexcept;

        /**
         * @brief Move assignment.
         */
        doc& operator=(doc&& other) noexcept;

    public:
        // ----------------------------------------------------------------
        // Tree navigation
        // ----------------------------------------------------------------

        /**
         * @brief Returns the first top-level node, or nullptr if the document is empty.
         */
        [[nodiscard]] node* first_child() noexcept
        {
            return m_first;
        }

        /// @copydoc first_child()
        [[nodiscard]] const node* first_child() const noexcept
        {
            return m_first;
        }

        /**
         * @brief Returns the last top-level node, or nullptr if the document is empty.
         */
        [[nodiscard]] node* last_child() noexcept
        {
            return m_last;
        }

        /// @copydoc last_child()
        [[nodiscard]] const node* last_child() const noexcept
        {
            return m_last;
        }

        /**
         * @brief Returns true if the document has at least one top-level node.
         */
        [[nodiscard]] bool has_children() const noexcept
        {
            return m_first != nullptr;
        }

        /**
         * @brief Returns a range for iterating top-level nodes.
         */
        [[nodiscard]] auto children() noexcept
        {
            return core::hierarchy_children_range<node>(m_first);
        }

        /// @copydoc children()
        [[nodiscard]] auto children() const noexcept
        {
            return core::hierarchy_children_range<node>(m_first);
        }

        /**
         * @brief Finds the first top-level node with the given key.
         *
         * Only direct top-level children are searched. For deep lookup
         * use at_path().
         *
         * @param key Key to search for.
         * @return Pointer to the matching node, or nullptr if not found.
         */
        node* child(core::string_view key) noexcept;

        /// @copydoc child(core::string_view)
        const node* child(core::string_view key) const noexcept;

        /**
         * @brief Navigates a dot-separated path from the document root.
         *
         * Each segment of the path names a direct child of the node
         * selected by the preceding segments. Returns nullptr if any
         * segment is not found.
         *
         * @param path Dot-separated key path, e.g. @c "graphics.resolution.width".
         * @return Pointer to the target node, or nullptr if the path is not found.
         */
        node* at_path(core::string_view path) noexcept;

        /// @copydoc at_path(core::string_view)
        const node* at_path(core::string_view path) const noexcept;

    public:
        // ----------------------------------------------------------------
        // Tree - mutation (nodes are allocated by owning doc)
        // ----------------------------------------------------------------

        /**
         * @brief Appends a new object node at the top level of the document.
         *
         * The returned pointer is valid for the lifetime of the document.
         *
         * @param key Key for the new node.
         * @return Pointer to the newly created object node.
         * @throws std::bad_alloc if the node pool is exhausted.
         */
        node* append_object(core::string_view key)
        {
            auto* n = node::make_obj_node(this, key);
            insert_last(n);
            return n;
        }

        /**
         * @brief Appends a new scalar node at the top level of the document.
         *
         * Supported value types: bool, integral types, floating-point types,
         * core::string, core::string_view, and other string-like types.
         * The returned pointer is valid for the lifetime of the document.
         *
         * To append a keyless continuation node (value sequence), pass an
         * empty string_view as @p key.
         *
         * @tparam T  Value type. Must be one of the supported scalar types.
         * @param key Key for the new node. Pass @c {} for a keyless node.
         * @param val Scalar value to store.
         * @return Pointer to the newly created node.
         * @throws std::bad_alloc if the node pool is exhausted.
         */
        template<class T>
        node* append(core::string_view key, T val)
        {
            using U = std::remove_cvref_t<T>;
            node* n = nullptr;
            if constexpr (core::is_string_like_v<U>) {
                core::string_view sv = core::string_view(val);
                n = node::make_str_node(this, key, sv);
            } else if constexpr (std::is_same_v<U, bool>) {
                n = node::make_bool_node(this, key, static_cast<bool>(val));
            } else if constexpr (std::is_integral_v<U>) {
                n = node::make_int_node(this, key, static_cast<int64>(val));
            } else if constexpr (std::is_floating_point_v<U>) {
                n = node::make_flt_node(this, key, static_cast<double>(val));
            } else {
                static_assert(!sizeof(T), "Unsupported type for doc::append");
            }

            insert_last(n);
            return n;
        }

        /**
         * @brief Destroys all nodes and resets the document to an empty state.
         *
         * All previously returned pointers and references become invalid
         * after this call.
         */
        void clear() noexcept;

    private:
        /**
         * @brief Allocates raw memory for a single node from the pool.
         */
        [[nodiscard]] node* alloc_node();

        /**
         * @brief Destroys a subtree of nodes and returns their memory to the pool.
         *
         * Performs a depth-first traversal. A node is destroyed only after
         * all of its descendants have been destroyed. The traversal starts
         * from the root of the subtree containing @p n.
         *
         * @param n Any node within the subtree to destroy.
         */
        static void free_nodes(node* n) noexcept;

        /**
         * @brief Appends a node (or chain of nodes) at the end of the
         *        top-level sequence, updating m_first and m_last.
         *
         * @param n First node in the chain to append.
         */
        void insert_last(node* n) noexcept;

    private:
        core::unique_ptr<pool_type> m_pool;

        node* m_first = nullptr;
        node* m_last = nullptr;
    };

} // namespace tavros::teff
