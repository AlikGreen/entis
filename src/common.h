#pragma once

#include <memory>

namespace Neon
{
    template<typename T>
    using Rc   = std::shared_ptr<T>;

    template<typename T>
    using Box = std::unique_ptr<T>;

    template<typename T, typename... Args>
    constexpr Rc<T> makeRc(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    constexpr Box<T> makeBox(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}
