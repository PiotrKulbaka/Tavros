#pragma once

#include <concepts>
#include <fmt/fmt.hpp>
#include <tavros/core/ids/index_base.hpp>

namespace tavros::core
{

    using handle_type_t = uint16;
    using handle_gen_t = uint16;
    using handle_id_t = uint64;

    static_assert(sizeof(handle_type_t) + sizeof(handle_gen_t) + sizeof(index_t) == sizeof(handle_id_t));

    template<handle_type_t Id>
    struct handle_type_registration
    {
        static_assert(Id != 0);
        using is_handle_type_registrator = void;
        static constexpr handle_type_t type_id_value = Id;
    };

    template<class T>
    concept handle_tagged =
        requires {
            typename T::is_handle_type_registrator;
            requires std::same_as<decltype(T::type_id_value), const handle_type_t>;
        };

    template<handle_tagged T>
    constexpr handle_type_t type_id_v = T::type_id_value;


    constexpr handle_id_t make_handle_id(handle_type_t type_id, handle_gen_t generation, index_t index) noexcept
    {
        return (static_cast<handle_id_t>(type_id) << 48)
             | (static_cast<handle_id_t>(generation) << 32)
             | static_cast<handle_id_t>(index);
    }

    constexpr handle_type_t handle_type_of(handle_id_t id) noexcept
    {
        return static_cast<handle_type_t>((id >> 48) & 0xffff);
    }

    constexpr handle_gen_t handle_gen_of(handle_id_t id) noexcept
    {
        return static_cast<handle_gen_t>((id >> 32) & 0xffff);
    }

    constexpr index_t handle_index_of(handle_id_t id) noexcept
    {
        return static_cast<index_t>(id & 0xffffffff);
    }


    template<handle_tagged Tag>
    struct handle_base
    {
        static constexpr handle_type_t k_type_id = type_id_v<Tag>;

        handle_id_t id = 0;

        handle_base() noexcept = default;

        explicit handle_base(handle_id_t raw_id)
            : id(raw_id)
        {
        }

        explicit handle_base(handle_gen_t gen, index_t index)
            : id(make_handle_id(k_type_id, gen, index))
        {
        }

        constexpr handle_type_t type() const noexcept
        {
            return handle_type_of(id);
        }

        constexpr handle_gen_t generation() const noexcept
        {
            return handle_gen_of(id);
        }

        constexpr index_t index() const noexcept
        {
            return handle_index_of(id);
        }

        constexpr bool valid() const noexcept
        {
            return id != 0;
        }

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (handle) { ... }
         */
        explicit operator bool() const noexcept
        {
            return valid();
        }
    };


    template<handle_tagged Tag>
    constexpr bool operator==(handle_base<Tag> lhs, handle_base<Tag> rhs) noexcept
    {
        return lhs && rhs && lhs.id == rhs.id;
    }

    template<handle_tagged Tag>
    constexpr bool operator!=(handle_base<Tag> lhs, handle_base<Tag> rhs) noexcept
    {
        return !(lhs == rhs);
    }

} // namespace tavros::core

template<class Tag>
struct fmt::formatter<tavros::core::handle_base<Tag>>
{
    fmt::formatter<uint64_t> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    static void to_hex(uint64 num, size_t count, char* dst)
    {
        static const char alpha[] = "0123456789abcdef";
        for (size_t i = 0; i < count; ++i) {
            dst[count - i - 1] = alpha[num & 0xf];
            num >>= 4;
        }
        dst[count] = 0;
    }

    static string_view uint64_to_base64(uint64 u) noexcept
    {
        static const char        base64_alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        thread_local static char s[] = "000000000000";
        for (auto i = 0; i < 12; ++i) {
            s[12 - i - 1] = base64_alpha[u & 0x3f];
            u >>= 6;
        }
        return string_view(s, 12);
    }

    template<typename FormatContext>
    auto format(const tavros::core::handle_base<Tag>& h, FormatContext& ctx) const
    {
        char ty[5] = {0};
        char gen[5] = {0};
        char idx[9] = {0};
        to_hex(h.type(), 4, ty);
        to_hex(h.generation(), 4, gen);
        to_hex(h.index(), 8, idx);
        if (!h) {
            return fmt::format_to(ctx.out(), "(invalid) {}:{}:{}", fmt::styled_error(ty), fmt::styled_error(gen), fmt::styled_error(idx));
        }
        return fmt::format_to(ctx.out(), "{}:{}:{}", fmt::styled_important(ty), fmt::styled_name(gen), fmt::styled_important(idx));
    }
};
