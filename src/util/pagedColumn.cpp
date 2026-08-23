#include "pagedColumn.h"

#include <cassert>

namespace entis
{
    void* PagedColumn::get(const size_t index)
    {
        assert(index < m_size);

        const size_t pageIndex = index / m_elementsPerPage;
        const size_t offset = index % m_elementsPerPage;

        return &m_pages[pageIndex][offset * m_elementSize];
    }

    void* PagedColumn::back()
    {
        return get(m_size-1);
    }

    void* PagedColumn::allocateBack()
    {
        const size_t index = m_size++;

        const size_t pageIndex = index / m_elementsPerPage;
        const size_t offset = index % m_elementsPerPage;

        if (pageIndex >= m_pages.size())
            m_pages.resize(pageIndex + 1);

        if (!m_pages[pageIndex])
        {
            m_pages[pageIndex] = allocatePage();
        }

        return &m_pages[pageIndex][offset * m_stride];
    }

    void PagedColumn::popBack()
    {
        assert(m_size > 0);
        m_size--;
    }


    size_t PagedColumn::size() const
    {
        return m_size;
    }

    std::unique_ptr<std::byte[], PagedColumn::PageDeleter> PagedColumn::allocatePage()
    {
        const size_t bytes = m_elementsPerPage * m_stride;

        void* memory = operator new(
            bytes,
            static_cast<std::align_val_t>(m_alignment)
        );

        return {static_cast<std::byte*>(memory), PageDeleter{m_alignment}};
    }
}
