#pragma once
#include <cstdint>

namespace Neon::ECS
{
    class Registry;

    class ViewBase
    {
    public:
        virtual ~ViewBase() = default;

        explicit ViewBase(Registry* registry);
    protected:
        friend class Registry;
        Registry* registry;
        size_t version{};
    };
}