#include "CkActorRebind_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorRebind_UE::
    Request_RebindActor(
        FCk_Handle& InEntity,
        AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity) && ck::IsValid(InActor, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Request_RebindActor: invalid entity [{}] or actor [{}]"), InEntity, InActor)
    { return; }

    // Reverse link (actor -> entity component) + forward fragment (entity -> actor). Mirrors WithActor::Construct.
    // Order matters: OwningActor MUST be added before Transform so Transform auto-detects the actor's root.
    UCk_Utils_OwningActor_UE::SetupActorEntityLink(InEntity, InActor);
    UCk_Utils_OwningActor_UE::Add(InEntity, InActor);

    // Re-bind the actor's root component to the entity's Transform. Transform::Add routes (OwningActor is now
    // present) to AddAndAttachToUnrealComponent, which binds the root + (via its own Has<FFragment_Transform> check)
    // PRESERVES the restored world-transform value while establishing the transform-sync binding. So the entity's
    // saved position is kept; if the entity has no Transform (edge), the actor's spawn transform seeds it.
    if (ck::IsValid(InActor->GetRootComponent()))
    {
        UCk_Utils_Transform_UE::Add(InEntity, InActor->GetActorTransform());
    }

    // Signal game code that actor-side wiring needs re-establishing (see the tag's declaration for the contract).
    InEntity.AddOrGet<ck::FTag_ActorJustRebound>();

    ck::ecs::Display(TEXT("[M2b] Request_RebindActor: re-bridged Actor=[{}] to restored Entity=[{}]"),
        InActor->GetName(), InEntity);
}

// --------------------------------------------------------------------------------------------------------------------
