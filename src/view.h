#pragma once
#include <optional>
#include "storage.h"

namespace Neon::ECS
{
template<typename... Components>
class View
{
public:
    explicit View(Storage<Components>&... storages) : storages(storages...)
    {
        if constexpr (sizeof...(Components) == 0)
            return;

        auto& firstStorage = std::get<0>(this->storages); // ideally smallest
        const size_t maxSize = firstStorage.size();

        if constexpr (sizeof...(Components) == 1)
        {
            indices.resize(maxSize);
            for (size_t i = 0; i < maxSize; ++i)
                indices[i] = std::make_tuple(i);

            entities = firstStorage.getDenseEntities();
            return;
        }

        for (size_t i = 0; i < maxSize; ++i)
        {
            size_t entityId = firstStorage.entityAt(i);

            auto indexTuple = findIndicesForEntity(entityId, std::index_sequence_for<Components...>{});

            if (indexTuple.has_value())
            {
                indices.push_back(indexTuple.value());
                entities.emplace_back(entityId);
            }
        }
    }

    [[nodiscard]] size_t size() const
    {
        return entities.size();
    }

    std::tuple<size_t, Components&...> operator[](const size_t index)
    {
        return at(index);
    }

    class Iterator
    {
    public:
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType = std::ptrdiff_t;
        using ValueType = std::tuple<size_t, Components&...>;
        using PointerType = ValueType*;
        using ReferenceType = ValueType;

        Iterator(View const* view, const size_t index) : view(view), index(index) {  }

        ReferenceType operator*() const
        {
            return view->at(index);
        }

        Iterator& operator++()
        {
            ++index;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(Iterator const& other) const
        {
            return index == other.index;
        }

        bool operator!=(Iterator const& other) const
        {
            return !(*this == other);
        }
    private:
        View const* view;
        size_t index;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, indices.size()); }

    std::tuple<size_t, Components&...> at(size_t viewIndex) const
    {
        return getAllImpl(viewIndex, std::index_sequence_for<Components...>{});
    }
private:
    std::tuple<Storage<Components>&...> storages;
    using IndexTuple = std::tuple<std::conditional_t<true, size_t, Components>...>;
    std::vector<IndexTuple> indices{};
    std::vector<EntityID> entities{};

    template<size_t... Is>
    std::optional<IndexTuple> findIndicesForEntity(size_t entityId, std::index_sequence<Is...>)
    {
        std::array<size_t, sizeof...(Components)> storageIndices = {
            std::get<Is>(storages).indexOf(entityId)...
        };

        // Check if any index is INVALID
        constexpr size_t INVALID = std::numeric_limits<size_t>::max();
        bool existsInAll = ((storageIndices[Is] != INVALID) && ...);

        if (!existsInAll)
            return std::nullopt;

        return std::make_tuple(storageIndices[Is]...);
    }

    template<size_t... Is>
    std::tuple<size_t, Components&...> getAllImpl(size_t viewIndex, std::index_sequence<Is...>) const
    {
        auto const& indexTuple = indices[viewIndex];

        size_t entityId = entities[viewIndex];

        return std::tuple<size_t, Components&...>{
            entityId,
            const_cast<Components&>(std::get<Is>(storages).getByIndex(std::get<Is>(indexTuple)))...
        };
    }
};
}
