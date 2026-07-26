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

    // Order matters: OwningActor MUST be added before Transform so Transform auto-detects the actor's root.
    UCk_Utils_OwningActor_UE::SetupActorEntityLink(InEntity, InActor);
    UCk_Utils_OwningActor_UE::Add(InEntity, InActor);

    // Transform::Add routes to AddAndAttachToUnrealComponent (OwningActor is now present), which PRESERVES an
    // already-restored world transform and only seeds from the actor's spawn transform when there is none.
    if (ck::IsValid(InActor->GetRootComponent()))
    {
        UCk_Utils_Transform_UE::Add(InEntity, InActor->GetActorTransform());
    }

    InEntity.AddOrGet<ck::FTag_ActorJustRebound>();

    ck::ecs::Display(TEXT("[M2b] Request_RebindActor: re-bridged Actor=[{}] to restored Entity=[{}]"),
        InActor->GetName(), InEntity);
}

// --------------------------------------------------------------------------------------------------------------------
