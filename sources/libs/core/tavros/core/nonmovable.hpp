#pragma once

namespace tavros::core
{
    /**
     * @brief Nonmovable class that does not allow moving.
     */
    class nonmovable
    {
    public:
        nonmovable() noexcept = default;
        nonmovable(nonmovable&&) noexcept = delete;
        nonmovable& operator=(nonmovable&&) noexcept = delete;
    };
} // namespace tavros::core
