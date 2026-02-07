#include "typeErasedStorage.h"
#include <cstring>

namespace entis
{
    TypeErasedStorage::TypeErasedStorage(size_t type, const size_t size, const size_t alignment)
        : m_metadata(type, size, alignment, typeid(void))
    {
        m_metadata.getByIndex = [this](const size_t idx) { return this->getOpaquePtrByIndex(idx); };
        m_metadata.get = [this](const EntityId id) { return this->getOpaquePtr(id); };
    }

    TypeErasedStorage::~TypeErasedStorage()
    {
        for (const auto* page : sparse_pages)
            delete page;
    }

    void * TypeErasedStorage::emplace(const EntityId entityID, const void *data)
    {
        size_t& sparse_entry = sparseEntryAt(entityID);

        // Allocate space in packed array
        const size_t oldSize = componentData.size();
        smart_resize(componentData, oldSize + m_metadata.size);

        void* newComp = componentData.data() + oldSize;

        if (data)
            std::memcpy(newComp, data, m_metadata.size);
        else
            std::memset(newComp, 0, m_metadata.size);

        const size_t index = dense.size();
        dense.push_back(entityID);
        sparse_entry = index;

        return newComp;
    }

    void TypeErasedStorage::remove(EntityId id)
    {
        const size_t idx = indexOf(id);
        if (idx == INVALID) return;

        const size_t last = dense.size() - 1;
        const EntityId last_entity = dense.back();

        if (idx != last)
        {
            // Copy last component to removed slot
            void* dstComp = componentData.data() + (idx * m_metadata.size);
            const void* srcComp = componentData.data() + (last * m_metadata.size);
            std::memcpy(dstComp, srcComp, m_metadata.size);

            dense[idx] = last_entity;
            sparseEntryAt(last_entity) = idx;
        }

        sparseEntryAt(id) = INVALID;
        componentData.resize(componentData.size() - m_metadata.size);
        dense.pop_back();
    }

    size_t TypeErasedStorage::indexOf(const EntityId entityID) const
    {
        const Page* page = sparsePageFor(entityID);
        if (!page) return INVALID;
        return (*page)[entityID & (PAGE_SIZE - 1)];
    }

    bool TypeErasedStorage::has(const EntityId id) const
    {
        return indexOf(id) != INVALID;
    }

    size_t TypeErasedStorage::size() const
    {
        return dense.size();
    }

    EntityId TypeErasedStorage::entityAt(const size_t index) const
    {
        return dense.at(index);
    }

    std::vector<EntityId> const & TypeErasedStorage::getDenseEntities() const
    {
        return dense;
    }

    const ComponentMetadata & TypeErasedStorage::metadata() const
    {
        return m_metadata;
    }

    void * TypeErasedStorage::getOpaquePtr(const EntityId id)
    {
        const size_t idx = indexOf(id);
        if (idx == INVALID) return nullptr;
        return componentData.data() + (idx * m_metadata.size);
    }

    void * TypeErasedStorage::getOpaquePtrByIndex(size_t index)
    {
        if (index >= dense.size()) return nullptr;
        return componentData.data() + (index * m_metadata.size);
    }

    grl::Box<StorageBase> TypeErasedStorage::cloneEmpty() const
    {
        return grl::makeBox<TypeErasedStorage>(m_metadata.type, m_metadata.size, m_metadata.alignment);
    }

    void* TypeErasedStorage::emplaceOpaquePtr(EntityId id, const void *data)
    {
        if (has(id))
        {
            return getOpaquePtr(id);
        }

        size_t& sparse_entry = sparseEntryAt(id);

        // allocate space in packed array
        const size_t oldSize = componentData.size();
        smart_resize(componentData, oldSize + m_metadata.size);

        void* newComp = componentData.data() + oldSize;

        if (data)
            std::memcpy(newComp, data, m_metadata.size);
        else
            std::memset(newComp, 0, m_metadata.size);

        const size_t index = dense.size();
        dense.push_back(id);
        sparse_entry = index;

        return newComp;
    }

    const TypeErasedStorage::Page * TypeErasedStorage::sparsePageFor(const EntityId entityID) const
    {
        const size_t page_index = entityID >> PAGE_BITS;
        if (page_index >= sparse_pages.size()) return nullptr;
        return sparse_pages[page_index];
    }

    size_t & TypeErasedStorage::sparseEntryAt(const EntityId entityID)
    {
        const size_t page_index = entityID >> PAGE_BITS;
        const size_t offset = entityID & (PAGE_SIZE - 1);

        if (page_index >= sparse_pages.size())
            sparse_pages.resize(page_index + 1, nullptr);

        if (!sparse_pages[page_index])
        {
            sparse_pages[page_index] = new Page;
            sparse_pages[page_index]->fill(INVALID);
        }

        return (*sparse_pages[page_index])[offset];
    }
}
