#pragma once
#include <bitset>
#include <memory>
#include <type_traits>
#include <vector>

namespace entis
{
template<typename T, size_t PageSize>
class SparsePagedArray
{
    struct Page
    {
        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

        Storage storage[PageSize];
        std::bitset<PageSize> occupied;

        T* ptr(const size_t index)
        {
            return std::launder(reinterpret_cast<T*>(&storage[index]));
        }

        const T* ptr(const size_t index) const
        {
            return std::launder(reinterpret_cast<const T*>(&storage[index]));
        }
    };

public:
    SparsePagedArray() = default;

    SparsePagedArray(const SparsePagedArray&) = delete;
    SparsePagedArray& operator=(const SparsePagedArray&) = delete;

    SparsePagedArray(SparsePagedArray&&) noexcept = default;
    SparsePagedArray& operator=(SparsePagedArray&&) noexcept = default;

    ~SparsePagedArray()
    {
        clear();
    }

    T* get(const size_t index)
    {
        const size_t pageIndex = index / PageSize;
        const size_t offset = index % PageSize;

        if (pageIndex >= m_pages.size())
            return nullptr;

        auto& page = m_pages[pageIndex];

        if (!page || !page->occupied[offset])
            return nullptr;

        return page->ptr(offset);
    }

    const T* get(const size_t index) const
    {
        const size_t pageIndex = index / PageSize;
        const size_t offset = index % PageSize;

        if (pageIndex >= m_pages.size())
            return nullptr;

        const auto& page = m_pages[pageIndex];

        if (!page || !page->occupied[offset])
            return nullptr;

        return page->ptr(offset);
    }

    void set(const size_t index, const T& value)
    {
        const size_t pageIndex = index / PageSize;
        const size_t offset = index % PageSize;

        ensurePage(pageIndex);

        Page& page = *m_pages[pageIndex];

        if (page.occupied[offset])
        {
            *page.ptr(offset) = value;
        }
        else
        {
            std::construct_at(page.ptr(offset), value);
            page.occupied[offset] = true;
        }
    }

    void set(const size_t index, T&& value)
    {
        const size_t pageIndex = index / PageSize;
        const size_t offset = index % PageSize;

        ensurePage(pageIndex);

        Page& page = *m_pages[pageIndex];

        if (page.occupied[offset])
        {
            *page.ptr(offset) = std::move(value);
        }
        else
        {
            std::construct_at(page.ptr(offset), std::move(value));
            page.occupied[offset] = true;
        }
    }

    void erase(const size_t index)
    {
        const size_t pageIndex = index / PageSize;
        const size_t offset = index % PageSize;

        if (pageIndex >= m_pages.size())
            return;

        auto& page = m_pages[pageIndex];

        if (!page || !page->occupied[offset])
            return;

        std::destroy_at(page->ptr(offset));
        page->occupied[offset] = false;
    }

    void clear()
    {
        for (auto& page : m_pages)
        {
            if (!page)
                continue;

            for (size_t i = 0; i < PageSize; ++i)
            {
                if (page->occupied[i])
                {
                    std::destroy_at(page->ptr(i));
                    page->occupied[i] = false;
                }
            }
        }

        m_pages.clear();
    }

private:
    void ensurePage(const size_t pageIndex)
    {
        if (pageIndex >= m_pages.size())
            m_pages.resize(pageIndex + 1);

        if (!m_pages[pageIndex])
            m_pages[pageIndex] = std::make_unique<Page>();
    }

    std::vector<std::unique_ptr<Page>> m_pages;
};
}
