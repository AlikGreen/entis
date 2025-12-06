#include "viewBase.h"

#include "registry.h"

namespace Neon::ECS
{
    ViewBase::ViewBase(Registry *registry) : registry(registry)
    {
        version = registry->version;
    }
}