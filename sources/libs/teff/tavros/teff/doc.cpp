#include <tavros/teff/doc.hpp>

#include <tavros/core/containers/fixed_vector.hpp>

namespace
{
    template<class Node>
    Node* find_child_by_key(Node* first, tavros::core::string_view key) noexcept
    {
        for (auto* child = first; child != nullptr; child = child->next()) {
            if (child->key() == key) {
                return child;
            }
        }
        return nullptr;
    }
} // namespace

namespace tavros::teff
{

    doc::doc()
        : m_pool(core::make_unique<pool_type>())
    {
    }

    doc::~doc() noexcept
    {
        clear();
    }

    doc::doc(doc&& other) noexcept
        : m_pool(std::move(other.m_pool))
        , m_first(other.m_first)
        , m_last(other.m_last)
    {
        other.m_first = nullptr;
        other.m_last = nullptr;
    }

    doc& doc::operator=(doc&& other) noexcept
    {
        if (this != &other) {
            clear();
            m_pool = std::move(other.m_pool);
            m_first = other.m_first;
            m_last = other.m_last;
            other.m_first = nullptr;
            other.m_last = nullptr;
        }
        return *this;
    }

    node* doc::child(core::string_view key) noexcept
    {
        return find_child_by_key(first_child(), key);
    }

    const node* doc::child(core::string_view key) const noexcept
    {
        return find_child_by_key(first_child(), key);
    }

    node* doc::at_path(core::string_view path) noexcept
    {
        if (path.empty()) {
            return nullptr;
        }

        auto dot = path.find('.');
        auto is_npos = dot == core::string_view::npos;
        auto segment = is_npos ? path : path.substr(0, dot);
        path = is_npos ? core::string_view() : path.substr(dot + 1);

        if (segment.empty()) {
            return nullptr;
        }

        if (auto child = find_child_by_key(first_child(), segment)) {
            return path.empty() ? child : child->at_path(path);
        }

        return nullptr;
    }

    const node* doc::at_path(core::string_view path) const noexcept
    {
        if (path.empty()) {
            return nullptr;
        }

        auto dot = path.find('.');
        auto is_npos = dot == core::string_view::npos;
        auto segment = is_npos ? path : path.substr(0, dot);
        path = is_npos ? core::string_view() : path.substr(dot + 1);

        if (segment.empty()) {
            return nullptr;
        }

        if (auto child = find_child_by_key(first_child(), segment)) {
            return path.empty() ? child : child->at_path(path);
        }

        return nullptr;
    }

    void doc::clear() noexcept
    {
        if (m_first) {
            free_nodes(m_first);
            m_first = nullptr;
            m_last = nullptr;
        }
    }

    node* doc::alloc_node()
    {
        auto* ptr = m_pool->allocate();
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    void doc::free_nodes(node* n) noexcept
    {
        TAV_ASSERT(n);

        core::fixed_vector<node*, k_max_nesting_level> stack;
        node*                                          current = core::hierarchy<node>::root_of(n);
        bool                                           from_stack = false;

        while (current) {
            if (!from_stack && current->first_child()) {
                // Go deeper
                stack.push_back(current);
                current = current->first_child();
                from_stack = false;
            } else {
                node* to_free = current;

                if (current->next()) {
                    current = current->next();
                    from_stack = false;
                } else {
                    if (stack.empty()) {
                        current = nullptr;
                    } else {
                        current = stack.back();
                        stack.pop_back();
                    }
                    from_stack = true;
                }

                auto* owner = to_free->m_owner;
                to_free->~node();
                owner->m_pool->deallocate(to_free);
            }
        }
    }

    void doc::insert_last(node* n) noexcept
    {
        TAV_ASSERT(n);

        node* last_in_chain = n;
        while (last_in_chain->next()) {
            last_in_chain = last_in_chain->next();
        }

        if (m_first == nullptr) {
            m_first = n;
        } else {
            m_last->insert_after(n);
        }
        m_last = last_in_chain;
    }

} // namespace tavros::teff