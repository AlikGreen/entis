#pragma once
#include <assert.h>
#include <typeindex>
#include <unordered_map>
#include <ranges>
#include <vector>

#include "storage.h"
#include "viewBase.h"

#include <neonCore/neonCore.h>

#include "typeErasedStorage.h"
#include "typeErasedView.h"

namespace Neon::ECS
{
static size_t hashVector(const std::vector<size_t>& vec)
{
    size_t result = 0;
    for (const size_t val : vec)
    {
        // Mix the element to reduce collisions
        const size_t mixed = val * 0x9e3779b97f4a7c15; // arbitrary large prime
        result ^= mixed; // XOR is order-independent
    }
    return result;
}

class Entity;

class ViewBase;
template<typename... Components>
class View;

class Registry
{
public:
    Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    std::vector<Entity> merge(Registry const& other);
    Entity createEntity();

    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args);

    template<typename T>
    bool has(Entity entity);

    template<typename T>
    T& get(Entity entity);

    template<typename T>
    void remove(Entity entity);
    void destroy(Entity entity);

    bool isValid(Entity entity) const;

    template<typename... Components>
    const View<Components...>& view();

    TypeErasedView& viewTypeErased(const std::vector<size_t>& componentTypes)
    {
        std::vector<StorageBase*> storages;
        storages.reserve(componentTypes.size());

        const size_t combinedType = hashVector(componentTypes);

        const auto it = typeErasedViewCache.find(combinedType);
        if (it != typeErasedViewCache.end())
        {
            if (it->second->version == version)
                return *it->second;
        }


        for (auto& type : componentTypes)
        {
            auto* storage = &storageTypeErased(type);
            storages.push_back(storage);
        }

        auto newView = makeBox<TypeErasedView>(this, std::move(storages));
        TypeErasedView *viewPtr = newView.get();
        typeErasedViewCache[combinedType] = std::move(newView);

        return *viewPtr;
    }

    void* emplaceTypeErased(Entity entity, size_t type, const void* data);

    StorageBase& storageTypeErased(const size_t type)
    {
        if (!componentStorages.contains(type))
        {
            assert(registeredTypeErasedTypes.contains(type) && "Type was viewed without being registered please register the type first");

            auto info = registeredTypeErasedTypes.at(type);
            componentStorages[type] = makeBox<TypeErasedStorage>(type, info.size, info.alignment);
        }

        return *componentStorages.at(type).get();
    }

    // Only needs to be done for type erased types
    void registerType(const size_t type, const size_t size, const size_t alignment)
    {
        registeredTypeErasedTypes[type] = { size, alignment };
    }


    template<typename T>
    Storage<T>& storage()
    {
        const size_t type = typeid(T).hash_code();

        if (!componentStorages.contains(type))
            componentStorages[type] = makeBox<Storage<T>>();

        return *static_cast<Storage<T>*>(componentStorages.at(type).get());
    }
private:
    friend class Entity;
    friend class ViewBase;

    template<typename T, typename... Args>
    T& emplace(size_t entityId, Args&&... args)
    {
        ++version;
        return storage<T>().emplace(entityId, std::forward<Args>(args)...);
    }

    template<typename T>
    bool has(size_t entityId)
    {
        return storage<T>().has(entityId);
    }

    template<typename T>
    T& get(size_t entityId)
    {
        return storage<T>().get(entityId);
    }

    template<typename T>
    void remove(size_t entityId)
    {
        ++version;
        storage<T>().remove(entityId);
    }

    void destroy(const size_t entityId)
    {
        ++version;
        freeEntities.push_back(entityId);
        for (const auto &storage: componentStorages | std::views::values)
        {
            storage->remove(entityId);
        }
    }

    bool isValid(const size_t entityId) const
    {
        // Check if ID is 0 (invalid sentinel)
        if (entityId == 0)
            return false;

        // Check if entity was never created
        if (entityId >= nextEntity)
            return false;

        // Check if entity has been destroyed
        if (std::ranges::find(freeEntities, entityId) != freeEntities.end())
            return false;

        return true;
    }

    struct TypeErasedType
    {
        size_t size;
        size_t alignment;
    };

    std::unordered_map<size_t, Box<StorageBase>> componentStorages{};
    std::unordered_map<size_t, Box<ViewBase>> viewCache{};
    std::unordered_map<size_t, Box<TypeErasedView>> typeErasedViewCache{};
    std::unordered_map<size_t, TypeErasedType> registeredTypeErasedTypes{};
    std::vector<EntityID> freeEntities{};
    size_t nextEntity = 0;
    size_t version = 0;
};
}
