#include "entity.h"

#include "registry.h"

namespace entis
{
    Entity Entity::null()
    {
        return Entity(nullptr, 0);
    }

    bool Entity::operator==(const Entity &other) const
    {
        return m_id == other.m_id;
    }

    bool Entity::operator!=(const Entity &other) const
    {
        return !(*this == other);
    }

    Entity::operator bool() const
    {
        return isValid();
    }


    size_t Entity::id() const
    {
        return m_id;
    }

    bool Entity::isValid() const
    {
        return m_registry != nullptr && m_id != 0;
    }

    Entity::Entity(Registry *registry, const size_t id) : m_registry(registry), m_id(id)
    {
    }
}
