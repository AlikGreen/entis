#pragma once
#include "../storageBase.h"
#include <limits>

namespace Neon::ECS
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
    std::vector<EntityID> dense;
    ComponentMetadata m_metadata;

public:
    TypeErasedStorage(size_t type, size_t size, size_t alignment);
    ~TypeErasedStorage() override;

    TypeErasedStorage(const TypeErasedStorage&) = delete;
    TypeErasedStorage& operator=(const TypeErasedStorage&) = delete;

    void* emplace(EntityID entityID, const void* data = nullptr);
    void remove(EntityID id) override;

    [[nodiscard]] size_t indexOf(EntityID entityID) const override;

    [[nodiscard]] bool has(EntityID id) const override;
    [[nodiscard]] size_t size() const override;
    [[nodiscard]] EntityID entityAt(size_t index) const override;
    [[nodiscard]] std::vector<EntityID> const& getDenseEntities() const override;
    [[nodiscard]] const ComponentMetadata& metadata() const override;

    [[nodiscard]] void* getOpaquePtr(EntityID id) override;
    [[nodiscard]] void* getOpaquePtrByIndex(size_t index) override;

    void copyComponentFrom(const StorageBase&, EntityID, EntityID) override {}
    [[nodiscard]] Box<StorageBase> cloneEmpty() const override;
    void* emplaceOpaquePtr(EntityID id, const void *data) override;
private:
    [[nodiscard]] const Page* sparsePageFor(EntityID entityID) const;
    size_t& sparseEntryAt(EntityID entityID);
};
}
