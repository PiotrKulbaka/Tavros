# Tavros - Naming & Style Conventions

Tavros follows an **STL-like** coding style. When in doubt, ask:
*"How would the standard library name this?"*

---

## 1. General Rules

| Rule | Example |
|------|---------|
| All names in `snake_case` | `buffer_view`, `index_allocator` |
| Template parameters in `PascalCase` | `template<class Container>` |
| Macro names in `UPPER_CASE` | `TAV_ASSERT(...)`, `TAV_UNREACHABLE()` |
| Concepts in `snake_case` | `some_concept<T>` |
| Private members prefixed `m_` | `m_storage`, `m_size` |
| No redundant `get_`/`set_` for simple accessors | `size()` not `get_size()` |

### Const-Correctness

If a function does not modify observable state, it must be marked `const`.

Provide `const` overloads for all element access functions:

```cpp
T&       operator[](size_type i) noexcept;
const T& operator[](size_type i) const noexcept;
```

---

## 2. Class & Type Naming

### `basic_` prefix

Used when a class is a **generic implementation parameterized by a policy**,
and the user interacts with it through a ready-made alias.

```cpp
template<class Policy, class... Ty>
class basic_table { ... };

template<class... Components>
using dense_table = basic_table<dense_policy, Components...>;

template<class... Components>
using sparse_table = basic_table<sparse_policy, Components...>;
```

> Pattern: `basic_` = "raw generic class, not for direct use". See `std::basic_string`, `std::basic_ostream`.

### `_policy` suffix
Storage or behaviour strategies passed as template parameters.

```cpp
struct dense_policy { ... };
struct sparse_policy { ... };
```

### `_view` suffix

Non-owning, lightweight reference into existing data.

Rules for view types:
- Trivially copyable
- Cheap to copy
- Non-owning
- Never allocate memory

```cpp
core::buffer_view
std::string_view
```

### `_traits` suffix

Compile-time property bundles about a type, following `std::iterator_traits`.

```cpp
template<class T>
struct component_traits
{
    static constexpr bool is_tag = (sizeof(T) == 0);
};
```

### `_t` suffix

```cpp
using size_type       = std::size_t;
using difference_type = std::ptrdiff_t;
using handle_t        = generation_handle;
```

### `_v` suffix

`constexpr` variable templates (mirrors STL convention).

```cpp
template<class T, class Tuple>
inline constexpr bool contains_type_v = contains_type<T, Tuple>::value;
```

---

## 3. Method Naming

### Size & Capacity

| Method | Description |
|--------|-------------|
| `size()` | Number of **live** elements currently stored |
| `capacity()` | Number of allocated slots |
| `empty()` | Returns `true` if `size() == 0` |
| `max_size()` | Theoretical maximum number of elements |

### Memory Management

| Method | Description |
|--------|-------------|
| `reserve(n)` | Pre-allocate memory for at least `n` elements, does not change `size()` |
| `resize(n)` | Change logical size to `n`; new elements are default-constructed |
| `shrink_to_fit()` | Reduce `capacity()` to `size()` |
| `clear()` | Remove all elements; memory is **not** released |

### Element Access

| Method | Description |
|--------|-------------|
| `operator[](i)` | Access by index; **no bounds checking** |
| `at(i)` | Access by index; **throws** `std::out_of_range` |
| `front()` | First element |
| `back()` | Last element |
| `data()` | Raw pointer to the underlying array |

### Search & Lookup

| Method | Description |
|--------|-------------|
| `find(key)` | Search by **key/value**; returns iterator or `end()` |
| `find_if(pred)` | Search by **predicate**; returns iterator or `end()` |
| `contains(key)` | Returns `bool`; does not return position |
| `count(key)` | Number of elements matching key |
| `lower_bound(key)` | First element **not less than** key (sorted containers) |
| `upper_bound(key)` | First element **greater than** key (sorted containers) |

Rules:
- `find` is always by key/value - never by index. Index access is `operator[]` or `at()`.
- Boolean-returning functions should read naturally as predicates:

```cpp
// Prefer
is_valid()
has_value()
contains()

// Avoid
check_valid()
validate()
```

### Insertion

| Method | Description |
|--------|-------------|
| `insert(value)` | Insert a copy; returns iterator or `pair<iterator, bool>` |
| `insert(pos, value)` | Insert before `pos` |
| `emplace(args...)` | Construct in-place; preferred over `insert` |
| `emplace_back(args...)` | Construct in-place at the end |
| `push_back(value)` | Append a copy or move |
| `push_front(value)` | Prepend (deque-like containers) |

### Removal

| Method | Description |
|--------|-------------|
| `erase(pos)` | Remove at iterator; **preserves order**; returns next iterator |
| `erase(first, last)` | Remove range `[first, last)`; preserves order |
| `erase(handle)` | Remove by stable handle (handle-based containers) |
| `pop_back()` | Remove last element; O(1) |
| `pop_front()` | Remove first element (deque-like containers) |
| `swap_erase(index)` | Remove by swapping with last; **O(1); breaks order** |
| `clear()` | Remove all elements |

Rules:
- `erase` returns an iterator to the element following the erased one.
- `swap_erase` is allowed only when order does not matter.

### Iteration & Functional

| Method | Description |
|--------|-------------|
| `begin()` / `end()` | Mutable iterators |
| `cbegin()` / `cend()` | Const iterators |

---

## 4. Iterator Naming

Follow STL iterator category names exactly:

```cpp
using iterator_category = std::random_access_iterator_tag;
using value_type        = ...;
using difference_type   = std::ptrdiff_t;
using pointer           = ...;
using reference         = ...;
```

Unify mutable and const iterators into a single template, expose as aliases:

```cpp
template<bool IsConst>
struct basic_iterator { ... };

using iterator       = basic_iterator<false>;
using const_iterator = basic_iterator<true>;
```

---

## 5. Template Parameter Naming

| Kind | Convention | Example |
|------|-----------|---------|
| Type | `PascalCase` | `T`, `Key`, `Value`, `Components` |
| Non-type (size) | `N`, `Capacity` | `size_t N` |
| Non-type (bool flag) | Descriptive | `IsConst`, `IsOwning` |
| Policy | Descriptive suffix | `AllocatorPolicy`, `StoragePolicy` |
| Concept-constrained | Same as type | `class T requires archetype<T>` |

---

## 6. Concept Naming

Concepts use `snake_case` and read like predicates:

```cpp
concept archetype
concept sparse_archetype
concept random_access_range
```

---

## 7. Free Functions

Prefer non-member functions for symmetric operations.

Factory functions use `make_`:

```cpp
auto v = make_table_view<position, velocity>(container);
```

---

## 8. Attributes

Apply `[[nodiscard]]` to any function whose return value should not be silently ignored:

```cpp
[[nodiscard]] size_t size() const noexcept;
[[nodiscard]] bool empty() const noexcept;
[[nodiscard]] T* data() noexcept;
```

Prefer `constexpr` for pure, trivial operations.

---

## 9. `noexcept` Policy

| Situation | Rule |
|-----------|------|
| Move constructor / move assignment | `noexcept` if subobjects are nothrow-movable |
| `swap` | Always `noexcept` |
| Size / capacity queries | `noexcept` |
| `clear` | `noexcept` |
| Allocating operations | Not `noexcept` |

```cpp
basic_table(basic_table&&)
    noexcept(std::is_nothrow_move_constructible_v<storage_type>);
```

---

## 10. Constructors

Single-argument constructors must be marked `explicit`
unless implicit conversion is intentional and clearly safe:

```cpp
explicit basic_table(size_type initial_capacity);
```

---

## 11. File & Directory Layout

```
tavros/
  core/
    containers/
      vector.hpp
      basic_table.hpp
      ...
    ids/
      index_allocator.hpp
      handle_allocator.hpp
      ...
    types.hpp
    ...
```

Rules:
- One primary class per file.
- File name matches the primary class name in `snake_case`.

---

## 12. Quick Reference Card

```
size()          - count of live elements
capacity()      - allocated slots
empty()         - size() == 0
clear()         - remove all, keep memory
reserve(n)      - pre-allocate, don't change size
resize(n)       - change size, default-construct new
shrink_to_fit() - release excess memory

operator[]      - index access, no bounds check
at(i)           - index access, throws on bad index
front/back()    - first/last element
data()          - raw pointer

find(key)       - search by key/value -> iterator
find_if(pred)   - search by predicate -> iterator
contains(key)   - existence check -> bool

insert(v)       - insert copy -> iterator
emplace(args)   - construct in-place -> iterator
push_back(v)    - append
pop_back()      - remove last
erase(pos)      - remove at iterator, preserve order
erase(handle)   - remove by stable handle
swap_erase(i)   - O(1) remove, breaks order

is_valid(h)     - handle liveness check
get(h)          - access by handle, UB if invalid
try_get(h)      - safe access, returns nullptr/optional

view<Cs...>()   - non-owning component view
each(f)         - iterate: f(components...)
each_indexed(f) - iterate: f(index, components...)
each_n(...)     - iterate sub-range
invoke_at(i, f) - invoke f at single index
```
