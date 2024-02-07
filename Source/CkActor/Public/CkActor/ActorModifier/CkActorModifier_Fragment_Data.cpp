#include "CkActorModifier_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_SpawnActor_PostSpawn_Params::
    FCk_SpawnActor_PostSpawn_Params(
        FCk_Actor_AttachToActor_Params InParams)
    : _PostSpawnPolicy(ECk_SpawnActor_PostSpawnPolicy::AttachImmediately)
    , _Params(InParams)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_AddActorComponent_Params::
    FCk_AddActorComponent_Params(
        USceneComponent* InParent)
    : _Parent(InParent)
{
}

// --------------------------------------------------------------------------------------------------------------------
