#pragma once

namespace entis
{
using DestructorFn = void(*)(void*);
using MoveConstructorFn = void(*)(void* dst, void* src);

using ComponentId = uint32_t;
struct ComponentMeta
{
    uint32_t id;
    uint32_t size;
    uint32_t alignment;

    DestructorFn dtor;
    MoveConstructorFn moveCtor;

    bool operator==(const ComponentMeta& other) const
    {
        return id == other.id;
    }
};
}
