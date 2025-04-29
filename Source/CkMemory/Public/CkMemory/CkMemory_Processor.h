#pragma once

#include "Allocator/CkMemoryAllocator.h"

#if CK_ENABLE_MEMORY_TRACKING

#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKMEMORY_API FProcessor_Memory_Stats
    {
    public:
        auto Tick(FCk_Time InDeltaT) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#endif