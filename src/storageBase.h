#pragma once
#include <functional>
#include <typeindex>
#include <neonCore/neonCore.h>

namespace Neon::ECS
{
typedef uint32_t EntityID;
struct ComponentMetadata
{
    size_t type;
    size_t size;
    size_t alignment;
    std::type_index typeIndex;

    std::function<void*(size_t)> getByIndex;
    std::function<void*(EntityID)> get;

    ComponentMetadata(const size_t t, const size_t s, const size_t a, const std::type_index ti)
        : type(t), size(s), alignment(a), typeIndex(ti) {}
};

class StorageBase
{
public:
    virtual ~StorageBase() = default;
    virtual void remove(EntityID id) = 0;
    [[nodiscard]] virtual size_t indexOf(EntityID entityID) const = 0;
    [[nodiscard]] virtual bool has(EntityID id) const = 0;
    [[nodiscard]] virtual size_t size() const = 0;
    [[nodiscard]] virtual EntityID entityAt(size_t index) const = 0;
    [[nodiscard]] virtual std::vector<EntityID> const& getDenseEntities() const = 0;

    virtual void copyComponentFrom(const StorageBase& other, EntityID oldID, EntityID newID) = 0;
    [[nodiscard]] virtual Box<StorageBase> cloneEmpty() const = 0;

    [[nodiscard]] virtual const ComponentMetadata& metadata() const = 0;
    [[nodiscard]] virtual void* getOpaquePtr(EntityID id) = 0;
    [[nodiscard]] virtual void* getOpaquePtrByIndex(size_t index) = 0;
    virtual void* emplaceOpaquePtr(EntityID id, const void* data) = 0;
};

}
