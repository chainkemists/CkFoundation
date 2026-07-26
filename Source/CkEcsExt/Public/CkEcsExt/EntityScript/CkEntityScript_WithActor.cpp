#include "CkEntityScript_WithActor.h"
#include "CkEntityScript_WithActor_Data.h"

#include "CkEcs/CkEcsLog.h"

#include <Engine/World.h>
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_WithActor_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    CK_ENSURE_IF_NOT(ck::IsValid(_OwningActor),
        TEXT("EntityScript_WithActor [{}] has no valid OwningActor. "
             "Did you pass FCk_EntityScript_WithActor_SpawnParams with the Actor?"),
        GetClass()->GetName())
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    ck::ecs::Display(TEXT("[REP_DEBUG] WithActor::Construct Actor=[{}] ActorReplicates=[{}] EffectiveReplication=[{}]"),
        _OwningActor->GetName(),
        _OwningActor->GetIsReplicated(),
        Get_EffectiveReplication());

    // Self-owned entities don't inherit World from the ownership chain.
    // Add it directly so Get_WorldForEntity can resolve without recursion.
    InHandle.AddOrGet<TWeakObjectPtr<UWorld>>(_OwningActor->GetWorld());

    UCk_Utils_OwningActor_UE::SetupActorEntityLink(InHandle, _OwningActor);

    // Order matters: OwningActor MUST be added before Transform so that Transform
    // auto-detects the actor's root component and attaches to it
    UCk_Utils_OwningActor_UE::Add(InHandle, _OwningActor);

    if (ck::IsValid(_OwningActor->GetRootComponent()))
    {
        UCk_Utils_Transform_UE::Add(InHandle, _OwningActor->GetActorTransform(), Get_EffectiveReplication());
    }

    UCk_Utils_GameplayLabel_UE::Add(InHandle, {});
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *_OwningActor->GetName());

    if (Get_IsSnapshotRespawnable())
    {
        InHandle.AddOrGet<FFragment_ActorSpawnIntent>(
            FFragment_ActorSpawnIntent{_OwningActor->GetClass()->GetPathName()});
    }

    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_EntityScript_WithActor_UE::
    EndPlay()
    -> void
{
    Super::EndPlay();
}

auto
    UCk_EntityScript_WithActor_UE::
    DoGet_OwningActor() const
    -> AActor*
{
    return _OwningActor;
}

auto
    UCk_EntityScript_WithActor_UE::
    Get_AreSpawnParamsMatching(
        const FInstancedStruct& InClientSpawnParams,
        const UCk_EntityScript_UE* InConstructedScript) const
    -> bool
{
    const auto* WithActorScript =
        Cast<UCk_EntityScript_WithActor_UE>(InConstructedScript);

    if (ck::Is_NOT_Valid(WithActorScript, ck::IsValid_Policy_NullptrOnly{}))
    { return Super::Get_AreSpawnParamsMatching(InClientSpawnParams, InConstructedScript); }

    const auto* SpawnParams =
        InClientSpawnParams.GetPtr<FCk_EntityScript_WithActor_SpawnParams>();

    if (ck::Is_NOT_Valid(SpawnParams, ck::IsValid_Policy_NullptrOnly{}))
    { return Super::Get_AreSpawnParamsMatching(InClientSpawnParams, InConstructedScript); }

    return SpawnParams->Get_OwningActor().Get() == WithActorScript->Get_OwningActor();
}

auto
    UCk_EntityScript_WithActor_UE::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    // _OwningActor is injected from spawn params before Construct runs. On the CDO it is
    // null, in which case we fall through to the script's declared replication — callers
    // that need the runtime-aware answer must query the instance after spawn-param injection.
    if (ck::IsValid(_OwningActor) && NOT _OwningActor->GetIsReplicated())
    { return ECk_Replication::DoesNotReplicate; }

    return Super::Get_EffectiveReplication();
}

auto
    UCk_EntityScript_WithActor_UE::
    ShowReplicationInEditor() const
    -> bool
{
    return false;
}

// --------------------------------------------------------------------------------------------------------------------
