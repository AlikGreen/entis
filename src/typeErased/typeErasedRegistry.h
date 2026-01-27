#pragma once
#include <cstdint>
#include <vector>

#include "typeErasedView.h"
#include "../storageBase.h"

namespace Neon::ECS
{
    class Entity;
    class Registry;
    using TypeId = uint64_t;

class TypeErasedRegistry
{
public:
    explicit TypeErasedRegistry(Registry* registry);

    TypeErasedRegistry(const TypeErasedRegistry&) = delete;
    TypeErasedRegistry& operator=(const TypeErasedRegistry&) = delete;

    TypeErasedView& view(const std::vector<uint64_t>& componentTypes);
    void* emplace(Entity entity, TypeId type, const void* data) const;

    void registerType(TypeId type, size_t size, size_t alignment);

    template<typename T>
    void registerType()
    {
        registeredTypeErasedTypes[typeid(T).hash_code()] = { sizeof(T), alignof(T) };
    }

    void remove(TypeId type, Entity entityId) const;
private:
    StorageBase& storage(TypeId type) const;

    struct TypeErasedType
    {
        size_t size;
        size_t alignment;
    };

    Registry* m_registry;
    std::unordered_map<TypeId, Box<TypeErasedView>> typeErasedViewCache{};
    std::unordered_map<TypeId, TypeErasedType> registeredTypeErasedTypes{};
};
}
