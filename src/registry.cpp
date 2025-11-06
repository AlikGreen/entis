#include "registry.h"

#include <unordered_set>

#include "entity.h"

namespace Neon::ECS
{
    void Registry::merge(Registry const& other)
    {
        std::unordered_map<EntityID, EntityID> entityIDMap;
        std::unordered_set<EntityID> otherEntities;

        for (const auto &storagePtr: other.componentStorages | std::views::values)
        {
            const auto& denseEntities = storagePtr->getDenseEntities();
            for (EntityID entityID : denseEntities)
            {
                otherEntities.insert(entityID);
            }
        }

        for (EntityID oldEntityID : otherEntities)
        {
            const Entity newEntity = createEntity();
            entityIDMap[oldEntityID] = newEntity;
        }

        for (const auto& [type, otherStoragePtr] : other.componentStorages)
        {
            if (!componentStorages.contains(type))
            {
                componentStorages[type] = otherStoragePtr->cloneEmpty();
            }

            StorageBase* thisStorage = componentStorages[type].get();

            for (const auto& [oldID, newID] : entityIDMap)
            {
                thisStorage->copyComponentFrom(*otherStoragePtr, oldID, newID);
            }
        }

    }
}
