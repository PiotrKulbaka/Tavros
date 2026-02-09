#pragma once

#include <memory>
#include <tavros/core/memory/raw_ptr.hpp>
#include <tavros/core/memory/buffer_span.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace tavros::core
{
    template<class T>
    using shared_ptr = std::shared_ptr<T>;

    template<class T, class D = std::default_delete<T>>
    using unique_ptr = std::unique_ptr<T, D>;

    template<class T, typename... Args>
    inline shared_ptr<T> make_shared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<class T, typename... Args>
    inline unique_ptr<T> make_unique(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<class T>
    using enable_shared_from_this = std::enable_shared_from_this<T>;
} // namespace tavros::core
