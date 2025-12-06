#pragma once
#include <typeindex>
#include <unordered_map>
#include <ranges>
#include <vector>

#include "storage.h"
#include "viewBase.h"

#include "neonCore/memory.h"

namespace Neon::ECS
{
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

    void merge(Registry const& other);
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

    template<typename... Components>
    const View<Components...>& view();

    template<typename T>
    Storage<T>& storage()
    {
        const std::type_index type = typeid(T);

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

    std::unordered_map<std::type_index, Box<StorageBase>> componentStorages{};
    std::unordered_map<std::type_index, Box<ViewBase>> viewCache{};
    std::vector<EntityID> freeEntities{};
    size_t nextEntity = 0;
    size_t version = 0;
};
}