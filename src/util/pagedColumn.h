#pragma once
#include <cassert>
#include <memory>
#include <vector>

namespace entis
{

class  PagedColumn
{
public:
    PagedColumn(const size_t elementSize, const size_t alignment, const size_t elementsPerPage)
         : m_elementSize(elementSize),
           m_alignment(alignment),
           m_stride(((elementSize + alignment - 1) / alignment) * alignment),
           m_elementsPerPage(elementsPerPage)
    {
        assert(m_alignment > 0);
        assert(m_elementsPerPage > 0);
    }

    void* get(size_t index);
    void* back();
    void* allocateBack();
    void popBack();
    [[nodiscard]] size_t size() const;

    [[nodiscard]] void* pageData(const size_t page) const { return m_pages[page].get(); }   // for batch iteration/chunk handoff
    [[nodiscard]] size_t elementsPerPage() const { return m_elementsPerPage; }
    [[nodiscard]] size_t pageSize() const { return m_elementsPerPage * m_elementSize; }
    [[nodiscard]] size_t pageCount() const { return m_pages.size(); }
private:
    struct PageDeleter
    {
        size_t alignment;

        void operator()(std::byte* ptr) const
        {
            operator delete(ptr, static_cast<std::align_val_t>(alignment));
        }
    };

    std::unique_ptr<std::byte[], PageDeleter> allocatePage();

    size_t m_elementSize;
    size_t m_alignment;
    size_t m_stride;
    size_t m_elementsPerPage;
    size_t m_size{};
    std::vector<std::unique_ptr<std::byte[], PageDeleter>> m_pages;
};
}
