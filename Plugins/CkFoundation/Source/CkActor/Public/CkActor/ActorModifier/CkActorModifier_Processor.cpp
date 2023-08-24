#include "CkActorModifier_Processor.h"

#include "CkActorModifier_Utils.h"

#include "CkActor/CkActor_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_ActorModifier_Location_HandleRequests::
        ForEachEntity(
            const TimeType&                              InDeltaT,
            HandleType                                   InHandle,
            const FCk_Fragment_OwningActor_Current&      InOwningActorComp,
            FCk_Fragment_ActorModifier_LocationRequests& InRequestsComp) const
        -> void
    {
        for (const auto& InRequest : InRequestsComp._Requests)
        {
            const auto& targetActor = InOwningActorComp.Get_EntityOwningActor().Get();

            CK_ENSURE_IF_NOT(ck::IsValid(targetActor), TEXT("Actor Location Request on Entity [{}] that has NO Actor!"), InHandle)
            { return; }

            const auto previousTransform = targetActor->GetTransform();

            std::visit
            (
                [&](const auto& InRequestVariant)
                {
                    DoHandleRequest(InHandle, targetActor, InRequestVariant);
                }, InRequest
            );

            const auto& newTransform = targetActor->GetTransform();

            if (newTransform.Equals(previousTransform))
            { return; }

            // TODO: fire signal for Actor transform update
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_LocationRequests>();
    }

    auto
        FCk_Processor_ActorModifier_Location_HandleRequests::
        DoHandleRequest(
            HandleType                                  InHandle,
            AActor*                                      InActor,
            const FCk_Request_ActorModifier_SetLocation& InRequest) const
        -> void
    {
        actor::VeryVerbose(TEXT("Handling SetLocation Request for Entity [{}]"), InHandle);

        const auto& newLocation = InRequest.Get_NewLocation();

        switch (const auto& relativeOrAbsolute = InRequest.Get_RelativeAbsolute())
        {
            case ECk_RelativeAbsolute::Relative:
            {
                InActor->SetActorRelativeLocation(newLocation);
                break;
            }
            case ECk_RelativeAbsolute::Absolute:
            {
                InActor->SetActorLocation(newLocation);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(relativeOrAbsolute);
                break;
            }
        }
    }

    auto
        FCk_Processor_ActorModifier_Location_HandleRequests::
        DoHandleRequest(
            HandleType                                       InHandle,
            AActor*                                           InActor,
            const FCk_Request_ActorModifier_AddLocationOffset& InRequest) const
        -> void
    {
        actor::VeryVerbose(TEXT("Handling AddLocationOffset Request for Entity [{}]"), InHandle);

        const auto& deltaLocation = InRequest.Get_DeltaLocation();

        switch (const auto& relativeOrAbsolute = InRequest.Get_LocalWorld())
        {
            case ECk_LocalWorld::Local:
            {
                InActor->AddActorLocalOffset(deltaLocation);
                break;
            }
            case ECk_LocalWorld::World:
            {
                InActor->AddActorWorldOffset(deltaLocation);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(relativeOrAbsolute);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_ActorModifier_Scale_HandleRequests::
        ForEachEntity(
            const TimeType&                           InDeltaT,
            HandleType                                InHandle,
            const FCk_Fragment_OwningActor_Current&     InOwningActorComp,
            FCk_Fragment_ActorModifier_ScaleRequests& InRequestsComp) const
        -> void
    {
        for (const auto& InRequest : InRequestsComp._Requests)
        {
            actor::VeryVerbose(TEXT("Handling SetScale Request for Entity [{}]"), InHandle);

            const auto& targetActor = InOwningActorComp.Get_EntityOwningActor().Get();

            CK_ENSURE_IF_NOT(ck::IsValid(targetActor), TEXT("Actor Scale Request on Entity [{}] that has NO Actor!"), InHandle)
            { return; }

            const auto previousTransform = targetActor->GetTransform();

            switch (const auto& relativeOrAbsolute = InRequest.Get_RelativeAbsolute())
            {
                case ECk_RelativeAbsolute::Relative:
                {
                    targetActor->SetActorRelativeScale3D(InRequest.Get_NewScale());
                    break;
                }
                case ECk_RelativeAbsolute::Absolute:
                {
                    targetActor->SetActorScale3D(InRequest.Get_NewScale());
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(relativeOrAbsolute);
                    break;
                }
            }

            const auto& newTransform = targetActor->GetTransform();

            if (newTransform.Equals(previousTransform))
            { return; }

            // TODO: Fire signal for transform update
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_ScaleRequests>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_ActorModifier_Rotation_HandleRequests::
        ForEachEntity(
            const TimeType&                              InDeltaT,
            HandleType                                   InHandle,
            const FCk_Fragment_OwningActor_Current&        InOwningActorComp,
            FCk_Fragment_ActorModifier_RotationRequests& InRequestsComp) const
        -> void
    {
        for (const auto& InRequest : InRequestsComp._Requests)
        {
            const auto& targetActor = InOwningActorComp.Get_EntityOwningActor().Get();

            CK_ENSURE_IF_NOT(ck::IsValid(targetActor), TEXT("Actor Rotation Request on Entity [{}] that has NO Actor!"), InHandle)
            { return; }

            const auto previousTransform = targetActor->GetTransform();

            std::visit
            (
                [&](const auto& InRequestVariant)
                {
                    DoHandleRequest(InHandle, targetActor, InRequestVariant);
                }, InRequest
            );

            const auto& newTransform = targetActor->GetTransform();

            if (newTransform.Equals(previousTransform))
            { return; }

            // TODO: Fire signal for transform update
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_RotationRequests>();
    }

    auto
        FCk_Processor_ActorModifier_Rotation_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_SetRotation& InRequest) const -> void
    {
        actor::VeryVerbose(TEXT("Handling SetRotation Request for Entity [{}]"), InHandle);

        const auto& newRotation = InRequest.Get_NewRotation();

        switch (const auto& relativeOrAbsolute = InRequest.Get_RelativeAbsolute())
        {
            case ECk_RelativeAbsolute::Relative:
            {
                InActor->SetActorRelativeRotation(newRotation);
                break;
            }
            case ECk_RelativeAbsolute::Absolute:
            {
                InActor->SetActorRotation(newRotation);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(relativeOrAbsolute);
                break;
            }
        }
    }

    auto
        FCk_Processor_ActorModifier_Rotation_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_AddRotationOffset& InRequest) const
        -> void
    {
        actor::VeryVerbose(TEXT("Handling AddRotationOffset Request for Entity [{}]"), InHandle);

        const auto& deltaRotation = InRequest.Get_DeltaRotation();

        switch (const auto& relativeOrAbsolute = InRequest.Get_LocalWorld())
        {
            case ECk_LocalWorld::Local:
            {
                InActor->AddActorLocalRotation(deltaRotation);
                break;
            }
            case ECk_LocalWorld::World:
            {
                InActor->AddActorWorldRotation(deltaRotation);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(relativeOrAbsolute);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_ActorModifier_Transform_HandleRequests::
        ForEachEntity(
            const TimeType&                               InDeltaT,
            HandleType                                    InHandle,
            const FCk_Fragment_OwningActor_Current&         InOwningActorComp,
            FCk_Fragment_ActorModifier_TransformRequests& InRequestsComp) const
        -> void
    {
        for (const auto& InRequest : InRequestsComp._Requests)
        {
            actor::VeryVerbose(TEXT("Handling SetTransform Request for Entity [{}]"), InHandle);

            const auto& targetActor = InOwningActorComp.Get_EntityOwningActor().Get();

            CK_ENSURE_IF_NOT(ck::IsValid(targetActor), TEXT("Actor Transform Request on Entity [{}] that has NO Actor!"), InHandle)
            { return; }

            const auto previousTransform = targetActor->GetTransform();

            switch (const auto& relativeOrAbsolute = InRequest.Get_RelativeAbsolute())
            {
                case ECk_RelativeAbsolute::Relative:
                {
                    targetActor->SetActorRelativeTransform(InRequest.Get_NewTransform());
                    break;
                }
                case ECk_RelativeAbsolute::Absolute:
                {
                    targetActor->SetActorTransform(InRequest.Get_NewTransform());
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(relativeOrAbsolute);
                    break;
                }
            }

            const auto& newTransform = targetActor->GetTransform();

            if (newTransform.Equals(previousTransform))
            { return; }

            // TODO: fire signal for Transform update
        }

        InHandle.Remove<FCk_Fragment_ActorModifier_TransformRequests>();
    }

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

            const auto& spawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor(InRequest.Get_SpawnParams(), InRequest.Get_PreFinishSpawnFunc());

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