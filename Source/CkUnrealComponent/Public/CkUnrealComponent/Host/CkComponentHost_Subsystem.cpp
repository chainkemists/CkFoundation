#include "CkComponentHost_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ComponentHost_Subsystem_UE::
    Get(
        UWorld* InWorld)
    -> UCk_ComponentHost_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InWorld))
    { return nullptr; }

    return InWorld->GetSubsystem<UCk_ComponentHost_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------
