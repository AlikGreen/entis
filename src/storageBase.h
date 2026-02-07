#pragma once
#include <functional>
#include <typeindex>
#include <grl/grl.h>

namespace entis
{
typedef uint32_t EntityId;
struct ComponentMetadata
{
    size_t type;
    size_t size;
    size_t alignment;
    std::type_index typeIndex;

    std::function<void*(size_t)> getByIndex;
    std::function<void*(EntityId)> get;

    ComponentMetadata(const size_t t, const size_t s, const size_t a, const std::type_index ti)
        : type(t), size(s), alignment(a), typeIndex(ti) {}
};

class StorageBase
{
public:
    virtual ~StorageBase() = default;
    virtual void remove(EntityId id) = 0;
    [[nodiscard]] virtual size_t indexOf(EntityId entityID) const = 0;
    [[nodiscard]] virtual bool has(EntityId id) const = 0;
    [[nodiscard]] virtual size_t size() const = 0;
    [[nodiscard]] virtual EntityId entityAt(size_t index) const = 0;
    [[nodiscard]] virtual std::vector<EntityId> const& getDenseEntities() const = 0;

    virtual void copyComponentFrom(const StorageBase& other, EntityId oldID, EntityId newID) = 0;
    [[nodiscard]] virtual grl::Box<StorageBase> cloneEmpty() const = 0;

    [[nodiscard]] virtual const ComponentMetadata& metadata() const = 0;
    [[nodiscard]] virtual void* getOpaquePtr(EntityId id) = 0;
    [[nodiscard]] virtual void* getOpaquePtrByIndex(size_t index) = 0;
    virtual void* emplaceOpaquePtr(EntityId id, const void* data) = 0;
};

}
