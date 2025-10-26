#include "entity.h"

#include "registry.h"

namespace Neon::ECS
{
    Entity::Entity(Registry *registry, const size_t id) : registry(registry), id(id)
    {
    }
}
