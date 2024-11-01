#include "CkActorModifier_Processor.h"

#include "CkActorModifier_Utils.h"

#include "CkActor/CkActor_Log.h"

#include <Engine/World.h>

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ActorModifier_SpawnActor_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ActorModifier_SpawnActor_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_SpawnActorRequests& InRequests)
        -> void
    {
        DoHandleRequest(InHandle, InRequests.Get_Request());

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_ActorModifier_SpawnActor_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ActorModifier_SpawnActor& InRequest)
        -> void
    {
        switch(const auto& SpawnPolicy = InRequest.Get_SpawnPolicy())
        {
            case ECk_SpawnActor_SpawnPolicy::SpawnOnInstanceWithOwnership:
            {
                const auto OwningActor = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld().Get());

                CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
                    TEXT("SpawnPolicy [{}] REQUIRES the Owner to be an Actor. Unable to Spawn [{}]"),
                    SpawnPolicy,
                    InRequest.Get_SpawnParams().Get_ActorClass())
                { return; }

                if (const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);
                    ck::IsValid(OutermostActor))
                {
                    if (OutermostActor->GetLocalRole() == ROLE_AutonomousProxy ||
                        (OutermostActor->GetLocalRole() == ROLE_Authority && OutermostActor->GetRemoteRole() == ROLE_AutonomousProxy))
                    { break; }
                }

                actor::Verbose
                (
                    TEXT("Skipped Spawning [{}] because SpawnPolicy is [{}] and our role is "
                         "neither ROLE_AutomousProxy NOR (if we are a client) do we have ROLE_Authority"),
                    InRequest.Get_SpawnParams().Get_ActorClass(),
                    SpawnPolicy
                );

                return;
            }
            case ECk_SpawnActor_SpawnPolicy::SpawnOnServer:
            {
                const auto OwnerOrWorld = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld());

                CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld),
                    TEXT("Unable to Spawn [{}] because OwnerOrWorld is [{}]"),
                    InRequest.Get_SpawnParams().Get_ActorClass(),
                    InRequest.Get_SpawnParams().Get_OwnerOrWorld())
                { return; }

                if (NOT OwnerOrWorld->GetWorld()->IsNetMode(NM_Client))
                { break; }

                actor::Verbose
                (
                    TEXT("Skipped Spawning [{}] because SpawnPolicy is [{}] and we are a client"),
                    InRequest.Get_SpawnParams().Get_ActorClass(),
                    SpawnPolicy
                );

                return;
            }
            default:
            {
                CK_INVALID_ENUM(SpawnPolicy);
                return;
            }
        }

        const auto& SpawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor(InRequest.Get_SpawnParams(), InRequest.Get_PreFinishSpawnFunc());

        CK_ENSURE_IF_NOT(ck::IsValid(SpawnedActor), TEXT("Failed to Spawn Actor [{}]"), InRequest.Get_SpawnParams().Get_ActorClass())
        { return; }

        const auto& PostSpawnParams = InRequest.Get_PostSpawnParams();

        switch (const auto& PostSpawnPolicy = PostSpawnParams.Get_PostSpawnPolicy())
        {
            case ECk_SpawnActor_PostSpawnPolicy::None:
            {
                break;
            };
            case ECk_SpawnActor_PostSpawnPolicy::AttachImmediately:
            {
                // TODO: get attachments to work
                break;
            };
            default:
            {
                CK_INVALID_ENUM(PostSpawnPolicy);
                break;
            };
        }

        UUtils_Signal_OnActorSpawned::Broadcast(InHandle, MakePayload(SpawnedActor,
            UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag, IgnoreSucceededFailed)));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ActorModifier_AddActorComponent_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ActorModifier_AddActorComponent_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_AddActorComponentRequests& InRequests)
        -> void
    {
        DoHandleRequest(InHandle, InRequests.Get_Request());

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_ActorModifier_AddActorComponent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ActorModifier_AddActorComponent& InRequest)
        -> void
    {
        const auto& ComponentParams = InRequest.Get_ComponentParams();
        const auto& AttachmentType  = ComponentParams.Get_AttachmentType();
        const auto& ComponentParent = ComponentParams.Get_Parent();

        CK_ENSURE_IF_NOT(ck::IsValid(ComponentParent), TEXT("Invalid Parent Actor Component supplied to Request_AddActorComponent"))
        { return; }

        const auto& ComponentOwner  = ComponentParent->GetOwner();

        if (ck::Is_NOT_Valid(ComponentOwner))
        { return; }

        if (ComponentOwner->IsPendingKillPending())
        { return; }

        const auto& ParentComponent = [&]() -> USceneComponent*
        {
            if (AttachmentType == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
            { return {}; }

            return ck::IsValid(ComponentParent) ? ComponentParent.Get() : ComponentOwner->GetRootComponent();
        }();

        const auto& OptionalPayload = UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag, IgnoreSucceededFailed);

        const auto AddActorCompParams  = UCk_Utils_Actor_UE::AddNewActorComponent_Params{ComponentOwner, InRequest.Get_ComponentToAdd(), ParentComponent}
                                            .Set_IsUnique(InRequest.Get_IsUnique())
                                            .Set_Socket(ComponentParams.Get_AttachmentSocket())
                                            .Set_Tags(ComponentParams.Get_Tags());

        const auto& PreFinishInitializerFunc = [&](UActorComponent* InActorComponent)
        {
            InActorComponent->SetComponentTickEnabled(ComponentParams.Get_IsTickEnabled());
            InActorComponent->SetComponentTickInterval(ComponentParams.Get_TickInterval().Get_Seconds());

            if (const auto& InitializerFunc = InRequest.Get_InitializerFunc())
            {
                InitializerFunc(InActorComponent);
            }

            InRequest.Get_InitializerFunc_BP().ExecuteIfBound(InActorComponent, OptionalPayload);
        };

        auto* AddedActorComponent = UCk_Utils_Actor_UE::Request_AddNewActorComponent<UActorComponent>(AddActorCompParams, PreFinishInitializerFunc);

        CK_ENSURE_IF_NOT(ck::IsValid(AddedActorComponent), TEXT("Failed to Add new Actor Component [{}]"), InRequest.Get_ComponentToAdd())
        { return; }

        switch (AttachmentType)
        {
            case ECk_ActorComponent_AttachmentPolicy::Attach:
            {
                if (const auto* SceneComponent = Cast<USceneComponent>(AddedActorComponent);
                    ck::IsValid(SceneComponent))
                {
                    const auto& AddedComponentMobility = SceneComponent->Mobility;

                    CK_ENSURE_IF_NOT(AddedComponentMobility == EComponentMobility::Movable,
                        TEXT("The created Actor Component [{}] was has its Component Mobility set to [{}], but is meant to ATTACH to the Owner [{}].\n"
                             "Change its Component Mobility to MOVABLE for it to work properly"),
                        AddedActorComponent,
                        AddedComponentMobility.GetValue(),
                        ComponentOwner)
                    {}
                }

                break;
            }
            case ECk_ActorComponent_AttachmentPolicy::DoNotAttach:
            {
                if (auto* SceneComponent = Cast<USceneComponent>(AddedActorComponent);
                    CK_ENSURE(ck::IsValid(SceneComponent),
                        TEXT("The created Actor Component [{}] is specified to be [{}] "
                             "however it is NOT a SceneComponent and cannot be transformed to the "
                             "Actor's starting location"),
                        AddedActorComponent,
                        AttachmentType))
                {
                    SceneComponent->SetWorldTransform(ComponentOwner->GetTransform());
                }

                break;
            }
            default:
            {
                CK_INVALID_ENUM(AttachmentType);
                break;
            }
        }

        actor::Verbose(TEXT("ADDING Actor Component [{}] to Actor [{}]"), AddedActorComponent, ComponentOwner);

        UUtils_Signal_OnActorComponentAdded::Broadcast(InHandle, MakePayload(AddedActorComponent->GetOwner(), AddedActorComponent, OptionalPayload));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ActorModifier_RemoveActorComponent_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ActorModifier_RemoveActorComponent_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_RemoveActorComponentRequests& InRequests)
        -> void
    {
        DoHandleRequest(InHandle, InRequests.Get_Request());

        InHandle.Remove<MarkedDirtyBy, IsValid_Policy_IncludePendingKill>();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_ActorModifier_RemoveActorComponent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ActorModifier_RemoveActorComponent& InRequest)
        -> void
    {
        constexpr auto IncludePendingKill = true;

        const auto& ComponentToRemove         = InRequest.Get_ComponentToRemove().Get(IncludePendingKill);
        const auto& PromoteChildrenComponents = InRequest.Get_PromoteChildrenComponents();

        CK_ENSURE_IF_NOT(ck::IsValid(ComponentToRemove, ck::IsValid_Policy_IncludePendingKill{}), TEXT("Invalid Actor Component to REMOVE"))
        { return; }

        if (ck::Is_NOT_Valid(ComponentToRemove))
        { return; }

        const auto& ComponentOwner = ComponentToRemove->GetOwner();
        const auto& ComponentToRemoveClass = ComponentToRemove->GetClass();

        actor::Verbose(TEXT("REMOVING Actor Component [{}] from Actor [{}]"), ComponentToRemove, ComponentOwner);

        UCk_Utils_Actor_UE::Request_RemoveActorComponent(ComponentToRemove, PromoteChildrenComponents);

        UUtils_Signal_OnActorComponentRemoved::Broadcast(InHandle, MakePayload(ComponentOwner, ComponentToRemoveClass,
            UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag, IgnoreSucceededFailed)));
    }
}

// --------------------------------------------------------------------------------------------------------------------