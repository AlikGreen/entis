#include "ecsContext.h"

namespace entis {
    ComponentId EcsContext::registerComponent(
        const std::string &name,
        const uint32_t size,
        const uint32_t alignment,
        const DestructorFn dtor,
        const MoveConstructorFn moveCtor)
    {
        if (const auto it = m_componentNameMap.find(name); it != m_componentNameMap.end())
            return it->second;

        ComponentMeta meta{};
        meta.id = m_components.size();
        meta.size = size;
        meta.alignment = alignment;
        meta.dtor = dtor;
        meta.moveCtor = moveCtor;

        m_components.push_back(meta);

        m_componentNameMap[name] = meta.id;

        return meta.id;
    }

    ComponentId EcsContext::componentId(const std::string &name)
    {
        if (const auto it = m_componentNameMap.find(name); it != m_componentNameMap.end())
            return it->second;

        return std::numeric_limits<uint32_t>::max();
    }

    bool EcsContext::isValid(const EntityId id) const
    {
        if(id == std::numeric_limits<uint32_t>::max() || id >= m_nextEntity) return false;
        return true;
    }

    EntityId EcsContext::createEntity() 
    {
        const EntityId id = m_nextEntity++;
        m_entitySignatures.emplace_back();
        return id;
    }

    void EcsContext::destroyEntity(const EntityId id)
    {
        const ArchetypeSignature signature = m_entitySignatures.at(id);
        Archetype* archetype = getArchetype(signature);
        if(!archetype) return;
        archetype->remove(id);
    }

    void* EcsContext::add(EntityId entityId, ComponentId componentId, void* data)
    {
        const ArchetypeSignature oldSignature = m_entitySignatures.at(entityId);
        if(oldSignature.test(componentId)) return nullptr; // don't add if already exists. maybe should replace instead

        ArchetypeSignature newSignature = oldSignature;
        newSignature.set(componentId);
        m_entitySignatures[entityId] = newSignature;

        Archetype& newArchetype = getOrCreateArchetype(newSignature);

        newArchetype.addEntity(entityId);

        if(oldSignature.any())
        {
            if(Archetype* oldArchetype = getArchetype(oldSignature))
                oldArchetype->moveComponents(entityId, newArchetype);
        }

        return newArchetype.set(entityId, componentId, data);
    }

    bool EcsContext::has(const EntityId entityId, const ComponentId componentId) const
    {
        const ArchetypeSignature sig = m_entitySignatures.at(entityId);
        return sig.test(componentId);
    }

    bool EcsContext::remove(const EntityId entityId, const ComponentId componentId)
    {
        const ArchetypeSignature oldSignature = m_entitySignatures.at(entityId);
        if(!oldSignature.test(componentId)) return false;

        ArchetypeSignature newSignature = oldSignature;
        newSignature.reset(componentId);
        m_entitySignatures[entityId] = newSignature;

        Archetype& newArchetype = getOrCreateArchetype(newSignature);

        newArchetype.addEntity(entityId);

        if(oldSignature.any())
        {
            if(Archetype* oldArchetype = getArchetype(oldSignature))
                oldArchetype->moveComponents(entityId, newArchetype);
        }

        return true;
    }

    Archetype* EcsContext::getArchetype(const ArchetypeSignature& signature)
    {
        if(const auto it = m_archetypes.find(signature); it != m_archetypes.end())
            return &it->second;

        return nullptr;
    }

    Archetype& EcsContext::getOrCreateArchetype(const ArchetypeSignature& signature)
    {
        if(const auto it = m_archetypes.find(signature); it != m_archetypes.end())
            return it->second;

        StaticVector<ComponentMeta, 8> componentMetas;

        // FIXME
        for(size_t i = 0; i < kMaxComponents; i++)
        {
            if(signature.test(i))
            {
                componentMetas.add(m_components[i]);
            }
        }

        Archetype arch{componentMetas};

        auto [archetype, _] = m_archetypes.emplace(signature, std::move(arch));
        return archetype->second;
    }
}
