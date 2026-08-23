#pragma once
#include <bitset>

#include "componentMeta.h"
#include "util/pagedColumn.h"
#include "util/pagedVector.h"
#include "util/staticVector.h"


namespace entis
{
using EntityId = uint64_t;

constexpr size_t kMaxComponents = 256;
using ArchetypeSignature = std::bitset<kMaxComponents>;

class Archetype
{
public:
    void addEntity(EntityId entityId);
    void* set(EntityId entityId, ComponentId componentId, void* data);
    void remove(EntityId entityId);
    void moveComponents(EntityId entityId, Archetype& newArchetype);


    [[nodiscard]] EntityId entityAt(size_t row) const;
    [[nodiscard]] size_t rowOf(EntityId id) const;
    [[nodiscard]] size_t rows() const;
private:
    friend class EcsContext;
    friend class Registry;
    explicit Archetype(const StaticVector<ComponentMeta, 8> &componentMetas);

    int findColumnIndex(ComponentId id);
    PagedColumn* findColumn(ComponentId id);
    ComponentMeta getMeta(ComponentId id);

    PagedVector<size_t, 1024> m_entityToRow;
    std::vector<EntityId> m_rowToEntity;
    std::vector<PagedColumn> m_columns;
    StaticVector<ComponentMeta, 8> m_componentMetas;
};
}
