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

    // The Transform fragment is non-snapshotable, so it is ABSENT post-restore — re-create it bound to the fresh
    // actor's root (re-establishing the transform-sync bridge). NOTE (M2b-1 limitation): the entity's saved WORLD
    // POSITION does not round-trip yet (FFragment_Transform is not snapshotable), so the actor is positioned at its
    // current/spawn transform. Position-restore is a follow-up (mark FFragment_Transform snapshotable).
    if (ck::IsValid(InActor->GetRootComponent()) && NOT UCk_Utils_Transform_UE::Has(InEntity))
    {
        UCk_Utils_Transform_UE::Add(InEntity, InActor->GetActorTransform());
    }

    ck::ecs::Display(TEXT("[M2b] Request_RebindActor: re-bridged Actor=[{}] to restored Entity=[{}]"),
        InActor->GetName(), InEntity);
}

// --------------------------------------------------------------------------------------------------------------------
