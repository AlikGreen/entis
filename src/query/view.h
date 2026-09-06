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
            : m_view(view), m_registry(view.m_registry), m_archetypeIndex(archetypeIndex)
        {
            advanceToValidArchetype();
        }

        value_type operator*() const
        {
            auto archetype = m_view.m_archetypes[m_archetypeIndex];
            return createTuple(Entity(archetype->entityAt(m_row), m_registry), m_columns, m_row, std::index_sequence_for<Components...>{});
        }

        Iterator& operator++()
        {
            m_row++;

            if(m_row >= m_archetype->rows())
            {
                ++m_archetypeIndex;
                m_row = 0;
                advanceToValidArchetype();
            }

            return *this;;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator== (const Iterator& a, const Iterator& b)
        {
            return &a.m_view == &b.m_view && a.m_row == b.m_row && a.m_archetypeIndex == b.m_archetypeIndex;
        }

        friend bool operator!= (const Iterator& a, const Iterator& b)
        {
            return !(a == b);
        }
    private:
        Registry& m_registry;
        View& m_view;
        size_t m_archetypeIndex = 0;
        Archetype* m_archetype{};
        size_t m_row = 0;
        std::array<PagedColumn*, sizeof...(Components)> m_columns;

        void advanceToValidArchetype()
        {
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
            m_row = 0;
        }

        template<size_t... Indices>
        static std::tuple<Entity, Components&...> createTuple(
            Entity entity,
            const std::array<PagedColumn*, sizeof...(Components)>& columns,
            size_t row,
            std::index_sequence<Indices...>)
        {
            return std::tuple<Entity, Components&...>(entity, *static_cast<Components*>(columns[Indices]->get(row))...);
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
