#include "CkEntityScript_WithActor.h"

#include "CkEcs/CkEcsLog.h"

#include <Engine/World.h>
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_WithActor_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    // _OwningActor is injected by TryInjectEntityScriptSpawnParams before Construct is called

    CK_ENSURE_IF_NOT(ck::IsValid(_OwningActor),
        TEXT("EntityScript_WithActor [{}] has no valid OwningActor. "
             "Did you pass FCk_EntityScript_WithActor_SpawnParams with the Actor?"),
        GetClass()->GetName())
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    ck::ecs::Display(TEXT("[REP_DEBUG] WithActor::Construct Actor=[{}] ActorReplicates=[{}] ScriptReplication=[{}]"),
        _OwningActor->GetName(),
        _OwningActor->GetIsReplicated(),
        Get_Replication());

    if (NOT _OwningActor->GetIsReplicated())
    {
        _Replication = ECk_Replication::DoesNotReplicate;
        ck::ecs::Display(TEXT("[REP_DEBUG] WithActor::Construct — Actor does NOT replicate, forced DoesNotReplicate"));
    }

    // Self-owned entities don't inherit World from the ownership chain.
    // Add it directly so Get_WorldForEntity can resolve without recursion.
    InHandle.AddOrGet<TWeakObjectPtr<UWorld>>(_OwningActor->GetWorld());

    // Order matters: OwningActor MUST be added before Transform so that Transform
    // auto-detects the actor's root component and attaches to it
    UCk_Utils_OwningActor_UE::Add(InHandle, _OwningActor);

    if (ck::IsValid(_OwningActor->GetRootComponent()))
    {
        UCk_Utils_Transform_UE::Add(InHandle, _OwningActor->GetActorTransform(), Get_Replication());
    }

    // Set up the reverse link: Actor -> Entity
    UCk_Utils_OwningActor_UE::SetupActorEntityLink(InHandle, _OwningActor);

    UCk_Utils_GameplayLabel_UE::Add(InHandle, {});
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *_OwningActor->GetName());

    return ConstructWithActor(InHandle, _OwningActor);
}

auto
    UCk_EntityScript_WithActor_UE::
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor)
    -> ECk_EntityScript_ConstructionFlow
{
    return DoConstructWithActor(InHandle, InOwningActor);
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
    ShowReplicationInEditor() const
    -> bool
{
    return false;
}

// --------------------------------------------------------------------------------------------------------------------
