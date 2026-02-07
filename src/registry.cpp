#include "registry.h"

#include <unordered_set>

#include "entity.h"

namespace entis
{
    Registry::Registry(): typeErasedRegistry(this) { }

    std::vector<Entity> Registry::merge(Registry const& other)
    {
        std::unordered_map<EntityId, EntityId> entityIDMap;
        std::unordered_set<EntityId> otherEntities;

        for (const auto &storagePtr: other.componentStorages | std::views::values)
        {
            const auto& denseEntities = storagePtr->getDenseEntities();
            for (EntityId entityID : denseEntities)
            {
                otherEntities.insert(entityID);
            }
        }

        for (EntityId oldEntityID : otherEntities)
        {
            const Entity newEntity = createEntity();
            entityIDMap[oldEntityID] = newEntity.id();
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

        std::vector<Entity> newEntities{};
        for (const auto id : entityIDMap | std::views::values)
        {
            newEntities.push_back(Entity(this, id));
        }

        return newEntities;
    }
}
