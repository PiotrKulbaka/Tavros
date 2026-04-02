#include <tavros/core/config.hpp>
#include <tavros/core/debug/assert.hpp>

namespace tavros::core
{

    // --- Constructors ---

    config_node::config_node() noexcept
        : m_type(value_type::null)
        , m_value(null_t{})
    {
    }

    config_node::config_node(config* owner, core::string_view key, value_variant val, value_type type) noexcept
		: m_owner(owner)
        , m_type(type)
        , m_value(std::move(val))
        , m_key(key)
    {
        TAV_ASSERT(m_owner);
    }

    config_node::config_node(config_node&& other) noexcept
        : m_type(other.m_type)
        , m_value(std::move(other.m_value))
        , m_key(std::move(other.m_key))
    {
        TAV_ASSERT(!other.parent() && !other.prev() && !other.next());
    }

    config_node& config_node::operator=(config_node&& other) noexcept
    {
        TAV_ASSERT(!other.parent() && !other.prev() && !other.next());
        if (this != &other) {
            m_type = other.m_type;
            m_value = std::move(other.m_value);
            m_key = std::move(other.m_key);
        }
        return *this;
    }

    // --- Key ---

    void config_node::set_key(core::string_view key) noexcept
    {
        m_key = core::string(key);
    }

    // --- Direct unchecked access ---

    core::string_view config_node::as_string() const noexcept
    {
        TAV_ASSERT(is_string());
        return std::get<core::string>(m_value);
    }

    int64 config_node::as_integer() const noexcept
    {
        TAV_ASSERT(is_integer());
        return std::get<int64>(m_value);
    }

    double config_node::as_float() const noexcept
    {
        TAV_ASSERT(is_floating_point());
        return std::get<double>(m_value);
    }

    bool config_node::as_bool() const noexcept
    {
        TAV_ASSERT(is_boolean());
        return std::get<bool>(m_value);
    }

    // --- Navigation ---

    config_node* config_node::find(core::string_view key) noexcept
    {
        for (auto& child : children()) {
            if (child.m_key == key) {
                return &child;
            }
        }
        return nullptr;
    }

    const config_node* config_node::find(core::string_view key) const noexcept
    {
        for (const auto& child : children()) {
            if (child.m_key == key) {
                return &child;
            }
        }
        return nullptr;
    }

    config_node* config_node::at_path(core::string_view path) noexcept
    {
        config_node* current = this;

        while (!path.empty() && current != nullptr) {
            // split off next segment
            auto              dot = path.find('.');
            core::string_view segment;

            if (dot == core::string_view::npos) {
                segment = path;
                path = {};
            } else {
                segment = path.substr(0, dot);
                path = path.substr(dot + 1);
            }

            if (segment.empty()) {
                return nullptr;
            }

            current = current->find(segment);
        }

        return current;
    }

    const config_node* config_node::at_path(core::string_view path) const noexcept
    {
        const config_node* current = this;

        while (!path.empty() && current != nullptr) {
            auto              dot = path.find('.');
            core::string_view segment;

            if (dot == core::string_view::npos) {
                segment = path;
                path = {};
            } else {
                segment = path.substr(0, dot);
                path = path.substr(dot + 1);
            }

            if (segment.empty()) {
                return nullptr;
            }

            current = current->find(segment);
        }

        return current;
    }





    config::config() noexcept = default;

    config::~config() noexcept
    {
        reset();
    }

    //config::config(config&& other) noexcept
    //    : m_pool(std::move(other.m_pool))
    //    , m_root(other.m_root)
    //{
    //    other.m_root = nullptr;
    //}

    //config& config::operator=(config&& other) noexcept
    //{
    //    if (this != &other) {
    //        reset();
    //        m_pool       = std::move(other.m_pool);
    //        m_root       = other.m_root;
    //        other.m_root = nullptr;
    //    }
    //    return *this;
    //}

    // --- Root ---

    config_node* config::root() noexcept
    {
        return m_root;
    }

    const config_node* config::root() const noexcept
    {
        return m_root;
    }

    void config::set_root(config_node* node) noexcept
    {
        TAV_ASSERT(!node || !node->parent());
        m_root = node;
    }

    bool config::empty() const noexcept
    {
        return m_root == nullptr;
    }

    // --- Lifetime ---

    void config::reset() noexcept
    {
        // Destroy all nodes — call destructors manually since pool owns raw memory
        if (m_root) {
            // Depth-first traversal, destroy all nodes
            config_node* node = m_root;
            while (node) {
                if (node->first_child()) {
                    node = node->first_child();
                } else {
                    config_node* to_destroy = node;
                    // Move to next before destroying
                    if (node->next()) {
                        node = node->next();
                    } else {
                        node = node->parent();
                    }
                    to_destroy->~config_node();
                }
            }
            m_root = nullptr;
        }
        m_pool.reset();
    }

    // --- Internal ---

    config_node* config::alloc_node() noexcept
    {
        auto* ptr = m_pool.allocate();
        TAV_ASSERT(ptr);
        return ptr;
    }

    // --- Node factories ---

    //config_node* config::make_null(core::string_view key) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_null(key));
    //    return node;
    //}

    //config_node* config::make_object(core::string_view key) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_object(key));
    //    return node;
    //}

    //config_node* config::make_string(core::string_view key, core::string_view val) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_string(key, val));
    //    return node;
    //}

    //config_node* config::make_integer(core::string_view key, int64 val) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_integer(key, val));
    //    return node;
    //}

    //config_node* config::make_float(core::string_view key, double val) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_float(key, val));
    //    return node;
    //}

    //config_node* config::make_bool(core::string_view key, bool val) noexcept
    //{
    //    auto* node = alloc_node();
    //    new (node) config_node(config_node::make_bool(key, val));
    //    return node;
    //}

} // namespace tavros::core
