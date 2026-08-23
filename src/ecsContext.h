#pragma once
#include <string>
#include <unordered_map>

#include "archetype.h"
#include "componentMeta.h"
#include "entity.h"


namespace entis
{

class EcsContext
{
public:
    ComponentId registerComponent(
        const std::string& name,
        uint32_t size,
        uint32_t alignment,
        DestructorFn dtor = nullptr,
        MoveConstructorFn moveCtor = nullptr);

    ComponentId componentId(const std::string& name);

    [[nodiscard]] bool isValid(EntityId id) const;

    EntityId createEntity();
    void destroyEntity(EntityId id);

    void* add(EntityId entityId, ComponentId componentId, void* data);
private:
    friend class Registry;

    EntityId m_nextEntity = 0;

    std::unordered_map<ArchetypeSignature, Archetype> m_archetypes;

    Archetype* getArchetype(const ArchetypeSignature& signature);
    Archetype& getOrCreateArchetype(const ArchetypeSignature &signature);

    std::unordered_map<std::string, ComponentId> m_componentNameMap{};
    std::vector<ArchetypeSignature> m_entitySignatures;
    std::vector<ComponentMeta> m_components;
};
}
