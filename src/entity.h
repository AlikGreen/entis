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
        return registry->get<T>(id);
    }

    template<typename T>
    [[nodiscard]] bool has() const
    {
        return registry->has<T>(id);
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args)
    {
        return registry->emplace<T>(id, std::forward<Args>(args)...);
    }

    template<typename T>
    void remove() const
    {
        registry->remove<T>(id);
    }
private:
    friend class Registry;
    template<typename... Components>
    friend class View;

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

inline void Registry::destroy(const Entity entity)
{
    freeEntities.push_back(entity.id);
    for (const auto &storage: componentStorages | std::views::values)
    {
        storage->remove(entity.id);
    }
}

template<typename T, typename... Args>
T& Registry::emplace(Entity entity, Args&&... args)
{
    return emplace<T>(entity.id, std::forward<Args>(args)...);
}

template<typename T>
bool Registry::has(const Entity entity)
{
    return has<T>(entity.id);
}

template<typename T>
T& Registry::get(const Entity entity)
{
    return get<T>(entity.id);
}

template<typename T>
void Registry::remove(const Entity entity)
{
    remove<T>(entity.id);
}

}
