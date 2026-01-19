#pragma once
#include <array>
#include <utility>
#include <vector>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <utility>
#include <neonCore/neonCore.h>

#include "storageBase.h"

namespace Neon::ECS
{
typedef uint32_t EntityID;

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
    ComponentMetadata m_metadata;
public:
    Storage()
        : m_metadata(typeid(T).hash_code(), sizeof(T), alignof(T), std::type_index(typeid(T)))
    {
        m_metadata.getByIndex = [this](const size_t idx) -> void* {
            return &this->getByIndex(idx);
        };
        m_metadata.get = [this](const EntityID id) -> void* {
            return &this->get(id);
        };
    }

    ~Storage() override
    {
        for (const Page* page : sparse_pages)
            delete page;
    }

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    template <typename... Args>
    T& emplace(EntityID entityID, Args&&... args)
    {
        size_t& sparse_entry = sparseEntryAt(entityID);

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

        sparseEntryAt(last_entity) = index_to_remove;

        sparseEntryAt(id) = INVALID;

        components.pop_back();
        dense.pop_back();
    }

    [[nodiscard]] size_t indexOf(const EntityID entityID) const override
    {
        const Page* page = sparsePageFor(entityID);

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

    [[nodiscard]] const ComponentMetadata& metadata() const override
    {
        return m_metadata;
    }

    [[nodiscard]] void* getOpaquePtr(const EntityID id) override
    {
        return &get(id);
    }

    [[nodiscard]] void* getOpaquePtrByIndex(const size_t index) override
    {
        return &getByIndex(index);
    }

    void * emplaceOpaquePtr(EntityID id, const void *data) override
    {
        if (has(id))
        {
            return getOpaquePtr(id);
        }

        size_t& sparseEntry = sparseEntryAt(id);
        const size_t index = components.size();

        if (data)
        {
            const T* src = reinterpret_cast<const T*>(data);
            components.emplace_back(*src);
        }
        else
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                components.emplace_back();
            }
            else
            {
                throw std::runtime_error("emplaceOpaquePtr: no data provided for non-default-constructible component type");
            }
        }

        dense.emplace_back(id);
        sparseEntry = index;
        return &components.back();
    }

private:
    [[nodiscard]] const Page* sparsePageFor(const EntityID entityID) const
    {
        const size_t page_index = entityID >> PAGE_BITS;
        if (page_index >= sparse_pages.size())
        {
            return nullptr;
        }
        return sparse_pages[page_index];
    }

    size_t& sparseEntryAt(const EntityID entityID)
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
