#pragma once

#ifndef CK_ENABLE_MEMORY_TRACKING
// Track allocations when stats are available to display it.
#define CK_ENABLE_MEMORY_TRACKING STATS
#endif

#if CK_ENABLE_MEMORY_TRACKING

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // TODO Add custom Memory Tracking Allocator
}
#endif

// --------------------------------------------------------------------------------------------------------------------
