#include "CkEcsConstructionScript_ActorComponent.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

void UCk_EcsConstructionScript_ActorComponent::BeginDestroy()
{
    Super::BeginDestroy();
}

auto
    UCk_EcsConstructionScript_ActorComponent::
    Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams)
    -> void
{
    Super::Do_Construct_Implementation(InParams);

    CK_ENSURE_IF_NOT(ck::IsValid(_UnrealEntity),
        TEXT("UnrealEntity is [{}]. Did you forget to set the default value in the component?.[{}]"), _UnrealEntity, ck::Context(this))
    { return; }

    const auto WorldSubsystem = Cast<UCk_EcsWorld_Subsystem_UE>(GetWorld()->GetSubsystemBase(_EcsWorldSubsystem));

    // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
    CK_ENSURE_IF_NOT(ck::IsValid(WorldSubsystem),
        TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"), _EcsWorldSubsystem, ck::Context(this))
    { return; }

    _Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldSubsystem->Get_TransientEntity());
    _UnrealEntity->Build(_Entity);

    // --------------------------------------------------------------------------------------------------------------------
    // LINK TO ACTOR
    // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

    auto InHandle = _Entity;
    auto InActor = GetOwner();

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Params>(FCk_Fragment_ActorInfo_ParamsData
    {
        InActor->GetClass(),
        InActor->GetActorTransform(),
        InActor->GetOwner(),
        InActor->GetIsReplicated() ? ECk_Actor_NetworkingType::Replicated : ECk_Actor_NetworkingType::Local
    });

    UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ActorInfo_ActorComponent_UE>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ActorInfo_ActorComponent_UE>
        {
            InActor,
            true
        },
        [&](UCk_ActorInfo_ActorComponent_UE* InComp)
        {
            InComp->_EntityHandle = InHandle;
        }
    );

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Current>(InActor);
}
