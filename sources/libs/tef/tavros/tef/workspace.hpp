#pragma once

#include <tavros/core/memory/fixed_pool_allocator.hpp>
#include <tavros/core/memory/memory.hpp>

#include <tavros/tef/node.hpp>

namespace tavros::tef
{

    /**
     * @brief TEF node workspace and memory owner.
     *
     * The workspace owns all nodes allocated for TEF documents and provides
     * facilities for creating, accessing, and destroying document trees.
     *
     * Memory for nodes is managed using an internal pool allocator, ensuring
     * stable pointers and fast allocations.
     *
     * The workspace stores multiple documents in a linked sequence.
     */
    class workspace final : core::noncopyable
    {
    private:
        friend class node;

    public:
        /// @brief Number of nodes the internal pool can hold.
        static constexpr size_t k_pool_capacity = 1024 * 32;

        /// @brief Maximum supported nesting depth.
        static constexpr size_t k_max_nesting_level = 64;

        /// @brief Maximum supported prototype inheritance depth.
        static constexpr size_t k_max_proto_depth = 32;

        /// @brief Internal node pool type.
        using pool_type = core::fixed_pool_allocator<node, k_pool_capacity>;

    public:
        /**
         * @brief Constructs an empty workspace.
         *
         * Initializes the internal memory pool and document list.
         */
        workspace();

        /**
         * @brief Destroys the workspace and all owned nodes.
         *
         * All allocated nodes are destroyed and their memory is released.
         * All previously obtained pointers become invalid.
         */
        ~workspace() noexcept;

        /**
         * @brief Creates and inserts a new document node into the workspace.
         *
         * @param path Path associated with the document (e.g. source file path).
         *
         * @param pos Optional insertion position.
         * - If @p pos is not null, the new document is inserted **before** the given node.
         * - If @p pos is null, the document is appended to the end of the document list.
         *
         * @return Pointer to the newly created document node.
         *
         * @note The returned pointer remains valid as long as the owning workspace exists.
         *
         * @warning The @p pos node must belong to the same workspace. Passing a node from
         * another workspace results in undefined behavior.
         */
        [[nodiscard]] node* new_document(core::string_view path, node* pos = nullptr);

        /**
         * @brief Finds a document by its path.
         *
         * @param path Path associated with the document.
         *
         * @return Pointer to the document node, or nullptr if not found.
         */
        [[nodiscard]] node* document(core::string_view path) noexcept;

        /// @copydoc document(core::string_view)
        [[nodiscard]] const node* document(core::string_view path) const noexcept;

        /**
         * @brief Returns the first document in the workspace.
         *
         * @return Pointer to the first document, or nullptr if empty.
         */
        [[nodiscard]] node* first_document() noexcept
        {
            return m_first;
        }

        /// @copydoc first_document()
        [[nodiscard]] const node* first_document() const noexcept
        {
            return m_first;
        }

        /**
         * @brief Returns the last document in the workspace.
         *
         * @return Pointer to the last document, or nullptr if empty.
         */
        [[nodiscard]] node* last_document() noexcept
        {
            return m_last;
        }

        /// @copydoc last_document()
        [[nodiscard]] const node* last_document() const noexcept
        {
            return m_last;
        }

        /**
         * @brief Returns an iterator over all documents.
         *
         * Iterates documents in insertion order (from first to last).
         */
        [[nodiscard]] node::iterator documents() noexcept
        {
            return node::iterator{m_first};
        }

        /// @copydoc documents()
        [[nodiscard]] node::const_iterator documents() const noexcept
        {
            return node::const_iterator{m_first};
        }

        /**
         * @brief Navigates a dot-separated path across documents.
         *
         * The lookup is performed relative to the contents of the documents and does
         * not include the document name in the path.
         *
         * Each segment of the path refers to a direct child of the node selected by
         * the preceding segments.
         *
         * The search is performed sequentially across all documents in their storage
         * order (from first to last). The first matching path is returned.
         *
         * @param path Dot-separated key path, e.g. @c "graphics.resolution.width".
         *             The path must not include any document identifier.
         *
         * @return Pointer to the first matching node, or nullptr if the path
         *         cannot be resolved in any document.
         */
        node* at_path(core::string_view path) noexcept;

        /// @copydoc at_path(core::string_view)
        const node* at_path(core::string_view path) const noexcept;

        /**
         * @brief Resolves a node by dot-separated path with prototype-aware lookup.
         *
         * Searches for a node using the provided hierarchical path. If a node is
         * not found in the local structure at a given level, the search continues
         * through the prototype chain of the current node.
         *
         * This method performs a semantic lookup rather than a strict structural
         * traversal.
         *
         * @param path  Hierarchical path to the target node.
         *
         * @return Pointer to the resolved node if found, otherwise @c nullptr.
         */
        node* resolve_path(core::string_view path) noexcept;

        /// @copydoc resolve_path(core::string_view)
        const node* resolve_path(core::string_view path) const noexcept;

        /**
         * @brief Destroys all nodes and resets the workspace.
         *
         * Clears all documents and releases all memory back to the pool.
         *
         * @warning All pointers, references, and iterators to nodes become invalid.
         */
        void clear() noexcept;

    private:
        /**
         * @brief Allocates memory for a single node from the pool.
         * @return Pointer to uninitialized node memory.
         */
        [[nodiscard]] node* alloc_node();

        /**
         * @brief Destroys a subtree of nodes and returns memory to the pool.
         *
         * Performs a depth-first traversal starting from @p n.
         * A node is destroyed only after all its descendants.
         *
         * @param n Any node within the subtree to destroy.
         *
         * @note The traversal depth must not exceed @ref k_max_nesting_level.
         */
        static void free_nodes(node* n) noexcept;

    private:
        core::unique_ptr<pool_type> m_pool;

        node* m_first = nullptr;
        node* m_last = nullptr;
    };

} // namespace tavros::tef
