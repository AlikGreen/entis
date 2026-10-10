#include "registry.h"

namespace entis
{
    void Registry::merge(const Registry &registry)
    {
        m_context.merge(registry.context());
    }
}
