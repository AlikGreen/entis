#include "archetype.h"

#include <cassert>

namespace entis
{
    PagedVector<EntityId, Archetype::kElementsPerPage> & Archetype::rowEntities()
    {
        return m_rowToEntity;
    }

    Archetype::Archetype(const StaticVector<ComponentMeta, 8>& componentMetas)
        : m_componentMetas(componentMetas)
    {
        for(const auto& meta : m_componentMetas)
        {
            m_columns.emplace_back(meta.size, meta.alignment, kElementsPerPage);
        }
    }


    void Archetype::addEntity(const EntityId entityId)
    {
        const size_t row = m_rowToEntity.size();

        m_entityToRow.set(entityId, row);
        m_rowToEntity.push_back(entityId);

        for (PagedColumn& column : m_columns)
        {
            column.allocateBack();
        }
    }

    void* Archetype::set(const EntityId entityId, const ComponentId componentId, void *data)
    {
        const size_t row = *m_entityToRow.get(entityId); // FIXME

        PagedColumn* column = findColumn(componentId);
        const ComponentMeta& meta = getMeta(componentId);

        assert(row < column->size());

        void* dst = column->get(row);

        meta.moveCtor(dst, data);
        meta.dtor(data);

        return dst;
    }

    void Archetype::remove(const EntityId entityId)
    {
        const size_t row = *m_entityToRow.get(entityId); // FIXME
        const size_t lastRow = rows() - 1;

        for(size_t i = 0; i < m_columns.size(); i++)
        {
            PagedColumn& column = m_columns[i];
            const ComponentMeta& meta = m_componentMetas[i];

            void* toRemove = column.get(row);
            meta.dtor(toRemove);

            if (row != rows()-1)
            {
                void* last = column.back();

                meta.moveCtor(toRemove, last);
                meta.dtor(last);
            }

            column.popBack();
        }

        if (row != lastRow)
        {
            const EntityId movedEntity = m_rowToEntity[lastRow];

            m_rowToEntity[row] = movedEntity;
            m_entityToRow.set(movedEntity, row);
        }

        m_rowToEntity.pop_back();
        m_entityToRow.erase(entityId);
    }

    void Archetype::moveComponents(const EntityId entityId, Archetype &newArchetype)
    {
        const size_t srcRow = *m_entityToRow.get(entityId); // FIXME
        const size_t dstRow = *newArchetype.m_entityToRow.get(entityId); // FIXME

        for (size_t i = 0; i < m_columns.size(); ++i)
        {
            const ComponentMeta& meta = m_componentMetas[i];

            PagedColumn& srcColumn = m_columns[i];
            PagedColumn& dstColumn = *newArchetype.findColumn(meta.id);

            void* src = srcColumn.get(srcRow);
            void* dst = dstColumn.get(dstRow);

            meta.moveCtor(dst, src);
            meta.dtor(src);

        }

        const size_t lastRow = rows() - 1;

        if (srcRow != lastRow)
        {
            for (size_t i = 0; i < m_columns.size(); ++i)
            {
                const ComponentMeta& meta = m_componentMetas[i];
                PagedColumn& column = m_columns[i];

                void* dst = column.get(srcRow);
                void* src = column.get(lastRow);

                meta.moveCtor(dst, src);
                meta.dtor(src);
            }

            const EntityId movedEntity = m_rowToEntity[lastRow];
            m_rowToEntity[srcRow] = movedEntity;
            m_entityToRow.set(movedEntity, srcRow);
        }

        for (PagedColumn& column : m_columns)
        {
            column.popBack();
        }

        m_rowToEntity.pop_back();
        m_entityToRow.erase(entityId);
    }

    EntityId Archetype::entityAt(const size_t row) const
    {
        return m_rowToEntity[row]; // FIXME
    }

    size_t Archetype::rowOf(const EntityId id) const
    {
        return *m_entityToRow.get(id);
    }

    size_t Archetype::rows() const
    {
        return m_rowToEntity.size();
    }

    size_t Archetype::pages() const
    {
        return m_columns[0].pageCount();
    }

    int Archetype::findColumnIndex(const ComponentId id)
    {
        for(int i = 0; i < m_componentMetas.size(); i++)
        {
            if(m_componentMetas[i].id == id)
                return i;
        }

        return -1;
    }

    PagedColumn* Archetype::findColumn(const ComponentId id)
    {
        for(int i = 0; i < m_componentMetas.size(); i++)
        {
            if(m_componentMetas[i].id == id)
                return &m_columns[i];
        }

        return nullptr;
    }

    ComponentMeta Archetype::getMeta(const ComponentId id)
    {
        for(const auto& meta : m_componentMetas)
        {
            if(meta.id == id) return meta;
        }

        return {};
    }
}
