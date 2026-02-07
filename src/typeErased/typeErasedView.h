#pragma once

#include <span>

#include "../storageBase.h"
#include "../viewBase.h"

namespace entis
{
class TypeErasedView final : public ViewBase
{
public:
    TypeErasedView(const TypeErasedView&) = delete;
    TypeErasedView& operator=(const TypeErasedView&) = delete;

    struct ComponentPack
    {
        EntityId entityId;
        std::span<void* const> components;
    };

    explicit TypeErasedView(Registry* registry, std::vector<StorageBase*> storages);

    [[nodiscard]] size_t size() const { return m_entities.size(); }
    [[nodiscard]] ComponentPack at(size_t index) const;

private:
    friend class TypeErasedRegistry;

    std::vector<StorageBase*> m_storages;
    std::vector<EntityId> m_entities;
    std::vector<std::vector<void*>> m_componentPtrs;
    size_t m_currentIndex = 0;

    void rebuild();
};
}
