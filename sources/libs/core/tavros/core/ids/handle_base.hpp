#pragma once

#include <tavros/core/ids/index_base.hpp>
#include <concepts>

namespace tavros::core
{

    using handle_type_t = uint16;
    using handle_gen_t = uint16;
    using handle_id_t = uint64;

    static_assert(sizeof(handle_type_t) + sizeof(handle_gen_t) + sizeof(index_t) == sizeof(handle_id_t));

    template<class T>
    struct type_id_of
    {
        static_assert(sizeof(T) == 0 && "Type is not registered as handle type");
    };

    template<handle_type_t Id>
    struct handle_type_registration
    {
        static_assert(Id != 0);
        using is_handle_type_registrator = void;
        static constexpr handle_type_t value = Id;
    };

    template<class T>
    concept handle_tagged =
        requires {
            typename type_id_of<T>::is_handle_type_registrator;
            requires std::same_as<decltype(type_id_of<T>::value), const handle_type_t>;
        };

    template<handle_tagged T>
    constexpr handle_type_t type_id_v = type_id_of<T>::value;


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
            : id(make_handle_id(tid, gen, index))
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

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (handle) { ... }
         */
        explicit operator bool() const noexcept
        {
            return id != 0;
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
