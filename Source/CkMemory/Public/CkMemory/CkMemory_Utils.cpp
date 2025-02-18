#include "CkMemory_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Memory_UE::
    Get_MemoryCountSnapshot(
        const UObject* InContext)
    -> FCk_Utils_Memory_MemoryCountSnapshot_Result
{
#if STATS
    static auto StatsSubsystem = GEngine->GetEngineSubsystem<UCk_Stats_Subsystem_UE>();

    if (ck::Is_NOT_Valid(StatsSubsystem))
    { return {}; }

    return StatsSubsystem->Get_MemoryCountSnapshot(InContext);
#else
    return {};
#endif
}

// --------------------------------------------------------------------------------------------------------------------
