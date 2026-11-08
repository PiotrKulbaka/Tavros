#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/traits.hpp>

#include <new>
#include <utility>

namespace tavros::core
{

    /**
     * @brief Compile-time registered heterogeneous container.
     *
     * Stores at most one object of every registered type.
     *
     * @tparam List       List of types supported by the container.
     * @tparam MaxCellSize  Maximum allowed size of a registered object.
     *
     * All objects are stored inline in a contiguous array. No allocation is
     * performed for individual objects.
     */
    template<class List, size_t MaxCellSize>
    class type_storage;


    template<class... Ts, size_t MaxCellSize>
    class type_storage<type_list<Ts...>, MaxCellSize>
    {
    public:
        using types = type_list<Ts...>;

        static_assert(sizeof...(Ts) > 0, "type_storage requires at least one registered type");
        static_assert(are_unique_unqualified_v<types>, "type_storage contains duplicate types");
        static_assert(max_size_types_v<types> <= MaxCellSize, "type_storage contains a type larger than MaxCellSize");

        using size_type = size_t;

        static constexpr size_type k_type_count = list_size_v<types>;
        static constexpr size_type k_max_object_size = MaxCellSize;
        static constexpr size_type k_max_size = max_size_types_v<types>;
        static constexpr size_type k_cell_alignment = max_align_types_v<types>;
        static constexpr size_type k_cell_size = ((k_max_size + k_cell_alignment - 1) / k_cell_alignment) * k_cell_alignment;

    private:
        /**
         * @brief Storage cell.
         *
         * Its size is the largest registered object rounded up to the
         * largest required alignment.
         */
        struct alignas(k_cell_alignment) cell_t
        {
            uint8 data[k_cell_size];
            bool  has_data = false;
        };

        static_assert(alignof(cell_t) == k_cell_alignment);
        static_assert(sizeof(cell_t) == k_cell_size);


    public:
        type_storage() = default;

        ~type_storage() noexcept
        {
            clear();
        }

        type_storage(const type_storage&) = delete;
        type_storage& operator=(const type_storage&) = delete;

        type_storage(type_storage&& other) noexcept
        {
            move_from(std::move(other));
        }

        type_storage& operator=(type_storage&& other) noexcept
        {
            if (this != &other) {
                clear();
                move_from(std::move(other));
            }

            return *this;
        }

    public:
        /**
         * @brief Adds an object of type T.
         *
         * Only one object of every registered type can exist.
         *
         * @return true if the object was added, false if an object of the
         *         same type already exists.
         */
        template<class T>
        bool add_data(T&& value)
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            if (m_cells[index].has_data) {
                return false;
            }

            new (m_cells[index].data) U(std::forward<T>(value));

            m_cells[index].has_data = true;

            return true;
        }

        /**
         * @brief Default-constructs an object of type T.
         *
         * @return true if the object was added, false if an object of the same
         *         type already exists.
         */
        template<class T>
            requires(!std::is_const_v<T>)
        bool add_data()
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            if (m_cells[index].has_data) {
                return false;
            }

            static_assert(std::is_default_constructible_v<U>, "T must be default constructible");

            new (m_cells[index].data) U();

            m_cells[index].has_data = true;

            return true;
        }

        /**
         * @brief Returns a reference to an existing object.
         *
         * Asserts if no object of type T exists.
         */
        template<class T>
        [[nodiscard]] T& get_data() noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            TAV_ASSERT(m_cells[index].has_data);

            return *std::launder(reinterpret_cast<T*>(m_cells[index].data));
        }

        /**
         * @brief Returns a const reference to an existing object.
         *
         * Asserts if no object of type T exists.
         */
        template<class T>
        [[nodiscard]] const T& get_data() const noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            TAV_ASSERT(m_cells[index].has_data);

            return *std::launder(reinterpret_cast<const T*>(m_cells[index].data));
        }

        /**
         * @brief Returns a pointer to an existing object or nullptr.
         */
        template<class T>
        [[nodiscard]] T* try_get_data() noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            if (!m_cells[index].has_data) {
                return nullptr;
            }

            return std::launder(reinterpret_cast<T*>(m_cells[index].data));
        }

        /**
         * @brief Returns a const pointer to an existing object or nullptr.
         */
        template<class T>
        [[nodiscard]] const T* try_get_data() const noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            if (!m_cells[index].has_data) {
                return nullptr;
            }

            return std::launder(reinterpret_cast<const T*>(m_cells[index].data));
        }

        /**
         * @brief Removes an object of type T.
         *
         * @return true if an object was removed.
         */
        template<class T>
        bool remove_data() noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            if (!m_cells[index].has_data) {
                return false;
            }

            std::destroy_at(std::launder(reinterpret_cast<U*>(m_cells[index].data)));

            m_cells[index].has_data = false;

            return true;
        }


        /**
         * @brief Returns true if an object of type T exists.
         */
        template<class T>
        [[nodiscard]] bool has_data() const noexcept
        {
            using U = std::remove_cv_t<T>;

            static_assert(contains_type_v<U, types>, "T is not registered in type_storage");

            constexpr size_type index = type_index_v<U, types>;

            return m_cells[index].has_data;
        }

        /**
         * @brief Removes all stored objects.
         */
        void clear() noexcept
        {
            for (size_type i = 0; i < k_type_count; ++i) {
                if (m_cells[index].has_data) {
                    k_destroy_functions[i](m_cells[i].data);
                    m_cells[index].has_data = false;
                }
            }
        }

    private:
        void move_from(type_storage&& other) noexcept
        {
            for (size_type i = 0; i < k_type_count; ++i) {
                if (!other.m_cells[index].has_data) {
                    continue;
                }

                k_move_functions[i](m_cells[i].data, other.m_cells[i].data);
                m_cells[index].has_data = true;
                k_destroy_functions[i](other.m_cells[i].data);
                other.m_cells[index].has_data = false;
            }
        }

    private:
        using move_function_t = void    (*)(void*, void*) noexcept;
        using destroy_function_t = void (*)(void*) noexcept;

        template<class T>
        static void move(void* dst, void* src) noexcept
        {
            static_assert((std::is_nothrow_move_constructible_v<Ts> && ...), "all registered types must be nothrow move constructible");
            new (dst) T(std::move(*static_cast<T*>(src)));
        }

        template<class T>
        static void destroy(void* ptr) noexcept
        {
            std::destroy_at(static_cast<T*>(ptr));
        }

    private:
        vector<cell_t> m_cells = vector<cell_t>(k_type_count);

        inline static constexpr destroy_function_t k_destroy_functions[] = {&destroy<Ts>...};
        inline static constexpr move_function_t    k_move_functions[] = {&move<Ts>...};
    };

} // namespace tavros::core
