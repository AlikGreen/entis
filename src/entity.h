#pragma once

#include "registry.h"

namespace Neon::ECS
{
class Entity
{
public:
    static Entity null();

    template<typename T>
    T& get()
    {
        return m_registry->get<T>(m_id);
    }

    template<typename T>
    [[nodiscard]] bool has() const
    {
        return m_registry->has<T>(m_id);
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args)
    requires std::constructible_from<T, Args...>
    {
        return m_registry->emplace<T>(m_id, std::forward<Args>(args)...);
    }

    template<typename T>
    void remove() const
    {
        m_registry->remove<T>(m_id);
    }

    bool operator==(const Entity& other) const
    {
        return m_id == other.m_id;
    }

    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

    operator bool() const
    {
        return m_id != 0;
    }

    [[nodiscard]] size_t id() const
    {
        return m_id;
    }
private:
    friend class Registry;
    template<typename... Components>
    friend class View;

    explicit Entity(Registry* registry, size_t id);

    Registry* m_registry;
    size_t m_id;
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
    destroy(entity.id());
}

inline bool Registry::isValid(const Entity entity) const
{
    return isValid(entity.id());

}

template<typename T, typename... Args>
T& Registry::emplace(Entity entity, Args&&... args)
{
    return emplace<T>(entity.id(), std::forward<Args>(args)...);
}
inline Entity Registry::getEntity(const EntityID id)
{
    return Entity(this, id);
}

inline TypeErasedRegistry& Registry::asTypeErased()
{
    return typeErasedRegistry;
}

template<typename T>
bool Registry::has(const Entity entity)
{
    return has<T>(entity.id());
}

template<typename T>
T& Registry::get(const Entity entity)
{
    return get<T>(entity.id());
}

template<typename T>
void Registry::remove(const Entity entity)
{
    remove<T>(entity.id());
}

}

template<>
struct std::hash<Neon::ECS::Entity>
{
    size_t operator()(const Neon::ECS::Entity& e) const noexcept
    {
        return e.id();
    }
};
