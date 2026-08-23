#include "entity.h"

namespace entis
{
    Entity::Entity(const EntityId id, Registry &registry)
        : m_id(id), m_registry(registry)
    {

    }
}
