#pragma once

#include <tavros/core/memory/pool_allocator.hpp>
#include <tavros/core/memory/memory.hpp>

#include <tavros/tef/node.hpp>

namespace tavros::tef
{

    class registry final : core::noncopyable
    {
    private:
        friend class node;

    public:
        /// @brief Number of nodes the internal pool can hold.
        static constexpr size_t k_pool_capacity = 1024 * 32;

        /// @brief Maximum supported nesting depth for free_nodes traversal.
        static constexpr size_t k_max_nesting_level = 64;

        using pool_type = core::pool_allocator<node, k_pool_capacity>;

    public:
        /** @brief Constructs an empty registry. */
        registry();

        /** @brief Destroys the document and releases all allocated nodes. */
        ~registry() noexcept;

        /** @brief Move constructor. */
        registry(registry&& other) noexcept;

        /** @brief Move assignment. */
        registry& operator=(registry&& other) noexcept;

        /**
         * @brief Creates and inserts a new document node into the registry.
         *
         * @param path Path associated with the document (e.g. source file path).
         *
         * @param pos Optional insertion position.
         * - If @p pos is not null, the new document is inserted **before** the given node.
         * - If @p pos is null, the document is appended to the end of the document list.
         *
         * @return Pointer to the newly created document node.
         *
         * @note The returned pointer remains valid as long as the owning registry exists.
         *
         * @warning The @p pos node must belong to the same registry. Passing a node from
         * another registry results in undefined behavior.
         */
        [[nodiscard]] node* new_document(core::string_view path, node* pos = nullptr);

        [[nodiscard]] node* document(core::string_view path) noexcept;

        [[nodiscard]] const node* document(core::string_view path) const noexcept;

        [[nodiscard]] node* first_document() noexcept
        {
            return m_first;
        }

        [[nodiscard]] const node* first_document() const noexcept
        {
            return m_first;
        }

        [[nodiscard]] node* last_document() noexcept
        {
            return m_last;
        }

        [[nodiscard]] const node* last_document() const noexcept
        {
            return m_last;
        }

        [[nodiscard]] node::iterator documents() noexcept
        {
            return node::iterator{m_first};
        }

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

    private:
        core::unique_ptr<pool_type> m_pool;

        node* m_first = nullptr;
        node* m_last = nullptr;
    };

} // namespace tavros::tef
