#include "CkCue_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Cue_Execute::
    FCk_Request_Cue_Execute(
        const FGameplayTag& InCueName,
        const FInstancedStruct& InSpawnParams)
    : _CueName(InCueName)
    , _SpawnParams(InSpawnParams)
{
}

// --------------------------------------------------------------------------------------------------------------------
