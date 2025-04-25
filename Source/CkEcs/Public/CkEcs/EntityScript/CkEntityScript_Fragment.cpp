#include "CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_EntityScript_Current::
        FFragment_EntityScript_Current(
            UCk_EntityScript_UE* InScript)
        : _Script(InScript)
    {
    }

    FRequest_EntityScript_Replicate::
        FRequest_EntityScript_Replicate(
            const FCk_Handle& InOwner,
            const FInstancedStruct& InSpawnParams,
            UCk_EntityScript_UE* InScript)
        : _Owner(InOwner)
        , _SpawnParams(InSpawnParams)
        , _Script(InScript)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------