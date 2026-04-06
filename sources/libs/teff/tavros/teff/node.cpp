#include <tavros/teff/node.hpp>

#include <tavros/teff/doc.hpp>

namespace
{
    template<class Node>
    Node* find_child_by_key(Node* parent, tavros::core::string_view key) noexcept
    {
        for (auto* child = parent->first_child(); child != nullptr; child = child->next()) {
            if (child->key() == key) {
                return child;
            }
        }
        return nullptr;
    }
} // namespace

namespace tavros::teff
{

    node* node::child(core::string_view key) noexcept
    {
        return find_child_by_key(this, key);
    }

    const node* node::child(core::string_view key) const noexcept
    {
        return find_child_by_key(this, key);
    }

    node* node::at_path(core::string_view path) noexcept
    {
        if (path.empty()) {
            return nullptr;
        }

        auto dot = path.find('.');
        auto is_npos = dot == core::string_view::npos;
        auto segment = is_npos ? path : path.substr(0, dot);
        path = is_npos ? core::string_view() : path.substr(dot + 1);

        if (segment.empty() || !is_object()) {
            return nullptr;
        }

        if (auto child = find_child_by_key(this, segment)) {
            return path.empty() ? child : child->at_path(path);
        }

        return nullptr;
    }

    const node* node::at_path(core::string_view path) const noexcept
    {
        if (path.empty()) {
            return nullptr;
        }

        auto dot = path.find('.');
        auto is_npos = dot == core::string_view::npos;
        auto segment = is_npos ? path : path.substr(0, dot);
        path = is_npos ? core::string_view() : path.substr(dot + 1);

        if (segment.empty() || !is_object()) {
            return nullptr;
        }

        if (auto child = find_child_by_key(this, segment)) {
            return path.empty() ? child : child->at_path(path);
        }

        return nullptr;
    }

    void node::clear_children() noexcept
    {
        auto* ch = extract_children();
        if (ch) {
            doc::free_nodes(ch);
        }
    }

    node* node::make_obj_node(doc* owner, core::string_view key)
    {
        return new (owner->alloc_node()) node(owner, key, value_type::object, nullptr);
    }

    node* node::make_str_node(doc* owner, core::string_view key, core::string_view val)
    {
        return new (owner->alloc_node()) node(owner, key, value_type::string, core::string(val));
    }

    node* node::make_int_node(doc* owner, core::string_view key, int64 val)
    {
        return new (owner->alloc_node()) node(owner, key, value_type::integer, val);
    }

    node* node::make_flt_node(doc* owner, core::string_view key, double val)
    {
        return new (owner->alloc_node()) node(owner, key, value_type::floating_point, val);
    }

    node* node::make_bool_node(doc* owner, core::string_view key, bool val)
    {
        return new (owner->alloc_node()) node(owner, key, value_type::boolean, static_cast<int64>(val));
    }

    node::node(doc* owner, core::string_view key, value_type type, value_variant value)
        : m_owner(owner)
        , m_key(key)
        , m_type(type)
        , m_value(std::move(value))
    {
    }

} // namespace tavros::teff