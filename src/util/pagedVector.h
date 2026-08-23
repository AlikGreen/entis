#pragma once
#include <memory>
#include <vector>

namespace entis
{
template<typename T, size_t PageSize>
class PagedVector
{
public:
    PagedVector() = default;

    PagedVector(const PagedVector&) = delete;
    PagedVector& operator=(const PagedVector&) = delete;

    PagedVector(PagedVector&&) noexcept = default;
    PagedVector& operator=(PagedVector&&) noexcept = default;

    T& at(const size_t index) const
    {
        const size_t pageIndex = index / PageSize;
        size_t offset = index % PageSize;
        return m_pages[pageIndex][offset];
    }

    T* get(const size_t index)
    {
        const size_t pageIndex = index / PageSize;
        if(pageIndex >= m_pages.size()) return nullptr;
        auto& page = m_pages[pageIndex];
        if(page == nullptr) return nullptr;

        size_t offset = index % PageSize;
        return page[offset];
    }

    void insert(const size_t index, const T& val)
    {
        const size_t pageIndex = index / PageSize;
        size_t offset = index % PageSize;

        if(m_pages.size() <= pageIndex) m_pages.resize(pageIndex+1);
        if(!m_pages[pageIndex]) m_pages[pageIndex] = std::make_unique<T[]>(PageSize);

        m_pages[pageIndex][offset] = val;
    }

    void erase(const size_t index)
    {
        // FIXME
        // const size_t pageIndex = index / PageSize;
        // size_t offset = index % pageIndex;
        // m_pages[pageIndex][offset] = nullptr;
    }
private:
    std::vector<std::unique_ptr<T[]>> m_pages;
};
}
