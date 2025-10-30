#pragma once
#include <typeindex>
#include <unordered_map>
#include <ranges>
#include <vector>

#include "storage.h"
#include "view.h"
#include "neonCore/memory.h"

namespace Neon::ECS
{
class Entity;

class Registry
{
public:
    Registry() = default;

    void merge(Registry const& other);
    Entity createEntity();

    template<typename T, typename... Args>
    T& emplace(EntityID entity, Args&&... args)
    {
        return storage<T>().emplace(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    bool has(const EntityID entity)
    {
        return storage<T>().has(entity);
    }

    template<typename T>
    T& get(const EntityID entity)
    {
        return storage<T>().get(entity);
    }

    template<typename T>
    void remove(const EntityID entity)
    {
        storage<T>().remove(entity);
        freeEntities.push_back(entity);
    }

    void destroy(const EntityID entity)
    {
        freeEntities.push_back(entity);
        for (const auto &storage: componentStorages | std::views::values)
        {
            storage->remove(entity);
        }
    }

    template<typename... Components>
    View<Components...> view()
    {
        return View<Components...>(storage<Components>()...);
    }

    template<typename T>
    Storage<T>& storage()
    {
        const std::type_index type = typeid(T);

        if (!componentStorages.contains(type))
            componentStorages[type] = std::make_unique<Storage<T>>();

        return *static_cast<Storage<T>*>(componentStorages.at(type).get());
    }
private:
    std::unordered_map<std::type_index, Box<StorageBase>> componentStorages{};
    std::vector<EntityID> freeEntities{};
    size_t nextEntity = 0;
};
}