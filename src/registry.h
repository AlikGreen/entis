#pragma once
#include <cassert>
#include <typeindex>
#include <ranges>
#include <vector>
#include <ankerl/unordered_dense.h>

#include "storage.h"
#include "viewBase.h"

#include <neonCore/neonCore.h>

#include "typeErased/typeErasedRegistry.h"

namespace Neon::ECS
{
using TypeId = uint64_t;

class Entity;

class ViewBase;
template<typename... Components>
class View;

class Registry
{
public:
    Registry();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

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

    Entity getEntity(EntityID id);
    TypeErasedRegistry& asTypeErased();
private:
    friend class Entity;
    friend class ViewBase;
    friend class TypeErasedRegistry;
    template<typename... Components>
    friend class View;

    template<typename T>
    Storage<T>& storage()
    {
        static Storage<T>* cached = nullptr;
        static Registry* cachedRegistry = nullptr;

        if (cached && cachedRegistry == this) [[likely]]
            return *cached;

        const uint64_t type = typeid(T).hash_code();

        if (!componentStorages.contains(type))
            componentStorages[type] = makeBox<Storage<T>>();

        cached = static_cast<Storage<T>*>(componentStorages.at(type).get());
        cachedRegistry = this;
        return *cached;
    }

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

    TypeErasedRegistry typeErasedRegistry;

    std::unordered_map<TypeId, Box<StorageBase>> componentStorages{};
    std::unordered_map<TypeId, Box<ViewBase>> viewCache{};
    std::vector<EntityID> freeEntities{};
    size_t nextEntity = 1;
    size_t version = 0;
};
}
