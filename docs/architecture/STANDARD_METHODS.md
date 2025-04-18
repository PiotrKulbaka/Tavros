
# 🧭 Naming Guidelines for Consistent APIs in Tavros Engine

This document lists common and recommended method names based on C++ Standard Library and common game engine conventions.
Use this list to maintain consistent naming for actions with the same semantics across the entire codebase.

---

## 📦 Standard STL-style Methods

| Name                  | Description                                                       |
|-----------------------|-------------------------------------------------------------------|
| `clear()`             | Clears all elements. Frees memory if applicable.                  |
| `push_back()`         | Adds an element to the end.                                       |
| `pop_back()`          | Removes the last element.                                         |
| `insert()`            | Inserts element(s).                                               |
| `erase()`             | Removes element(s).                                               |
| `find()`              | Searches for an element.                                          |
| `contains()`          | Checks if an element exists.                                      |
| `reserve()`           | Reserves memory to avoid reallocations.                           |
| `resize()`            | Changes container size.                                           |
| `shrink_to_fit()`     | Shrinks memory to fit current size.                               |
| `swap()`              | Swaps contents with another object.                               |
| `assign()`            | Replaces the contents.                                            |
| `append()`            | Add to end.                                                       |
| `count()`             | Counting the quantity.                                            |

| Name                  | Description (Capacity info)                                       |
|-----------------------|-------------------------------------------------------------------|
| `empty()`             | Checks if the container has no elements.                          |
| `size()`              | Returns the number of elements.                                   |
| `length()`            | Returns the number of elements in contiguous memory.              |
| `capacity()`          | Amount of allocated memory in elements.                           |

| Name                  | Description (Element access)                                      |
|-----------------------|-------------------------------------------------------------------|
| `at()`                | Bounds-checked access to elements.                                |
| `operator[]`          | No-checked access to elements.                                    |
| `front()`             | Returns reference to the first element.                           |
| `back()`              | Returns reference to the last element.                            |
| `data()`              | Returns raw pointer to underlying data.                           |

| Name                  | Description (Iterators)                                           |
|-----------------------|-------------------------------------------------------------------|
| `begin()`             | Iterator to the first element.                                    |
| `end()`               | Iterator past the last element.                                   |
| `rbegin()`            | Reverse iterator to the first element.                            |
| `rend()`              | Reverse iterator to the last element.                             |
| `cbegin()`            | Const iterator to the first element.                              |
| `cend()`              | Const iterator past the last element.                             |
| `crbegin()`           | Const reverse iterator to the first element.                      |
| `crend()`             | Const reverse iterator past the last element.                     |

---

## ⚙️ Engine-Specific or Low-Level API Methods

| Name                  | Description                                                               |
|-----------------------|---------------------------------------------------------------------------|
| `init()`              | Initializes the object or subsystem.                                      |
| `shutdown()`          | Cleans up resources and finalizes the system.                             |
| `reset()`             | Resets the state without reallocating or freeing memory.                  |
| `destroy()`           | Frees memory and resources explicitly.                                    |
| `release()`           | Releases ownership of a resource or handle.                               |
| `reload()`            | Reloads data or resources (useful for hot-reload systems).                |
| `bind()`              | Binds the resource (e.g., texture, buffer, shader).                       |
| `unbind()`            | Unbinds the currently bound resource.                                     |
| `update()`            | Updates the internal state (e.g., buffers, transforms).                   |
| `tick()`              | Advances one logical frame or simulation step.                            |
| `step()`              | Advances by one custom-defined step (e.g., physics simulation).           |
| `apply()`             | Applies pending settings or configuration changes.                        |
| `load()`              | Loads data from disk or memory.                                           |
| `save()`              | Saves data to disk or memory.                                             |
| `serialize()`         | Converts object to a binary or textual format.                            |
| `deserialize()`       | Restores object from a binary or textual format.                          |
| `clone()`             | Performs deep copy of the object.                                         |
| `copy_from()`         | Copies values from another object.                                        |
| `move_from()`         | Moves values from another object.                                         |
| `is_valid()`          | Checks if the object is in a usable state.                                |
| `mark_dirty()`        | Marks an object as needing update or refresh.                             |
| `invalidate()`        | Invalidates internal state or references.                                 |


---

## 🧠 Notes

- Try to avoid inventing new names for operations that already have common terminology.
- Consistent naming drastically reduces mental overhead when navigating large codebases.
- For internal APIs, prefer `reset()` over `clear()` when no memory is freed.
- Methods that allocate/deallocate should have clear, strong names (e.g., `destroy`, `release`).

---
