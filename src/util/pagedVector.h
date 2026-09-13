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

    [[nodiscard]] size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_size == 0;
    }

    [[nodiscard]] size_t pageCount() const noexcept
    {
        return m_pages.size();
    }

    T& operator[](const size_t index)
    {
        assert(index < m_size);
        return m_pages[index / PageSize][index % PageSize];
    }

    const T& operator[](const size_t index) const
    {
        assert(index < m_size);
        return m_pages[index / PageSize][index % PageSize];
    }

    T& back()
    {
        assert(m_size > 0);
        return (*this)[m_size - 1];
    }

    const T& back() const
    {
        assert(m_size > 0);
        return (*this)[m_size - 1];
    }

    T* pageData(const size_t pageIndex)
    {
        assert(pageIndex < m_pages.size());
        return m_pages[pageIndex].get();
    }

    const T* pageData(const size_t pageIndex) const
    {
        assert(pageIndex < m_pages.size());
        return m_pages[pageIndex].get();
    }

    void push_back(const T& value)
    {
        ensurePageFor(m_size);

        const size_t pageIndex = m_size / PageSize;
        const size_t offset = m_size % PageSize;

        m_pages[pageIndex][offset] = value;
        ++m_size;
    }

    void push_back(T&& value)
    {
        ensurePageFor(m_size);

        const size_t pageIndex = m_size / PageSize;
        const size_t offset = m_size % PageSize;

        m_pages[pageIndex][offset] = std::move(value);
        ++m_size;
    }

    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
        ensurePageFor(m_size);

        const size_t pageIndex = m_size / PageSize;
        const size_t offset = m_size % PageSize;

        T& element = m_pages[pageIndex][offset];
        element = T(std::forward<Args>(args)...);

        ++m_size;

        return element;
    }

    void pop_back()
    {
        assert(m_size > 0);

        --m_size;

        const size_t pageIndex = m_size / PageSize;
        const size_t offset = m_size % PageSize;

        m_pages[pageIndex][offset] = T{};
    }

    void clear()
    {
        m_size = 0;
    }

private:
    void ensurePageFor(const size_t index)
    {
        const size_t pageIndex = index / PageSize;

        if (pageIndex >= m_pages.size())
        {
            m_pages.push_back(
                std::make_unique<T[]>(PageSize)
            );
        }
    }

    std::vector<std::unique_ptr<T[]>> m_pages;
    size_t m_size = 0;
};
}
