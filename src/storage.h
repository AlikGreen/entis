#pragma once
#include <array>
#include <vector>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <neonCore/neonCore.h>

namespace Neon::ECS
{
typedef uint32_t EntityID;

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
};


template <typename T>
class Storage final : public StorageBase
{
private:
    static constexpr size_t INVALID = std::numeric_limits<size_t>::max();
    static constexpr size_t PAGE_SIZE = 256;
    static constexpr size_t PAGE_BITS = 8;

    using Page = std::array<size_t, PAGE_SIZE>;

    std::vector<Page*> sparse_pages;

    std::vector<T> components;
    std::vector<EntityID> dense;


public:
    Storage() = default;

    ~Storage() override
    {
        for (const Page* page : sparse_pages)
        {
            delete page;
        }
    }

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    template <typename... Args>
    T& emplace(EntityID entityID, Args&&... args)
    {
        size_t& sparse_entry = sparse_entry_at(entityID);

        const size_t index = components.size();
        components.emplace_back(std::forward<Args>(args)...);
        dense.emplace_back(entityID);
        sparse_entry = index;

        return components.back();
    }

    void remove(const EntityID id) override
    {
        const size_t index_to_remove = indexOf(id);
        if (index_to_remove == INVALID)
        {
            return;
        }

        const size_t last_index = components.size() - 1;
        const EntityID last_entity = dense.back();

        components[index_to_remove] = std::move(components[last_index]);
        dense[index_to_remove] = last_entity;

        sparse_entry_at(last_entity) = index_to_remove;

        sparse_entry_at(id) = INVALID;

        components.pop_back();
        dense.pop_back();
    }

    [[nodiscard]] size_t indexOf(const EntityID entityID) const override
    {
        const Page* page = sparse_page_for(entityID);

        if (!page)
        {
            return INVALID;
        }

        const size_t offset = entityID & (PAGE_SIZE - 1);
        return (*page)[offset];
    }

    T& get(const EntityID id)
    {
        const size_t index = indexOf(id);
        if (index == INVALID)
        {
            throw std::out_of_range("Entity ID does not exist in this storage or component was removed.");
        }
        return components[index];
    }

    [[nodiscard]] bool has(const EntityID id) const override
    {
        return indexOf(id) != INVALID;
    }

    T& getByIndex(size_t index)
    {
        return components.at(index);
    }

    [[nodiscard]] size_t size() const override
    {
        return components.size();
    }

    [[nodiscard]] EntityID entityAt(const size_t index) const override
    {
        return dense.at(index);
    }

    [[nodiscard]] std::vector<EntityID> const& getDenseEntities() const override
    {
        return dense;
    }

    void copyComponentFrom(const StorageBase& other, EntityID oldID, EntityID newID) override
    {
        const auto& otherStorage = static_cast<const Storage&>(other);
        if (otherStorage.has(oldID))
        {
            const T& component = const_cast<Storage&>(otherStorage).get(oldID);
            emplace(newID, component);
        }
    }

    [[nodiscard]] Box<StorageBase> cloneEmpty() const override
    {
        return makeBox<Storage>();
    }

private:
    [[nodiscard]] const Page* sparse_page_for(const EntityID entityID) const
    {
        const size_t page_index = entityID >> PAGE_BITS;
        if (page_index >= sparse_pages.size())
        {
            return nullptr;
        }
        return sparse_pages[page_index];
    }

    size_t& sparse_entry_at(const EntityID entityID)
    {
        const size_t page_index = entityID >> PAGE_BITS;
        const size_t offset = entityID & (PAGE_SIZE - 1);

        if (page_index >= sparse_pages.size())
        {
            sparse_pages.resize(page_index + 1, nullptr);
        }

        if (!sparse_pages[page_index])
        {
            sparse_pages[page_index] = new Page;
            sparse_pages[page_index]->fill(INVALID);
        }

        return (*sparse_pages[page_index])[offset];
    }
};
}
