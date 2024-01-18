#include "CkActorModifier_Processor.h"

#include "CkActorModifier_Utils.h"

#include "CkActor/CkActor_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ActorModifier_SpawnActor_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_SpawnActorRequests& InRequests) const
        -> void
    {
        for (const auto& InRequest : InRequests._Requests)
        {
            actor::VeryVerbose(TEXT("Handling SpawnActor Request for Entity [{}]"), InHandle);

            switch(const auto& SpawnPolicy = InRequest.Get_SpawnPolicy())
            {
                case ECk_SpawnActor_SpawnPolicy::SpawnOnInstanceWithOwership:
                {
                    const auto OwningActor = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld().Get());

                    CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
                        TEXT("SpawnPolicy [{}] REQUIRES the Owner to be an Actor. Unable to Spawn [{}]"),
                        SpawnPolicy,
                        InRequest.Get_SpawnParams().Get_ActorClass())
                    { continue; }

                    const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

                    if (ck::IsValid(OutermostActor))
                    {
                        if (OutermostActor->GetLocalRole() == ROLE_AutonomousProxy ||
                            (OutermostActor->GetLocalRole() == ROLE_Authority && OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy))
                        { break; }
                    }

                    actor::VeryVerbose
                    (
                        TEXT("Skipped Spawning [{}] from Entity [{}] because spawn policy is [{}] and our role is "
                        "neither ROLE_AutomousProxy NOR (if we are a client) do we have ROLE_Authority"),
                        InRequest.Get_SpawnParams().Get_ActorClass(),
                        InHandle,
                        SpawnPolicy
                    );

                    continue;
                }
                case ECk_SpawnActor_SpawnPolicy::SpawnOnServer:
                {
                    const auto OwnerOrWorld = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld());

                    CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld),
                        TEXT("Unable to Spawn [{}] from Entity [{}] because OwnerOrWorld is [{}]"),
                        InRequest.Get_SpawnParams().Get_ActorClass(),
                        InHandle,
                        InRequest.Get_SpawnParams().Get_OwnerOrWorld())
                    { continue; }

                    if (NOT OwnerOrWorld->GetWorld()->IsNetMode(NM_Client))
                    { break; }

                    actor::VeryVerbose
                    (
                        TEXT("Skipped Spawning [{}] from Entity [{}] because spawn policy is [{}] and we are a client"),
                        InRequest.Get_SpawnParams().Get_ActorClass(),
                        InHandle,
                        SpawnPolicy
                    );

                    continue;
                }
                default:
                {
                    CK_INVALID_ENUM(SpawnPolicy);
                    break;
                }
            }

            const auto& SpawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor(InRequest.Get_SpawnParams(), InRequest.Get_PreFinishSpawnFunc());

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

            UUtils_Signal_OnActorSpawned::Broadcast(InHandle, MakePayload(InHandle, SpawnedActor));
        }

        InHandle.Remove<FFragment_ActorModifier_SpawnActorRequests>();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle, ECk_EntityLifetime_DestructionBehavior::DestroyOnlyIfOrphan);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ActorModifier_AddActorComponent_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActorComp,
            FFragment_ActorModifier_AddActorComponentRequests& InRequests) const
        -> void
    {
        for (const auto& Request : InRequests._Requests)
        {
            actor::VeryVerbose(TEXT("Handling AddActorComponent Request for Entity [{}]"), InHandle);

            constexpr auto IncludePendingKill = true;
            const auto& EntityActor = InOwningActorComp.Get_EntityOwningActor().Get(IncludePendingKill);

            CK_ENSURE_IF_NOT(ck::IsValid(EntityActor, ck::IsValid_Policy_IncludePendingKill{}),
                TEXT("AddActorComponent Request on Entity [{}] that has NO Actor!"),
                InHandle)
            { continue; }

            if (EntityActor->IsPendingKillPending())
            { continue; }

            const auto& ComponentParams = Request.Get_ComponentParams();
            const auto& AttachmentType  = ComponentParams.Get_AttachmentType();

            const auto& Parent = [&]() -> USceneComponent*
            {
                const auto& ComponentParent = ComponentParams.Get_Parent();

                if (AttachmentType == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
                { return nullptr; }

                return ck::IsValid(ComponentParent) ? ComponentParent.Get() : EntityActor->GetRootComponent();
            }();


            auto* AddedActorComponent = UCk_Utils_Actor_UE::Request_AddNewActorComponent
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params
                {
                    EntityActor,
                    Request.Get_ComponentToAdd(),
                    Request.Get_IsUnique(),
                    Parent,
                    ComponentParams.Get_AttachmentSocket()
                }
            );

            CK_ENSURE_IF_NOT(ck::IsValid(AddedActorComponent), TEXT("Failed to Add new Actor Component [{}] to Entity [{}]"), Request.Get_ComponentToAdd(), InHandle)
            { return; }

            AddedActorComponent->SetComponentTickEnabled(ComponentParams.Get_IsTickEnabled());
            AddedActorComponent->SetComponentTickInterval(ComponentParams.Get_TickInterval().Get_Seconds());

            if (const auto& InitializerFunc = Request.Get_InitializerFunc())
            {
                InitializerFunc(AddedActorComponent);
            }

            if (AttachmentType == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
            {
                if (auto* SceneComponent = Cast<USceneComponent>(AddedActorComponent);
                    CK_ENSURE(ck::IsValid(SceneComponent),
                        TEXT("The created Component [{}] on Entity [{}] is specified to be [{}] "
                            "however it is NOT a SceneComponent and cannot be transformed to the "
                            "Actor's starting location"),
                        AddedActorComponent,
                        InHandle,
                        ComponentParams.Get_AttachmentType()))
                {
                    SceneComponent->SetWorldTransform(EntityActor->GetTransform());
                }
            }

            // TODO: fire signal when component added
        }

        InHandle.Remove<FFragment_ActorModifier_AddActorComponentRequests>();
    }
}

// --------------------------------------------------------------------------------------------------------------------