#include "viewBase.h"

#include "registry.h"

namespace entis
{
    ViewBase::ViewBase(Registry *registry) : registry(registry)
    {
        version = registry->version;
    }
}