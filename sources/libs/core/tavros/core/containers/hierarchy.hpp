#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/defines.hpp>

namespace tavros::core
{

    template<class D>
    class hierarchy_iterator;

    template<class D>
    class hierarchy_children_range;

    /**
     * @brief Represents a hierarchical tree node structure.
     *
     * Each node can contain arbitrary user data (`T*`) and maintain relationships
     * to its parent, siblings, and children. The class provides basic methods for
     * insertion, extraction, and traversal of nodes within a hierarchy.
     *
     * @tparam T Type of the data stored in each node.
     */
    template<class Derived>
    class hierarchy
    {
    public:
        using node_ptr = Derived*; /// Pointer to a hierarchy node.
        using iteration_range = hierarchy_children_range<Derived>;

    protected:
        /**
         * @brief Constructs an empty hierarchy node without attached data.
         */
        hierarchy() noexcept;

    public:
        /**
         * @brief Destroys the node.
         *
         * Does not delete child nodes or user data. Ownership and lifetime management
         * are the responsibility of the user.
         */
        ~hierarchy() noexcept;

        /**
         * @brief Returns a range object for iterating over child nodes.
         * @return Range with begin()/end() for use in range-based for loops.
         */
        [[nodiscard]] iteration_range children() noexcept;

        /**
         * @brief Returns a constant range object for iterating over child nodes.
         * @return Constant range with begin()/end().
         */
        [[nodiscard]] const iteration_range children() const noexcept;

        /**
         * @brief Returns the parent node.
         * @return Pointer to the parent node, or nullptr if this is a root node.
         */
        [[nodiscard]] node_ptr parent() const noexcept;

        /**
         * @brief Returns the next sibling node.
         * @return Pointer to the next sibling, or nullptr if this is the last sibling.
         */
        [[nodiscard]] node_ptr next() const noexcept;

        /**
         * @brief Returns the previous sibling node.
         * @return Pointer to the previous sibling, or nullptr if this is the first sibling.
         */
        [[nodiscard]] node_ptr prev() const noexcept;

        /**
         * @brief Returns the first child node.
         * @return Pointer to the first child, or nullptr if there are no children.
         */
        [[nodiscard]] node_ptr first_child() const noexcept;

        /**
         * @brief Returns the last child node.
         * @return Pointer to the last child, or nullptr if there are no children.
         */
        [[nodiscard]] node_ptr last_child() const noexcept;

        /**
         * @brief Inserts a node (or range of nodes) as the first child.
         * @param insert Node to insert as the first child.
         */
        void insert_first_child(node_ptr insert) noexcept;

        /**
         * @brief Inserts a node (or range of nodes) as the last child.
         * @param insert Node to insert as the last child.
         */
        void insert_last_child(node_ptr insert) noexcept;

        /**
         * @brief Inserts a node (or range of nodes) before the current node.
         * @param insert Node to insert before this node.
         */
        void insert_before(node_ptr insert) noexcept;

        /**
         * @brief Inserts a node (or range of nodes) after the current node.
         * @param insert Node to insert after this node.
         */
        void insert_after(node_ptr insert) noexcept;

        /**
         * @brief Detaches this node from its parent and sibling list.
         *
         * The node retains its children. It becomes a standalone hierarchy subtree.
         */
        void extract() noexcept;

        /**
         * @brief Detaches and returns the list of all child nodes.
         * @return Pointer to the first child in the extracted list, or nullptr if none.
         */
        [[nodiscard]] node_ptr extract_children() noexcept;

        /**
         * @brief Checks whether the node has any children.
         * @return True if the node has at least one child, false otherwise.
         */
        bool has_children() const noexcept;

    public:
        /**
         * @brief Returns the first node in a sibling list.
         * @param node Any node in the list.
         * @return Pointer to the first node in the chain.
         */
        /* static */ node_ptr first_of(node_ptr node) noexcept;

        /**
         * @brief Returns the last node in a sibling list.
         * @param node Any node in the list.
         * @return Pointer to the last node in the chain.
         */
        /* static */ node_ptr last_of(node_ptr node) noexcept;

        /**
         * @brief Returns the root node of the hierarchy.
         * @param node Any node in the hierarchy.
         * @return Pointer to the root node.
         */
        /* static */ node_ptr root_of(node_ptr node) noexcept;

    protected:
        /**
         * @brief Internal helper for inserting a sequence of nodes.
         *
         * Links the range [first_insert, last_insert] between @p left and @p right,
         * updating their parent pointers and sibling links.
         */
        /* static */ void insertion_helper(node_ptr left, node_ptr right, node_ptr parent, node_ptr first_insert, node_ptr last_insert) noexcept;

        /**
         * @brief Internal helper for extracting a sequence of nodes.
         *
         * Detaches the range [left, right] from the hierarchy while keeping internal
         * links between nodes intact. Returns pointer to the first node in the range.
         */
        /* static */ node_ptr extraction_helper(node_ptr left, node_ptr right) noexcept;

    private:
        node_ptr get_this() noexcept
        {
            return static_cast<node_ptr>(this);
        }

        const node_ptr get_this() const noexcept
        {
            return static_cast<const node_ptr>(this);
        }

    protected:
        template<class>
        friend class hierarchy;

        node_ptr m_parent;
        node_ptr m_next;
        node_ptr m_prev;
        node_ptr m_first_child;
        node_ptr m_last_child;
    };


    template<class D>
    inline hierarchy<D>::hierarchy() noexcept
        : m_parent(nullptr)
        , m_next(nullptr)
        , m_prev(nullptr)
        , m_first_child(nullptr)
        , m_last_child(nullptr)
    {
    }

    template<class D>
    inline hierarchy<D>::~hierarchy() noexcept
    {
    }

    template<class D>
    inline hierarchy<D>::iteration_range hierarchy<D>::children() noexcept
    {
        return iteration_range(m_first_child);
    }

    template<class D>
    inline const hierarchy<D>::iteration_range hierarchy<D>::children() const noexcept
    {
        return iteration_range(m_first_child);
    }

    template<class D>
    inline hierarchy<D>::node_ptr hierarchy<D>::parent() const noexcept
    {
        return m_parent;
    }

    template<class D>
    inline hierarchy<D>::node_ptr hierarchy<D>::next() const noexcept
    {
        return m_next;
    }

    template<class D>
    inline hierarchy<D>::node_ptr hierarchy<D>::prev() const noexcept
    {
        return m_prev;
    }

    template<class D>
    inline hierarchy<D>::node_ptr hierarchy<D>::first_child() const noexcept
    {
        return m_first_child;
    }

    template<class D>
    inline hierarchy<D>::node_ptr hierarchy<D>::last_child() const noexcept
    {
        return m_last_child;
    }

    template<class D>
    void hierarchy<D>::insert_first_child(node_ptr insert) noexcept
    {
        TAV_ASSERT(insert);
        insertion_helper(nullptr, m_first_child, get_this(), first_of(insert), last_of(insert));
    }

    template<class D>
    void hierarchy<D>::insert_last_child(node_ptr insert) noexcept
    {
        TAV_ASSERT(insert);
        insertion_helper(m_last_child, nullptr, get_this(), first_of(insert), last_of(insert));
    }

    template<class D>
    void hierarchy<D>::insert_before(node_ptr insert) noexcept
    {
        TAV_ASSERT(insert);
        insertion_helper(m_prev, get_this(), m_parent, first_of(insert), last_of(insert));
    }

    template<class D>
    void hierarchy<D>::insert_after(node_ptr insert) noexcept
    {
        TAV_ASSERT(insert);
        insertion_helper(get_this(), m_next, m_parent, first_of(insert), last_of(insert));
    }

    template<class D>
    void hierarchy<D>::extract() noexcept
    {
        extraction_helper(get_this(), get_this());
    }

    template<class D>
    hierarchy<D>::node_ptr hierarchy<D>::extract_children() noexcept
    {
        return extraction_helper(m_first_child, m_last_child);
    }

    template<class D>
    bool hierarchy<D>::has_children() const noexcept
    {
        TAV_ASSERT(!!m_first_child == !!m_last_child);
        return static_cast<bool>(m_first_child);
    }

    template<class D>
    hierarchy<D>::node_ptr hierarchy<D>::first_of(node_ptr node) noexcept
    {
        TAV_ASSERT(node);

        if (auto p = node->m_parent) {
            TAV_ASSERT(p->m_first_child);
            return p->m_first_child;
        }

        while (node->m_prev) {
            node = node->m_prev;
        }

        return node;
    }

    template<class D>
    hierarchy<D>::node_ptr hierarchy<D>::last_of(node_ptr node) noexcept
    {
        TAV_ASSERT(node);

        if (auto p = node->m_parent) {
            TAV_ASSERT(p->m_last_child);
            return p->m_last_child;
        }

        while (node->m_next) {
            node = node->m_next;
        }

        return node;
    }

    template<class D>
    hierarchy<D>::node_ptr hierarchy<D>::root_of(node_ptr node) noexcept
    {
        TAV_ASSERT(node);

        while (node->m_parent) {
            node = node->m_parent;
        }

        while (node->m_prev) {
            node = node->m_prev;
        }

        return node;
    }

    template<class D>
    void hierarchy<D>::insertion_helper(node_ptr left, node_ptr right, node_ptr parent, node_ptr first_insert, node_ptr last_insert) noexcept
    {
        TAV_ASSERT(left || right || parent);
        TAV_ASSERT(first_insert && last_insert);
        TAV_ASSERT(!first_insert->m_prev && !last_insert->m_next);
        TAV_ASSERT(first_insert != parent && first_insert != left && first_insert != right);
        TAV_ASSERT(last_insert != parent && last_insert != left && last_insert != right);

#if TAV_DEBUG
        for (auto it = first_insert; it; it = it->m_next) {
            TAV_ASSERT(nullptr == it->m_parent);
        }


        if (left && right) {
            TAV_ASSERT(left->m_parent == right->m_parent);
            TAV_ASSERT(left->m_next == right);
            TAV_ASSERT(right->m_prev == left);
            TAV_ASSERT(left != right);
        }

        if (parent) {
            TAV_ASSERT(parent != left && parent != right);

            if (left) {
                bool has_child = false;
                TAV_ASSERT(left->m_parent == parent);
                for (auto it = parent->m_first_child; it; it = it->m_next) {
                    if (it == left) {
                        has_child = true;
                        break;
                    }
                }
                TAV_ASSERT(has_child);
            }

            if (right) {
                bool has_child = false;
                TAV_ASSERT(right->m_parent == parent);
                for (auto it = parent->m_first_child; it; it = it->m_next) {
                    if (it == right) {
                        has_child = true;
                        break;
                    }
                }
                TAV_ASSERT(has_child);
            }
        }
#endif

        if (parent) {
            // set the whole sibling sequence of the parent and get a
            // pointer to the last node of the sibling sequence

            for (auto it = first_insert; it; it = it->m_next) {
                it->m_parent = parent;
                if (it == last_insert) {
                    break;
                }
            }

            if (left) {
                left->m_next = first_insert;
                first_insert->m_prev = left;
            } else {
                parent->m_first_child = first_insert;
            }

            if (right) {
                right->m_prev = last_insert;
                last_insert->m_next = right;
            } else {
                parent->m_last_child = last_insert;
            }
        } else {
            if (left) {
                left->m_next = first_insert;
                first_insert->m_prev = left;
            }
            if (right) {
                right->m_prev = last_insert;
                last_insert->m_next = right;
            }
        }
    }

    template<class D>
    hierarchy<D>::node_ptr hierarchy<D>::extraction_helper(node_ptr left, node_ptr right) noexcept
    {
        TAV_ASSERT(left || right);

        // Restore pointers to both left and right nodes
        if (!right) {
            right = last_of(left);
        } else if (!left) {
            left = first_of(right);
        }

        auto parent = left->m_parent;
        auto before = left->m_prev;
        auto after = right->m_next;

        // If the left node has a previous sibling and the right node has a next sibling
        if (before && after) {
            before->m_next = after;
            after->m_prev = before;
            left->m_prev = nullptr;
            right->m_next = nullptr;
        } else if (before || after) {
            if (before) {
                before->m_next = nullptr;
                left->m_prev = nullptr;
            } else {
                after->m_prev = nullptr;
                right->m_next = nullptr;
            }
        }

        // Separate from parent
        if (parent) {
            if (left == parent->m_first_child && right == parent->m_last_child) {
                parent->m_first_child = nullptr;
                parent->m_last_child = nullptr;
            } else if (left == parent->m_first_child) {
                parent->m_first_child = after;
            } else if (right == parent->m_last_child) {
                parent->m_last_child = before;
            }

            for (auto it = left; it; it = it->m_next) {
                it->m_parent = nullptr;
                if (it == right) {
                    break;
                }
            }
        }

        return left;
    }


    template<class T>
    class hierarchy_iterator
    {
    public:
        using node_ptr = T*;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

    public:
        explicit hierarchy_iterator(node_ptr node) noexcept
            : m_node(node)
        {
        }

        reference operator*() const noexcept
        {
            return *m_node;
        }
        pointer operator->() const noexcept
        {
            return m_node;
        }

        hierarchy_iterator& operator++() noexcept
        {
            if (m_node) {
                m_node = m_node->next();
            }
            return *this;
        }

        hierarchy_iterator operator++(int) noexcept
        {
            hierarchy_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        friend bool operator==(const hierarchy_iterator& a, const hierarchy_iterator& b) noexcept
        {
            return a.m_node == b.m_node;
        }

        friend bool operator!=(const hierarchy_iterator& a, const hierarchy_iterator& b) noexcept
        {
            return a.m_node != b.m_node;
        }

    private:
        node_ptr m_node;
    };

    template<class T>
    class hierarchy_children_range
    {
    public:
        using node_ptr = T*;
        using iterator = hierarchy_iterator<T>;

    public:
        explicit hierarchy_children_range(node_ptr first) noexcept
            : m_first(first)
        {
        }

        iterator begin() const noexcept
        {
            return iterator(m_first);
        }

        iterator end() const noexcept
        {
            return iterator(nullptr);
        }

    private:
        node_ptr m_first;
    };


} // namespace tavros::core