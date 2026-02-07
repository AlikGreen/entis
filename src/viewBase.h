#pragma once
#include <cstdint>

namespace entis
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