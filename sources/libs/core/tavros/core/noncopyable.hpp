#pragma once

namespace tavros::core
{
    /**
     * @brief Noncopyable class that does not allow copying.
     */
    class noncopyable
    {
    public:
        noncopyable() noexcept = default;
        noncopyable(const noncopyable&) noexcept = delete;
        noncopyable& operator=(const noncopyable&) noexcept = delete;
    };
} // namespace tavros::core
