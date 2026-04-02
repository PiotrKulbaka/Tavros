#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/containers/hierarchy.hpp>
#include <tavros/core/memory/pool_allocator.hpp>
#include <optional>
#include <variant>

namespace tavros::core
{
    class config;
    class config_node;





    class config_node : protected hierarchy<config_node>
    {
    public:
        enum class value_type
        {
            null,
            object,
            string,
            integer,
            floating_point,
            boolean,
        };

        // null sentinel
        struct null_t
        {
        };

        using value_variant = std::variant<
            null_t,
            bool, // boolean перед integer Ч иначе bool конвертируетс€ в int
            int64,
            double,
            core::string>;

    public:
        config_node() noexcept;
        ~config_node() noexcept = default;

        config_node(const config_node&) = delete;
        config_node& operator=(const config_node&) = delete;

        config_node(config_node&&) noexcept;
        config_node& operator=(config_node&&) noexcept;

    public:
        // --- Type checks ---

        [[nodiscard]] value_type type() const noexcept
        {
            return m_type;
        }

        [[nodiscard]] bool is_null() const noexcept
        {
            return m_type == value_type::null;
        }
        [[nodiscard]] bool is_object() const noexcept
        {
            return m_type == value_type::object;
        }
        [[nodiscard]] bool is_string() const noexcept
        {
            return m_type == value_type::string;
        }
        [[nodiscard]] bool is_integer() const noexcept
        {
            return m_type == value_type::integer;
        }
        [[nodiscard]] bool is_floating_point() const noexcept
        {
            return m_type == value_type::floating_point;
        }
        [[nodiscard]] bool is_boolean() const noexcept
        {
            return m_type == value_type::boolean;
        }

        [[nodiscard]] bool is_scalar() const noexcept
        {
            return !is_null() && !is_object();
        }

        template <class T>
		config_node* add_first_child(string_view key, T value) noexcept;
        
        template <class T>
		config_node* add_last_child(string_view key, T value) noexcept;

        template <class T>
		config_node* add_next(string_view key, T value) noexcept;

        template <class T>
		config_node* add_prev(string_view key, T value) noexcept;

        config_node* extract();

        config_node* extract_children();

        void remove_children();




        [[nodiscard]] bool has_key() const noexcept
        {
            return !m_key.empty();
        }
        [[nodiscard]] core::string_view key() const noexcept
        {
            return m_key;
        }

        void set_key(core::string_view key) noexcept;

    public:
        // --- Value access ---

        /// @brief Returns value as T with numeric conversions.
        ///        bool -> integer/float conversions are NOT allowed.
        ///        Returns nullopt if type is incompatible or node is object/null.
        template<typename T>
        [[nodiscard]] std::optional<T> get() const noexcept
        {
            if constexpr (std::is_same_v<T, bool>) {
                if (is_boolean()) {
                    return std::get<bool>(m_value);
                }
                return std::nullopt;

            } else if constexpr (std::is_integral_v<T>) {
                if (is_integer()) {
                    return static_cast<T>(std::get<int64>(m_value));
                } else if (is_floating_point()) {
                    return static_cast<T>(std::get<double>(m_value));
                }
                return std::nullopt;

            } else if constexpr (std::is_floating_point_v<T>) {
                if (is_floating_point()) {
                    return static_cast<T>(std::get<double>(m_value));
                } else if (is_integer()) {
                    return static_cast<T>(std::get<int64>(m_value));
                }
                return std::nullopt;

            } else if constexpr (std::is_same_v<T, core::string_view> || std::is_same_v<T, core::string>) {
                if (is_string()) {
                    return core::string_view(std::get<core::string>(m_value));
                }
                return std::nullopt;

            } else {
                static_assert(sizeof(T) == 0, "Unsupported config value type");
            }
        }

        /// @brief Returns value as T or default_val if unavailable.
        template<typename T>
        [[nodiscard]] T get_or(T default_val) const noexcept
        {
            auto result = get<T>();
            return result.has_value() ? *result : default_val;
        }

        /// @brief Direct unchecked access. UB if wrong type.
        [[nodiscard]] core::string_view as_string() const noexcept;
        [[nodiscard]] int64             as_integer() const noexcept;
        [[nodiscard]] double            as_float() const noexcept;
        [[nodiscard]] bool              as_bool() const noexcept;

    public:
        // --- Navigation helpers ---

        /// @brief Finds first direct child with given key. Returns nullptr if not found.
        [[nodiscard]] config_node*       find(core::string_view key) noexcept;
        [[nodiscard]] const config_node* find(core::string_view key) const noexcept;

        /// @brief Navigates dot-separated path. Returns nullptr if any segment missing.
        /// @example node.at_path("window.size.width")
        [[nodiscard]] config_node*       at_path(core::string_view path) noexcept;
        [[nodiscard]] const config_node* at_path(core::string_view path) const noexcept;

    private:
        explicit config_node(config* owner, core::string_view key, value_variant val, value_type type) noexcept;

		config*       m_owner = nullptr; // back-pointer to owning config for node factories
        value_type    m_type = value_type::null;
        value_variant m_value;
        core::string  m_key;
    };





    class config : noncopyable
    {
    public:
		friend class config_node;

        config() noexcept;
        ~config() noexcept;

    public:
        [[nodiscard]] config_node*       root() noexcept;
        [[nodiscard]] const config_node* root() const noexcept;


        [[nodiscard]] bool empty() const noexcept;

    public:
        void reset() noexcept;

    private:
        [[nodiscard]] config_node* alloc_node() noexcept;

    private:
        pool_allocator<config_node, 1024 * 32> m_pool;
        config_node* m_root = nullptr;
    };

} // namespace tavros::core
