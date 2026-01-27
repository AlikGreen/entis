#pragma once
#include <optional>


#include "storage.h"
#include "entity.h"
#include "viewBase.h"

namespace Neon::ECS
{
    class Entity;
    class Registry;


template<typename... Components>
class View final : public ViewBase
{
public:
    View(const View&) = delete;
    View& operator=(const View&) = delete;

    [[nodiscard]] size_t size() const
    {
        return entities.size();
    }

    std::tuple<Entity, Components&...> operator[](const size_t index)
    {
        return at(index);
    }

    class Iterator
    {
    public:
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType = std::ptrdiff_t;
        using ValueType = std::tuple<Entity, Components&...>;
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

    std::tuple<Entity, Components&...> at(size_t viewIndex) const
    {
        return getAllImpl(viewIndex, std::index_sequence_for<Components...>{});
    }
private:
    friend class Registry;

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
    std::tuple<Entity, Components&...> getAllImpl(size_t viewIndex, std::index_sequence<Is...>) const
    {
        auto const& indexTuple = indices[viewIndex];

        const size_t entityId = entities[viewIndex];

        return std::tuple<Entity, Components&...>
        {
            Entity(registry, entityId),
            const_cast<Components&>(std::get<Is>(storages).getByIndex(std::get<Is>(indexTuple)))...
        };
    }

    template<size_t I>
    [[nodiscard]] size_t storageSize() const
    {
        return std::get<I>(storages).size();
    }

    template<size_t... Is>
    size_t findSmallestStorageIndex(std::index_sequence<Is...>) const
    {
        size_t smallestIndex = 0;
        size_t smallestSize = std::numeric_limits<size_t>::max();

        auto consider = [&](const size_t i, const size_t s)
        {
            if (s < smallestSize)
            {
                smallestSize = s;
                smallestIndex = i;
            }
        };

        (consider(Is, storageSize<Is>()), ...);

        return smallestIndex;
    }

    template<size_t I>
    void buildFromStorage()
    {
        auto &firstStorage = std::get<I>(storages);
        const size_t maxSize = firstStorage.size();

        indices.reserve(maxSize);
        entities.reserve(maxSize);

        for (size_t i = 0; i < maxSize; ++i)
        {
            const size_t entityId = firstStorage.entityAt(i);

            auto indexTuple = findIndicesForEntity(entityId, std::index_sequence_for<Components...>{});

            if (indexTuple.has_value())
            {
                indices.push_back(*indexTuple);
                entities.emplace_back(entityId);
            }
        }
    }

    template<size_t I = 0>
    void buildFromSmallest(const size_t smallestIndex)
    {
        if constexpr (I < sizeof...(Components))
        {
            if (I == smallestIndex)
            {
                buildFromStorage<I>();
            }
            else
            {
                buildFromSmallest<I + 1>(smallestIndex);
            }
        }
    }

    explicit View(Registry* registry, Storage<Components>&... storages)
        : ViewBase(registry), storages(storages...)
    {
        if constexpr (sizeof...(Components) == 0)
            return;

        if constexpr (sizeof...(Components) == 1)
        {
            auto &firstStorage = std::get<0>(this->storages);
            const size_t maxSize = firstStorage.size();

            indices.resize(maxSize);
            for (size_t i = 0; i < maxSize; ++i)
                indices[i] = std::make_tuple(i);

            entities = firstStorage.getDenseEntities();
            return;
        }

        const size_t smallestIndex = findSmallestStorageIndex(std::index_sequence_for<Components...>{});
        buildFromSmallest(smallestIndex);
    }
};

template<typename... Components>
const View<Components...>& Registry::view()
{
    using ViewType = View<Components...>;

    const size_t type = typeid(View<Components...>).hash_code();

    const auto it = viewCache.find(type);
    if (it != viewCache.end())
    {
        if (it->second->version == version)
            return *static_cast<ViewType*>(it->second.get());
    }

    auto newView = Box<ViewType>(new ViewType(this, storage<Components>()...));
    ViewType *viewPtr = newView.get();
    viewCache[type] = std::move(newView);

    return *viewPtr;
}
}
