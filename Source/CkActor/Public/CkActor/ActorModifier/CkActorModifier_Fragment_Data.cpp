#include "CkActorModifier_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_SpawnActor_PostSpawn_Params::
    FCk_SpawnActor_PostSpawn_Params(
        ECk_SpawnActor_PostSpawnPolicy InPostSpawnPolicy,
        FCk_Actor_AttachToActor_Params InParams)
    : _PostSpawnPolicy(InPostSpawnPolicy)
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

FCk_Request_ActorModifier_SpawnActor::
    FCk_Request_ActorModifier_SpawnActor(
        FCk_Utils_Actor_SpawnActor_Params InSpawnParams,
        FCk_SpawnActor_PostSpawn_Params InPostSpawnParams,
        PreFinishSpawnFuncType InPreFinishSpawnFunc)
    : _SpawnParams(InSpawnParams)
    , _PostSpawnParams(InPostSpawnParams)
    , _PreFinishSpawnFunc(InPreFinishSpawnFunc)
{
}

// --------------------------------------------------------------------------------------------------------------------
