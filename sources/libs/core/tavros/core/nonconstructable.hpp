#pragma once

namespace tavros::core
{
    /**
     * @brief Nonconstructable class that does not allow construction.
     */
    class nonconstructable
    {
    public:
        nonconstructable() = delete;
        ~nonconstructable() = delete;
        nonconstructable(nonconstructable&&) = delete;
        nonconstructable& operator=(nonconstructable&&) = delete;
    };
} // namespace tavros::core
