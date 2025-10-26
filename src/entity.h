#pragma once

#include "registry.h"

namespace Neon::ECS
{
class Entity
{
public:
    template<typename T>
    T& get()
    {
        return registry->get<T>(id);;
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args)
    {
        return registry->emplace<T>(id, std::forward<Args>(args)...);
    }

    template<typename T>
    void remove()
    {
        registry->remove<T>(id);
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator size_t() const
    {
        return id;
    }
private:
    friend class Registry;

    explicit Entity(Registry* registry, size_t id);

    Registry* registry;
    size_t id;
};

inline Entity Registry::createEntity()
{
    if (freeEntities.empty())
        return Entity(this, nextEntity++);

    const EntityID id = freeEntities.back();
    freeEntities.pop_back();
    return Entity(this, id);
}
}
