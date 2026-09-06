#pragma once
#include <functional>
#include <string>
#include <array>
#include <unordered_map>

#include "archetype.h"
#include "entity.h"
#include "ecsContext.h"

namespace entis
{
class Entity;
template<typename... Components>
class View;
class Registry
{
public:
    [[nodiscard]] Entity create() { return Entity(m_context.createEntity(), *this); }
    void destroy(const Entity e) { m_context.destroyEntity(e.id()); }
    [[nodiscard]] bool valid(const Entity e) const { return m_context.isValid(e.id()); }

    template<typename T>
    T& add(Entity e, const T& comp)
    {
        ComponentId compId = componentId<std::remove_cvref_t<T>>();
        return *static_cast<T*>(m_context.add(e.id(), compId, const_cast<T*>(&comp)));
    }

    template<typename T>
    T& add(Entity e, T&& comp)
    {
        ComponentId compId = componentId<std::remove_cvref_t<T>>();
        return *static_cast<T*>(m_context.add(e.id(), compId, &comp));
    }

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T& emplace(Entity e, Args&&... args)
    {
        // FIXME
        return add(e, T(std::forward<Args>(args)...));
    }

    EcsContext& context() { return m_context; }

    template<typename... Components, typename Fn>
    requires std::invocable<Fn&, Entity, Components&...>
    void each(Fn&& fn)
    {
        for (Archetype* arch : matchingArchetypes<Components...>())
        {
            std::array<PagedColumn*, sizeof...(Components)> columns = getColumns<Components...>(*arch);
            for (size_t row = 0; row < arch->rows(); row++)
            {
                invokeForRow<Components...>(*arch, columns, row, fn, std::index_sequence_for<Components...>{});
            }
        }
    }

    template<typename... Components>
    View<Components...> view();
private:
    template<typename... Components>
    friend class View;

    EcsContext m_context;

    template<typename... Components>
    [[nodiscard]] ArchetypeSignature makeSignature()
    {
        ArchetypeSignature sig;
        (sig.set(componentId<Components>()), ...);
        return sig;
    }

    template<typename... Components>
    Archetype& getArchetype()
    {
        const ArchetypeSignature sig = makeSignature<Components...>();
        return m_context.getOrCreateArchetype(sig);
    }

    template<typename... Components>
    std::vector<Archetype*> matchingArchetypes()
    {
        std::vector<Archetype*> archetypes{};
        const ArchetypeSignature sig = makeSignature<Components...>();
        for(auto& [signature, archetype] : m_context.m_archetypes)
        {
            if((signature & sig) == sig)
                archetypes.push_back(&archetype);
        }

        return archetypes;
    }

    template<typename... Components>
    std::array<PagedColumn*, sizeof...(Components)> getColumns(Archetype& archetype)
    {
        return
        {
            archetype.findColumn(componentId<Components>())...
        };
    }

    template<typename... Components, typename Fn, size_t... Indices>
    requires std::invocable<Fn&, Entity, Components&...>
    void invokeForRow(
        Archetype& archetype,
        const std::array<PagedColumn*, sizeof...(Components)>& columns,
        size_t row,
        Fn& fn,
        std::index_sequence<Indices...>)
    {
        fn(Entity(archetype.entityAt(row), *this), *static_cast<Components*>(columns[Indices]->get(row))...);
    }

    template<typename T>
    ComponentId componentId()
    {
        static ComponentId id = [this]() -> ComponentId
        {
            const auto name = std::string(typeName<T>());

            return m_context.registerComponent(
                name,
                sizeof(T),
                alignof(T),
                [](void* ptr) { std::destroy_at(static_cast<T*>(ptr)); },
            [](void* dst, void* src) { ::new (dst) T(std::move(*static_cast<T*>(src))); }
            );
        }();

        return id;
    }

    template<typename T>
    static constexpr std::string_view typeName()
    {
        std::string_view sig;
        std::string_view prefix;
        std::string_view suffix;

#if defined(__clang__)
        sig = __PRETTY_FUNCTION__;
        prefix = "[T = ";
        suffix = "]";
#elif defined(__GNUC__)
        sig = __PRETTY_FUNCTION__;
        prefix = "with T = ";
        suffix = "]";
#elif defined(_MSC_VER)
        sig = __FUNCSIG__;
        prefix = "entis::Registry::typeName<";
        suffix = ">(void)";
#else
#error "Compiler not supported for compile-time type reflection!"
#endif

        // extract the core template argument string
        size_t start = sig.find(prefix);
        if (start == std::string_view::npos) return "";
        start += prefix.size();

        size_t end = sig.rfind(suffix);
        if (end == std::string_view::npos || end < start) return "";

        std::string_view type_name = sig.substr(start, end - start);

        // remove MSVC's "struct", "class", and "enum" prefixes
        if (type_name.starts_with("struct "))
            type_name.remove_prefix(7);
        else if (type_name.starts_with("class "))
            type_name.remove_prefix(6);
        else if (type_name.starts_with("enum "))
            type_name.remove_prefix(5);

        return type_name;
    }
};

template<typename T>
T& Entity::add(const T &comp)
{
    return m_registry.add(*this, comp);
}

template<typename T>
T& Entity::add(T &&comp)
{
    return m_registry.add(*this, std::forward<T>(comp));
}

template<typename T, typename ... Args> requires std::is_constructible_v<T, Args...>
T& Entity::emplace(Args &&...args)
{
    return m_registry.emplace<T, Args...>(*this, std::forward<Args>(args)...);
}
}
