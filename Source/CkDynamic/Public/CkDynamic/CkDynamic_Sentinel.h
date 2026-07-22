#pragma once

#include <InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::dynamic
{
    // Per-type, default-initialized storage handed back on script error paths. The backing store participates in GC:
    // an invalid Get can therefore never turn a UObject field written through the returned wildcard into the same
    // untraced stale-pointer hazard that dynamic storage admission rejects.
    CKDYNAMIC_API auto
    Get_InvalidSentinel_FragmentData(
        const UScriptStruct* InStructType) -> FInstancedStruct&;
}
