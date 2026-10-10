#pragma once
#include <cstdint>

#include "entity.h"

namespace entis
{
using DestructorFn = void(*)(void*);
using MoveFn = void(*)(void* dst, void* src);
using CopyFn = void(*)(void* dst, const void* src, const EntityMap& );

using ComponentId = uint32_t;
struct ComponentMeta
{
    uint32_t id;
    uint32_t size;
    uint32_t alignment;

    DestructorFn dtor;
    MoveFn moveCtor;
    CopyFn copyCtor;

    bool operator==(const ComponentMeta& other) const
    {
        return id == other.id;
    }
};
}
