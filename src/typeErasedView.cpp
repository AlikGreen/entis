#include "typeErasedView.h"


namespace Neon::ECS
{
    TypeErasedView::TypeErasedView(Registry* registry, std::vector<StorageBase*> storages)
        : ViewBase(registry), m_storages(std::move(storages))
    {
        rebuild();
    }

bool TypeErasedView::next()
{
    if (m_currentIndex >= m_entities.size())
        return false;
    m_currentIndex++;
    return true;
}

void TypeErasedView::reset()
{
    m_currentIndex = 0;
}

const TypeErasedView::ComponentPack& TypeErasedView::current() const
{
    if (m_currentIndex == 0 || m_currentIndex > m_entities.size())
    {
        // Return empty pack
        static const ComponentPack emptyPack = {};
        return emptyPack;
    }

    const size_t idx = m_currentIndex - 1;

    // Update cached pack
    m_cachedPack.entityId = m_entities[idx];
    m_cachedPack.components = m_componentPtrs[idx];  // This copies, but m_cachedPack persists

    return m_cachedPack;
}

void TypeErasedView::rebuild()
{
    if (m_storages.empty()) return;

    // Find smallest storage
    size_t smallestIdx = 0;
    size_t smallestSize = m_storages[0]->size();

    for (size_t i = 1; i < m_storages.size(); ++i)
    {
        size_t s = m_storages[i]->size();
        if (s < smallestSize)
        {
            smallestSize = s;
            smallestIdx = i;
        }
    }

    // Build entity list
    auto* smallest = m_storages[smallestIdx];

    for (size_t i = 0; i < smallest->size(); ++i)
    {
        EntityID entityId = smallest->entityAt(i);

        // Check all storages have this entity
        std::vector<void*> ptrs;
        ptrs.reserve(m_storages.size());
        bool existsInAll = true;

        for (auto* storage : m_storages)
        {
            const size_t idx = storage->indexOf(entityId);
            if (idx == std::numeric_limits<size_t>::max())
            {
                existsInAll = false;
                break;
            }
            ptrs.push_back(storage->getOpaquePtrByIndex(idx));
        }

        if (existsInAll)
        {
            m_entities.push_back(entityId);
            m_componentPtrs.push_back(std::move(ptrs));
        }
    }
}
}
