#include "typeErasedRegistry.h"

#include "typeErasedStorage.h"
#include "../registry.h"
#include "../entity.h"

namespace Neon::ECS
{
    TypeErasedRegistry::TypeErasedRegistry(Registry *registry)
        : m_registry(registry) { }

    TypeErasedView& TypeErasedRegistry::view(const std::vector<uint64_t>& componentTypes)
    {
        std::vector<StorageBase*> storages;
        storages.reserve(componentTypes.size());

        const size_t combinedType = hashVector(componentTypes);

        const auto it = typeErasedViewCache.find(combinedType);
        if (it != typeErasedViewCache.end())
        {
            if (it->second->version == m_registry->version)
                return *it->second;
        }

        for (auto& type : componentTypes)
        {
            storages.push_back(&storage(type));
        }

        auto newView = makeBox<TypeErasedView>(m_registry, std::move(storages));
        TypeErasedView *viewPtr = newView.get();
        typeErasedViewCache[combinedType] = std::move(newView);

        return *viewPtr;
    }

    void* TypeErasedRegistry::emplace(const Entity entity, const TypeId type, const void* data) const
    {
        ++m_registry->version;
        return storage(type).emplaceOpaquePtr(entity.id(), data);
    }

    StorageBase& TypeErasedRegistry::storage(const TypeId type) const
    {
        if (m_registry->componentStorages.size() <= type)
        {
            assert(registeredTypeErasedTypes.contains(type) && "Type was viewed without being registered please register the type first");

            auto info = registeredTypeErasedTypes.at(type);
            m_registry->componentStorages[type] = makeBox<TypeErasedStorage>(type, info.size, info.alignment);
        }

        return *m_registry->componentStorages.at(type).get();
    }

    void TypeErasedRegistry::registerType(const TypeId type, const size_t size, const size_t alignment)
    {
        registeredTypeErasedTypes[type] = { size, alignment };
    }
}
