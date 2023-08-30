#include "CkActorModifier_Fragment_Params.h"

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
        bool InIsTickEnabled,
        FCk_Time InTickInterval,
        ECk_ActorComponent_AttachmentPolicy InAttachmentType,
        USceneComponent* InParent,
        FName InAttachmentSocket)
    : _IsTickEnabled(InIsTickEnabled)
    , _TickInterval(InTickInterval)
    , _AttachmentType(InAttachmentType)
    , _Parent(InParent)
    , _AttachmentSocket(InAttachmentSocket)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_ActorModifier_AttachActor::
    FCk_Request_ActorModifier_AttachActor(
        FCk_Actor_AttachToActor_Params InParams)
    : _Params(InParams)
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

FCk_Request_ActorModifier_AddActorComponent::
    FCk_Request_ActorModifier_AddActorComponent(
        TSubclassOf<UActorComponent> InComponentToAdd,
        bool InIsUnique,
        FCk_AddActorComponent_Params InComponentParams,
        InitializerFuncType InInitializerFunc)
    : _ComponentToAdd(InComponentToAdd)
    , _IsUnique(InIsUnique)
    , _ComponentParams(InComponentParams)
    , _InitializerFunc(InInitializerFunc)
{
}

// --------------------------------------------------------------------------------------------------------------------
