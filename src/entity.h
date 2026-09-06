#pragma once
#include <cstdint>
#include <type_traits>


namespace entis
{
using EntityId = uint64_t;
class Registry;
class Entity
{
public:
    Entity(EntityId id, Registry& registry);

    [[nodiscard]] EntityId id() const { return m_id; }
    [[nodiscard]] Registry& registry() const { return m_registry; }

    template<typename T>
    T& add(const T& comp);

    template<typename T>
    T& add(T&& comp);

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T& emplace(Args&&... args);
private:
    EntityId m_id;
    Registry& m_registry;
};
}
