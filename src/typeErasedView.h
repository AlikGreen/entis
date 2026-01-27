#pragma once

#include "storageBase.h"
#include "viewBase.h"

namespace Neon::ECS
{
class TypeErasedView final : public ViewBase
{
public:
    struct ComponentPack
    {
        EntityID entityId;
        std::vector<void*> components;
    };

    explicit TypeErasedView(Registry* registry, std::vector<StorageBase*> storages);

    [[nodiscard]] size_t size() const { return m_entities.size(); }
    [[nodiscard]] ComponentPack at(size_t index) const;

private:
    std::vector<StorageBase*> m_storages;
    std::vector<EntityID> m_entities;
    std::vector<std::vector<void*>> m_componentPtrs;
    size_t m_currentIndex = 0;

    void rebuild();
};
}
