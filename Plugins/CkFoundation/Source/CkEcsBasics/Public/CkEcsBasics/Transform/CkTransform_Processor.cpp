#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsBasics/CkEcsBasics_Log.h"
#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Transform_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            FCk_Fragment_Transform_Requests& InRequestsComp) const
        -> void
    {
        const auto PreviousTransform = InComp.Get_Transform();

        algo::ForEachRequest(InRequestsComp._LocationRequests, ck::Visitor(
        [&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InComp, InRequestVariant);
        }));

        algo::ForEachRequest(InRequestsComp._RotationRequests, ck::Visitor(
        [&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InComp, InRequestVariant);
        }));

        algo::ForEachRequest(InRequestsComp._ScaleRequests,
        [&](const FCk_Fragment_Transform_Requests::ScaleRequestType& InRequest)
        {
            InComp._Transform.SetScale3D(InRequest.Get_NewScale());
        });

        const auto& NewTransform = InComp.Get_Transform();

        if (NOT PreviousTransform.Equals(NewTransform))
        {
            UCk_Utils_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle, [&](UCk_Fragment_Transform_Rep* InRepComp)
            {
                if (NOT NewTransform.GetLocation().Equals(PreviousTransform.GetLocation()))
                { InRepComp->_Location = NewTransform.GetLocation(); }

                if (NOT NewTransform.GetRotation().Equals(PreviousTransform.GetRotation()))
                { InRepComp->_Rotation = NewTransform.GetRotation(); }

                if (NOT NewTransform.GetScale3D().Equals(PreviousTransform.GetScale3D()))
                { InRepComp->_Scale = NewTransform.GetScale3D(); }
            });

            ecs_basics::VeryVerbose(TEXT("Updated Transform [Old: {} | New: {}] of Entity [{}]"), PreviousTransform, NewTransform, InHandle);
            InComp._Transform = NewTransform;
        }
    }

    auto
        FCk_Processor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) const
        -> void
    {
        InComp._Transform.SetLocation(InRequest.Get_NewLocation());
    }

    auto
        FCk_Processor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) const
        -> void
    {
        InComp._Transform.AddToTranslation(InRequest.Get_DeltaLocation());
    }

    auto
        FCk_Processor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) const
        -> void
    {
        InComp._Transform.SetRotation(InRequest.Get_NewRotation().Quaternion());
    }

    auto
        FCk_Processor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) const
        -> void
    {
        const auto& deltaRotation = InRequest.Get_DeltaRotation();
        InComp._Transform.Rotator().Add(deltaRotation.Pitch, deltaRotation.Yaw, deltaRotation.Roll);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Transform_Actor::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _Registry.Clear<FCk_Processor_Transform_HandleRequests::MarkedDirtyBy>();
    }

    auto
        FCk_Processor_Transform_Actor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_OwningActor_Current& InOwningActor,
            const FCk_Fragment_Transform_Current& InComp,
            const FCk_Fragment_Transform_Requests&) const
        -> void
    {
        const auto EntityOwningActor = InOwningActor.Get_EntityOwningActor();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityOwningActor), TEXT("Entity [{}] does NOT have a valid Owning Actor. Was it destroyed?"), InHandle)
        { return; }

        EntityOwningActor->SetActorTransform(InComp.Get_Transform());
    }
}

// --------------------------------------------------------------------------------------------------------------------
