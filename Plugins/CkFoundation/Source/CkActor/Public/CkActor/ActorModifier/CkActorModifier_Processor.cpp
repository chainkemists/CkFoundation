#include "CkActorModifier_Processor.h"

#include "CkActorModifier_Utils.h"

#include "CkActor/CkActor_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_ActorModifier_SpawnActor_HandleRequests::
        ForEachEntity(
            const TimeType&                                InDeltaT,
            HandleType                                     InHandle,
            FCk_Fragment_ActorModifier_SpawnActorRequests& InRequests) const
        -> void
    {
        for (const auto& InRequest : InRequests._Requests)
        {
            actor::VeryVerbose(TEXT("Handling SpawnActor Request for Entity [{}]"), InHandle);

            switch(InRequest.Get_SpawnPolicy())
            {
            case ECk_SpawnActor_SpawnPolicy::SpawnOnInstanceWithOwership:
            {
                auto OwningActor = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld().Get());

                CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
                    TEXT("SpawnPolicy [{}] REQUIRES the Owner to be an Actor. Unable to Spawn [{}]"),
                    InRequest.Get_SpawnPolicy(),
                    InRequest.Get_SpawnParams().Get_ActorClass())
                { continue; }

                const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

                if (ck::IsValid(OutermostActor))
                {
                    if (OutermostActor->GetLocalRole() == ROLE_AutonomousProxy ||
                        (OutermostActor->GetLocalRole() == ROLE_Authority && OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy))
                    { break; }
                }

                actor::VeryVerbose(TEXT("Skipped Spawning [{}] from Entity [{}] because spawn policy is [{}] and our role is "
                    "neither ROLE_AutomousProxy NOR (if we are a client) do we have ROLE_Authority"),
                    InRequest.Get_SpawnParams().Get_ActorClass(), InHandle, InRequest.Get_SpawnPolicy());

                continue;
            }
            case ECk_SpawnActor_SpawnPolicy::SpawnOnServer:
            {
                const auto OwnerOrWorld = Cast<AActor>(InRequest.Get_SpawnParams().Get_OwnerOrWorld());

                CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("Unable to Spawn [{}] from Entity [{}] because OwnerOrWorld is [{}]"),
                    InRequest.Get_SpawnParams().Get_ActorClass(), InHandle, InRequest.Get_SpawnParams().Get_OwnerOrWorld())
                { continue; }

                if (NOT OwnerOrWorld->GetWorld()->IsNetMode(NM_Client))
                { break; }

                actor::VeryVerbose(TEXT("Skipped Spawning [{}] from Entity [{}] because spawn policy is [{}] and we are a client"),
                    InRequest.Get_SpawnParams().Get_ActorClass(), InHandle, InRequest.Get_SpawnPolicy());

                continue;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_SpawnPolicy());
            }
            }

            const auto& SpawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor(InRequest.Get_SpawnParams(), InRequest.Get_PreFinishSpawnFunc());

            const auto& postSpawnParams = InRequest.Get_PostSpawnParams();

            switch (const auto& postSpawnPolicy = postSpawnParams.Get_PostSpawnPolicy())
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
                CK_INVALID_ENUM(postSpawnPolicy);
                break;
            };
            }

            // TODO: fire ActorSpawned signal
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_SpawnActorRequests>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_ActorModifier_AddActorComponent_HandleRequests::
        ForEachEntity(
            const TimeType&                                       InDeltaT,
            HandleType                                            InHandle,
            const FCk_Fragment_OwningActor_Current&                 InOwningActorComp,
            FCk_Fragment_ActorModifier_AddActorComponentRequests& InRequests) const
        -> void
    {
        for (const auto& InRequest : InRequests._Requests)
        {
            actor::VeryVerbose(TEXT("Handling AddActorComponent Request for Entity [{}]"), InHandle);

            const auto& entityActor = InOwningActorComp.Get_EntityOwningActor().Get();

            CK_ENSURE_IF_NOT(ck::IsValid(entityActor), TEXT("AddActorComponent Request on Entity [{}] that has NO Actor!"), InHandle)
            { return; }

            const auto& componentParams = InRequest.Get_ComponentParams();
            const auto& attachmentType  = componentParams.Get_AttachmentType();

            const auto& parent = [&]() -> USceneComponent*
            {
                const auto& componentParent = componentParams.Get_Parent();

                if (attachmentType == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
                { return nullptr; }

                return ck::IsValid(componentParent) ? componentParent : entityActor->GetRootComponent();
            }();


            auto* addedActorComponent = UCk_Utils_Actor_UE::Request_AddNewActorComponent
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params
                {
                    entityActor,
                    InRequest.Get_ComponentToAdd(),
                    InRequest.Get_IsUnique(),
                    parent,
                    componentParams.Get_AttachmentSocket()
                }
            );

            CK_ENSURE_IF_NOT(ck::IsValid(addedActorComponent), TEXT("Failed to Add new Actor Component [{}] to Entity [{}]"), InRequest.Get_ComponentToAdd(), InHandle)
            { return; }

            addedActorComponent->SetComponentTickEnabled(componentParams.Get_IsTickEnabled());
            addedActorComponent->SetComponentTickInterval(componentParams.Get_TickInterval().Get_Seconds());

            if (const auto& initializerFunc = InRequest.Get_InitializerFunc())
            {
                initializerFunc(addedActorComponent);
            }

            if (attachmentType == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
            {
                auto* sceneComponent = Cast<USceneComponent>(addedActorComponent);

                if (CK_ENSURE(ck::IsValid(sceneComponent),
                           TEXT("The created Component [{}] on Entity [{}] is specified to be [{}] "
                                "however it is NOT a SceneComponent and cannot be transformed to the "
                                "Actor's starting location"),
                            addedActorComponent,
                            InHandle,
                            componentParams.Get_AttachmentType()))
                {
                    sceneComponent->SetWorldTransform(entityActor->GetTransform());
                }
            }

            // TODO: fire signal when component added
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_AddActorComponentRequests>();
    }
}

// --------------------------------------------------------------------------------------------------------------------