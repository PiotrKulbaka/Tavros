#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/traits.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace tavros::core
{

    namespace detail
    {
        template<class...>
        inline constexpr bool type_storage_always_false_v = false;

        /**
         * @brief Compile-time memory layout of a type_storage buffer.
         *
         * [ obj0 | pad | obj1 | pad | ... | objN-1 | flag0 flag1 ... flagN-1 ]
         *
         * Objects are placed in declaration order, each at the next offset
         * satisfying its own alignment. One occupancy byte per type follows
         * the last object.
         */
        template<std::size_t N>
        struct type_storage_layout
        {
            std::size_t offsets[N];   ///< Byte offset of every object.
            std::size_t flags_offset; ///< Offset of the first occupancy flag.
            std::size_t total_size;   ///< Objects + padding + flags, in bytes.
            std::size_t alignment;    ///< Largest alignment among all types.
        };

        template<class... Ts>
        constexpr type_storage_layout<sizeof...(Ts)> make_type_storage_layout() noexcept
        {
            constexpr std::size_t count = sizeof...(Ts);
            constexpr std::size_t sizes[count] = {sizeof(Ts)...};
            constexpr std::size_t aligns[count] = {alignof(Ts)...};

            type_storage_layout<count> layout{};

            std::size_t cursor = 0;
            std::size_t max_align = 1;

            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t a = aligns[i];

                // Alignments are always powers of two.
                cursor = (cursor + a - 1) / a * a;

                layout.offsets[i] = cursor;
                cursor += sizes[i];

                if (a > max_align) {
                    max_align = a;
                }
            }

            layout.flags_offset = cursor;
            layout.total_size = cursor + count;
            layout.alignment = max_align;

            return layout;
        }
    } // namespace detail

    /**
     * @brief Compile-time registered heterogeneous container.
     *
     * Stores at most one object of every type listed in `type_list<Ts...>`.
     *
     * Memory layout is computed automatically from the sizes and alignments
     * of all registered types (see detail::type_storage_layout). Every type
     * gets its own offset, so a small type never pays for a large one.
     * To reduce padding, list types with larger alignment first.
     *
     * All state (objects + occupancy flags) lives in ONE heap buffer owned by
     * a vector, which is allocated in the constructor. Access by type is O(1):
     * the offset is a compile-time constant.
     *
     * Guarantees and rules:
     *  - Objects never move after construction. Moving the storage steals the
     *    buffer, so addresses of the objects stay valid (but any pointer the
     *    objects keep to the storage itself does not).
     *  - A moved-from storage is empty (has no buffer). It may only be
     *    destroyed, cleared or assigned to; any other call asserts.
     *  - Objects are destroyed in REVERSE order of the type list
     *    (in clear() and in the destructor).
     *  - Registered types must be unqualified, non-array object types with
     *    a non-throwing destructor.
     *  - Not thread-safe.
     *  - Constructors and destructors of stored objects must not add or remove
     *    objects of their own type through the same storage.
     *
     * Requirements on tavros::core::vector: data(), size(), constructor from a
     * count, noexcept move that steals the buffer and leaves the source empty,
     * and an allocator that honours over-aligned types.
     */
    template<class List>
    class type_storage
    {
        static_assert(detail::type_storage_always_false_v<List>, "type_storage<List>: List must be type_list<Ts...>");
    };

    template<class... Ts>
    class type_storage<type_list<Ts...>>
    {
    public:
        using types = type_list<Ts...>;
        using size_type = std::size_t;

        static_assert(sizeof...(Ts) > 0, "type_storage requires at least one registered type");
        static_assert((std::is_object_v<Ts> && ...), "type_storage: registered types must be object types");
        static_assert((!std::is_array_v<Ts> && ...), "type_storage: array types are not supported");
        static_assert((std::is_same_v<Ts, std::remove_cv_t<Ts>> && ...), "type_storage: registered types must not be cv-qualified");
        static_assert((std::is_nothrow_destructible_v<Ts> && ...), "type_storage: registered types must be nothrow destructible");
        static_assert(are_unique_unqualified_v<types>, "type_storage contains duplicate types");

    private:
        using byte_t = unsigned char;

        static constexpr auto k_layout = detail::make_type_storage_layout<Ts...>();

    public:
        static constexpr size_type k_type_count = sizeof...(Ts);

        /// Alignment of the buffer (largest alignment among the types).
        static constexpr size_type k_alignment = k_layout.alignment;

        /// Number of bytes actually used inside the buffer.
        static constexpr size_type k_buffer_size = k_layout.total_size;

    private:
        /**
         * @brief Buffer element. A vector of these gives a buffer that is
         *        aligned to k_alignment and sized in whole units.
         */
        struct alignas(k_alignment) unit_t
        {
            byte_t bytes[k_alignment];
        };

        static_assert(sizeof(unit_t) == k_alignment);
        static_assert(alignof(unit_t) == k_alignment);

        static constexpr size_type k_unit_count = (k_buffer_size + k_alignment - 1) / k_alignment;

        using buffer_t = vector<unit_t>;

        static_assert(std::is_nothrow_move_constructible_v<buffer_t>, "type_storage requires a vector with noexcept move construction");
        static_assert(std::is_nothrow_move_assignable_v<buffer_t>, "type_storage requires a vector with noexcept move assignment");

    public:
        /**
         * @brief Allocates the buffer. All slots are empty.
         *
         * May throw if the allocation fails.
         */
        type_storage()
            : m_buffer(k_unit_count)
        {
            TAV_ASSERT(reinterpret_cast<std::uintptr_t>(m_buffer.data()) % k_alignment == 0);

            // Do not rely on the vector value-initializing its elements.
            byte_t* flags = bytes() + k_layout.flags_offset;
            for (size_type i = 0; i < k_type_count; ++i) {
                flags[i] = 0;
            }
        }

        ~type_storage() noexcept
        {
            clear();
        }

        type_storage(const type_storage&) = delete;
        type_storage& operator=(const type_storage&) = delete;

        /**
         * @brief Steals the buffer. Stored objects are not moved.
         *
         * The moved-from storage is empty: it may only be destroyed,
         * cleared or assigned to.
         */
        type_storage(type_storage&& other) noexcept
            : m_buffer(std::move(other.m_buffer))
        {
            // The vector must steal the buffer. Otherwise the objects would be
            // copied bytewise, which is not allowed.
            TAV_ASSERT(other.m_buffer.size() == 0);
        }

        type_storage& operator=(type_storage&& other) noexcept
        {
            if (this != &other) {
                clear();
                m_buffer = std::move(other.m_buffer);

                TAV_ASSERT(other.m_buffer.size() == 0);
            }

            return *this;
        }

    public:
        /**
         * @brief Constructs an object of type T in place from @p args.
         *
         * T must always be given explicitly: add_data<Foo>(a, b). With no
         * arguments the object is value-initialized.
         *
         * If an object of type T already exists nothing happens and the
         * arguments are left untouched.
         *
         * @return true if the object was created, false if one already exists.
         *         If the constructor throws, the exception propagates and the
         *         slot stays empty.
         */
        template<class T, class... Args>
        bool add_data(Args&&... args)
        {
            using U = std::remove_cv_t<T>;

            constexpr size_type index = checked_index<U>();

            static_assert(std::is_constructible_v<U, Args...>, "T is not constructible from the given arguments");

            byte_t& flag = flag_at(index);

            if (flag != 0) {
                return false;
            }

            ::new (static_cast<void*>(slot(index))) U(std::forward<Args>(args)...);

            flag = 1;

            return true;
        }

        /**
         * @brief Returns a reference to an existing object.
         *
         * Asserts (debug) if no object of type T exists. T may be const.
         */
        template<class T>
        [[nodiscard]] T& get_data() noexcept
        {
            constexpr size_type index = checked_index<std::remove_cv_t<T>>();

            TAV_ASSERT(flag_at(index) != 0);

            return *std::launder(reinterpret_cast<T*>(slot(index)));
        }

        template<class T>
        [[nodiscard]] const T& get_data() const noexcept
        {
            constexpr size_type index = checked_index<std::remove_cv_t<T>>();

            TAV_ASSERT(flag_at(index) != 0);

            return *std::launder(reinterpret_cast<const T*>(slot(index)));
        }

        /**
         * @brief Returns a pointer to an existing object or nullptr.
         */
        template<class T>
        [[nodiscard]] T* try_get_data() noexcept
        {
            constexpr size_type index = checked_index<std::remove_cv_t<T>>();

            if (flag_at(index) == 0) {
                return nullptr;
            }

            return std::launder(reinterpret_cast<T*>(slot(index)));
        }

        template<class T>
        [[nodiscard]] const T* try_get_data() const noexcept
        {
            constexpr size_type index = checked_index<std::remove_cv_t<T>>();

            if (flag_at(index) == 0) {
                return nullptr;
            }

            return std::launder(reinterpret_cast<const T*>(slot(index)));
        }

        /**
         * @brief Returns true if an object of type T exists.
         */
        template<class T>
        [[nodiscard]] bool has_data() const noexcept
        {
            constexpr size_type index = checked_index<std::remove_cv_t<T>>();

            return flag_at(index) != 0;
        }

        /**
         * @brief Destroys the object of type T.
         *
         * @return true if an object was removed.
         */
        template<class T>
        bool remove_data() noexcept
        {
            using U = std::remove_cv_t<T>;

            constexpr size_type index = checked_index<U>();

            byte_t& flag = flag_at(index);

            if (flag == 0) {
                return false;
            }

            // Mark as empty first: a destructor that looks at the storage
            // sees a consistent state and cannot destroy itself twice.
            flag = 0;

            destroy_object<U>(slot(index));

            return true;
        }

        /**
         * @brief Destroys all stored objects (in reverse order of the type
         *        list). Safe to call on a moved-from storage.
         */
        void clear() noexcept
        {
            if (m_buffer.size() == 0) {
                return; // moved-from
            }

            for (size_type i = k_type_count; i > 0; --i) {
                const size_type index = i - 1;
                byte_t&         flag = flag_at(index);

                if (flag != 0) {
                    flag = 0;
                    k_destroy_functions[index](slot(index));
                }
            }
        }

    private:
        /// Index of T in the list, or k_type_count if T is not registered.
        template<class T>
        static constexpr size_type index_of() noexcept
        {
            constexpr bool matches[] = {std::is_same_v<T, Ts>...};

            for (size_type i = 0; i < k_type_count; ++i) {
                if (matches[i]) {
                    return i;
                }
            }

            return k_type_count;
        }

        template<class T>
        static constexpr size_type checked_index() noexcept
        {
            constexpr size_type index = index_of<T>();

            static_assert(index < k_type_count, "T is not registered in type_storage");

            return index;
        }

        byte_t* bytes() noexcept
        {
            TAV_ASSERT(m_buffer.size() != 0 && "type_storage used after being moved from");

            return reinterpret_cast<byte_t*>(m_buffer.data());
        }

        const byte_t* bytes() const noexcept
        {
            TAV_ASSERT(m_buffer.size() != 0 && "type_storage used after being moved from");

            return reinterpret_cast<const byte_t*>(m_buffer.data());
        }

        byte_t* slot(size_type index) noexcept
        {
            return bytes() + k_layout.offsets[index];
        }

        const byte_t* slot(size_type index) const noexcept
        {
            return bytes() + k_layout.offsets[index];
        }

        byte_t& flag_at(size_type index) noexcept
        {
            return bytes()[k_layout.flags_offset + index];
        }

        const byte_t& flag_at(size_type index) const noexcept
        {
            return bytes()[k_layout.flags_offset + index];
        }

        using destroy_function_t = void (*)(void*) noexcept;

        template<class T>
        static void destroy_object(void* ptr) noexcept
        {
            std::destroy_at(std::launder(static_cast<T*>(ptr)));
        }

        static constexpr destroy_function_t k_destroy_functions[] = {&destroy_object<Ts>...};

    private:
        buffer_t m_buffer;
    };

} // namespace tavros::core