#pragma once
#include "../storageBase.h"
#include <limits>

namespace entis
{
static void smart_resize(std::vector<uint8_t>& v, const size_t new_size)
{
    if (new_size > v.capacity())
    {
        const size_t new_capacity = std::max(v.capacity() * 2, new_size);
        v.reserve(new_capacity);
    }
    v.resize(new_size);
}

class TypeErasedStorage final : public StorageBase
{
private:
    static constexpr size_t INVALID = std::numeric_limits<size_t>::max();
    static constexpr size_t PAGE_SIZE = 256;
    static constexpr size_t PAGE_BITS = 8;

    using Page = std::array<size_t, PAGE_SIZE>;

    std::vector<Page*> sparse_pages;
    std::vector<uint8_t> componentData;  // Packed component bytes
    std::vector<EntityId> dense;
    ComponentMetadata m_metadata;

public:
    TypeErasedStorage(size_t type, size_t size, size_t alignment);
    ~TypeErasedStorage() override;

    TypeErasedStorage(const TypeErasedStorage&) = delete;
    TypeErasedStorage& operator=(const TypeErasedStorage&) = delete;

    void* emplace(EntityId entityID, const void* data = nullptr);
    void remove(EntityId id) override;

    [[nodiscard]] size_t indexOf(EntityId entityID) const override;

    [[nodiscard]] bool has(EntityId id) const override;
    [[nodiscard]] size_t size() const override;
    [[nodiscard]] EntityId entityAt(size_t index) const override;
    [[nodiscard]] std::vector<EntityId> const& getDenseEntities() const override;
    [[nodiscard]] const ComponentMetadata& metadata() const override;

    [[nodiscard]] void* getOpaquePtr(EntityId id) override;
    [[nodiscard]] void* getOpaquePtrByIndex(size_t index) override;

    void copyComponentFrom(const StorageBase&, EntityId, EntityId) override {}
    [[nodiscard]] grl::Box<StorageBase> cloneEmpty() const override;
    void* emplaceOpaquePtr(EntityId id, const void *data) override;
private:
    [[nodiscard]] const Page* sparsePageFor(EntityId entityID) const;
    size_t& sparseEntryAt(EntityId entityID);
};
}
