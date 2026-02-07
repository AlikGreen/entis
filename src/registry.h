#pragma once
#include <cassert>
#include <map>
#include <ranges>
#include <vector>

#include "storage.h"
#include "viewBase.h"

#include <grl/grl.h>

#include "typeErased/typeErasedRegistry.h"

namespace entis
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
    Entity createEntityWithId(EntityId id);

    template<typename T, typename... Args>
    requires std::constructible_from<T, Args...>
    T& emplace(Entity entity, Args&&... args);

    template<typename T>
    T& assign(Entity entity, T&& component);

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

    Entity getEntity(EntityId id);
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
            componentStorages[type] = grl::makeBox<Storage<T>>();

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
    T& assign(size_t entityId, T&& component)
    {
        ++version;
        return storage<T>().assign(entityId, std::forward<T>(component));
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

    std::map<TypeId, grl::Box<StorageBase>> componentStorages{};
    std::map<TypeId, grl::Box<ViewBase>> viewCache{};
    std::vector<EntityId> freeEntities{};
    size_t nextEntity = 1;
    size_t version = 0;
};

template<typename T>
void TypeErasedRegistry::registerType()
{
    const TypeId type = typeid(T).hash_code();

    m_registry->storage<T>();

    registeredTypeErasedTypes[type] = { sizeof(T), alignof(T) };
}
}
