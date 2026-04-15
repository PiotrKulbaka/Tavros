#include <tavros/tef/registry.hpp>

#include <tavros/core/containers/fixed_vector.hpp>

namespace tavros::tef
{

    registry::registry()
        : m_pool(core::make_unique<pool_type>())
        , m_first(nullptr)
        , m_last(nullptr)
    {
    }

    registry::~registry() noexcept
    {
        clear();
    }

    node* registry::new_document(core::string_view path, node* pos)
    {
        auto* new_n = new (alloc_node()) node(this, {}, node::node_type::document, core::string(path), nullptr);
        if (pos) {
            TAV_ASSERT(pos->m_owner == this);
            pos->insert_before(new_n);
            if (pos == m_first) {
                m_first = new_n;
            }
        } else {
            // Insert to the end
            if (m_last) {
                m_last->insert_after(new_n);
                m_last = new_n;
            } else {
                m_first = m_last = new_n;
            }
        }

        return new_n;
    }

    node* registry::document(core::string_view path) noexcept
    {
        node* n = m_first;
        while (n) {
            TAV_ASSERT(n->is_document());
            auto sv = n->value<core::string_view>();
            if (sv == path) {
                return n;
            }
            n = n->next();
        }

        return nullptr;
    }

    const node* registry::document(core::string_view path) const noexcept
    {
        const node* n = m_first;
        while (n) {
            TAV_ASSERT(n->is_document());
            auto sv = n->value<core::string_view>();
            if (sv == path) {
                return n;
            }
            n = n->next();
        }

        return nullptr;
    }

    node* registry::at_path(core::string_view path) noexcept
    {
        node* n = m_first;
        while (n) {
            TAV_ASSERT(n->is_document());
            auto* res = n->at_path(path);
            if (res) {
                return res;
            }
            n = n->next();
        }

        return nullptr;
    }

    const node* registry::at_path(core::string_view path) const noexcept
    {
        const node* n = m_first;
        while (n) {
            TAV_ASSERT(n->is_document());
            const auto* res = n->at_path(path);
            if (res) {
                return res;
            }
            n = n->next();
        }

        return nullptr;
    }

    void registry::clear() noexcept
    {
        if (m_first) {
            free_nodes(m_first);
            m_first = m_last = nullptr;
        }
    }

    node* registry::alloc_node()
    {
        auto* ptr = m_pool->allocate();
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    void registry::free_nodes(node* n) noexcept
    {
        TAV_ASSERT(n);

        core::fixed_vector<node*, k_max_nesting_level> stack;

        node* current = core::hierarchy<node>::root_of(n);
        bool  from_stack = false;

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

} // namespace tavros::tef
