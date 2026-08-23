#pragma once
#include <cstdint>


namespace entis
{
using EntityId = uint64_t;
class Registry;
class Entity
{
public:
    Entity(EntityId id, Registry& registry);

    [[nodiscard]] EntityId id() const { return m_id; }
    [[nodiscard]] Registry& registry() const { return m_registry; }
private:
    EntityId m_id;
    Registry& m_registry;
};
}
