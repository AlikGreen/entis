#include "entity.h"

#include "registry.h"

namespace Neon::ECS
{
    Entity Entity::null()
    {
        return Entity(nullptr, 0);
    }

    Entity::Entity(Registry *registry, const size_t id) : m_registry(registry), m_id(id)
    {
    }
}
