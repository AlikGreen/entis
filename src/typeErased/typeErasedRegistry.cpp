#include "typeErasedRegistry.h"

#include "typeErasedStorage.h"
#include "../registry.h"
#include "../entity.h"

namespace entis
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


    TypeErasedRegistry::TypeErasedRegistry(Registry *registry)
        : m_registry(registry) { }

    TypeErasedView& TypeErasedRegistry::view(const std::vector<uint64_t>& componentTypes)
    {
        const size_t combinedType = hashVector(componentTypes);

        const auto it = typeErasedViewCache.find(combinedType);
        if (it != typeErasedViewCache.end())
        {
            if (it->second->version == m_registry->version)
                return *it->second;
        }

        m_registry->version++;

        std::vector<StorageBase*> storages;
        storages.reserve(componentTypes.size());

        for (auto& type : componentTypes)
        {
            storages.push_back(&storage(type));
        }

        auto newView = grl::makeBox<TypeErasedView>(m_registry, std::move(storages));
        newView->version = m_registry->version;
        TypeErasedView *viewPtr = newView.get();
        typeErasedViewCache[combinedType] = std::move(newView);

        return *viewPtr;
    }

    void* TypeErasedRegistry::emplace(const Entity entity, const TypeId type, const void* data) const
    {
        ++m_registry->version;
        return storage(type).emplaceOpaquePtr(entity.id(), data);
    }

    void TypeErasedRegistry::remove(const TypeId type, const Entity entityId) const
    {
        ++m_registry->version;
        storage(type).remove(entityId.id());
    }

    Registry & TypeErasedRegistry::getRegistry() const
    {
        return *m_registry;
    }

    StorageBase& TypeErasedRegistry::storage(const TypeId type) const
    {
        if (!m_registry->componentStorages.contains(type))
        {
            assert(registeredTypeErasedTypes.contains(type) &&
                   "Type was viewed without being registered");

            auto info = registeredTypeErasedTypes.at(type);
            m_registry->componentStorages[type] =
                grl::makeBox<TypeErasedStorage>(type, info.size, info.alignment);
        }

        return *m_registry->componentStorages.at(type).get();
    }

    void TypeErasedRegistry::registerType(const TypeId type, const size_t size, const size_t alignment)
    {
        registeredTypeErasedTypes[type] = { size, alignment };
    }
}
