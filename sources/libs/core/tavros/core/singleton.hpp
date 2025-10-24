#pragma once

#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    // Meyers Singleton
    template<typename T>
    class singleton : noncopyable
    {
    public:
        static T& instance()
        {
            static T instance;
            return instance;
        }

    protected:
        singleton() = default;
        ~singleton() = default;
    };

} // namespace tavros::core
