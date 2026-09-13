#pragma once
#include <utility>

#include "../registry.h"

namespace entis
{
template<typename... Components>
class View
{
public:
    View(Registry& registry, std::vector<Archetype*> archetypes)
        : m_archetypes(std::move(archetypes)), m_registry(registry)
    {

    }

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::tuple<Entity, Components&...>;
        using pointer           = value_type*;
        using reference         = value_type&;

        explicit Iterator(View& view, const size_t archetypeIndex)
            : m_registry(view.m_registry), m_view(view), m_archetypeIndex(archetypeIndex)
        {
            if (m_archetypeIndex < m_view.m_archetypes.size())
            {
                loadArchetype();
                loadPage();
            }
        }

        value_type operator*() const
        {
            auto archetype = m_view.m_archetypes[m_archetypeIndex];
            size_t row = m_pageIndex * Archetype::kElementsPerPage + m_pageRow;
            return createTuple(Entity(m_entityPage[m_pageRow], m_registry), m_pages, m_pageRow, std::index_sequence_for<Components...>{});
        }

        Iterator& operator++()
        {
            m_pageRow++;

            if (m_pageRow < m_pageSize)
                return *this;

            ++m_pageIndex;

            if (m_pageIndex < m_archetype->pages())
            {
                m_pageRow = 0;
                loadPage();
                return *this;
            }

            ++m_archetypeIndex;

            if (m_archetypeIndex >= m_view.m_archetypes.size())
            {
                m_archetype = nullptr;
                m_pageRow = 0;
                m_pageIndex = 0;
                return *this;
            }

            loadArchetype();
            loadPage();

            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator== (const Iterator& a, const Iterator& b)
        {
            return &a.m_view == &b.m_view && a.m_pageRow == b.m_pageRow && a.m_pageIndex == b.m_pageIndex && a.m_archetypeIndex == b.m_archetypeIndex;
        }

        friend bool operator!= (const Iterator& a, const Iterator& b)
        {
            return !(a == b);
        }
    private:
        Registry& m_registry;
        View& m_view;
        Archetype* m_archetype{};

        size_t m_archetypeIndex = 0;
        size_t m_pageRow = 0;
        size_t m_pageIndex = 0;

        size_t m_pageSize = 0;

        std::array<PagedColumn*, sizeof...(Components)> m_columns;
        std::tuple<Components*...> m_pages{};
        EntityId* m_entityPage{};

        void loadArchetype()
        {
            m_pageRow = 0;
            m_pageIndex = 0;

            while (m_archetypeIndex < m_view.m_archetypes.size())
            {
                m_archetype = m_view.m_archetypes[m_archetypeIndex];

                if (m_archetype->rows() != 0)
                {
                    m_columns = m_registry.getColumns<Components...>(*m_archetype);
                    return;
                }

                ++m_archetypeIndex;
            }

            m_archetype = nullptr;
        }

        void loadPage()
        {
            constexpr size_t pageCapacity = Archetype::kElementsPerPage;

            m_pageSize = std::min(
                pageCapacity,
                m_archetype->rows() - m_pageIndex * pageCapacity
            );

            loadPageComponents(std::index_sequence_for<Components...>{});

            m_entityPage = m_archetype->rowEntities().pageData(m_pageIndex);
        }

        template<size_t... Indices>
        void loadPageComponents(std::index_sequence<Indices...>)
        {
            m_pages = std::tuple<Components*...>(static_cast<Components*>(m_columns[Indices]->pageData(m_pageIndex))...);
        }

        template<size_t... Indices>
        static std::tuple<Entity, Components&...> createTuple(
            Entity entity,
            const std::tuple<Components*...>& pages,
            size_t pageRow,
            std::index_sequence<Indices...>)
        {
            return std::tuple<Entity, Components&...>(entity, std::get<Indices>(pages)[pageRow]...);
        }
    };

    Iterator begin()
    {
        return Iterator(*this, 0);
    }

    Iterator end()
    {
        return Iterator(*this, m_archetypes.size());
    }
private:
    std::vector<Archetype*> m_archetypes;
    Registry& m_registry;
};

template<typename ... Components>
View<Components...> Registry::view()
{
    return View<Components...>(*this, matchingArchetypes<Components...>());
}
}
