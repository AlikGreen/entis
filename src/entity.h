#pragma once

#include "registry.h"

namespace entis
{
class Entity
{
public:
    static Entity null();
    explicit Entity(Registry* registry, size_t id);

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

    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;

    operator bool() const;

    [[nodiscard]] size_t id() const;
    bool isValid() const;
private:
    friend class Registry;
    template<typename... Components>
    friend class View;

    Registry* m_registry;
    size_t m_id;
};

inline Entity Registry::createEntity()
{
    if (freeEntities.empty())
        return Entity(this, nextEntity++);

    const EntityId id = freeEntities.back();
    freeEntities.pop_back();
    return Entity(this, id);
}

inline Entity Registry::createEntityWithId(const EntityId id)
{
    if(id >= nextEntity)
    {
        for(EntityId i = nextEntity; i < id; i++)
        {
            freeEntities.push_back(i);
        }

        nextEntity = id + 1;
        return Entity(this, id);
    }

    const auto it = std::ranges::find(freeEntities, id);
    if(it != freeEntities.end())
    {
        freeEntities.erase(it);
        return Entity(this, id);
    }

    return Entity::null();
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
requires std::constructible_from<T, Args...>
T& Registry::emplace(const Entity entity, Args&&... args)
{
    return emplace<T>(entity.id(), std::forward<Args>(args)...);
}

template<typename T>
T & Registry::assign(const Entity entity, T &&component)
{
    return assign<T>(entity.id(), std::forward<T>(component));
}

inline Entity Registry::getEntity(const EntityId id)
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
struct std::hash<entis::Entity>
{
    size_t operator()(const entis::Entity& e) const noexcept
    {
        return e.id();
    }
};
