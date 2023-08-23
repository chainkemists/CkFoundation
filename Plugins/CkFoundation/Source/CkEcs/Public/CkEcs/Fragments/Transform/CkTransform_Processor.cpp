#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"

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
        const auto previousTransform = InComp.Get_Transform();

        algo::ForEachRequest(InRequestsComp._LocationRequests,
        [&](const FCk_Fragment_Transform_Requests::LocationRequestType& InRequest)
        {
            std::visit
            (
                [&](const auto& InRequestVariant)
                {
                    DoHandleRequest(InHandle, InComp, InRequestVariant);
                }, InRequest
            );
        });

        algo::ForEachRequest(InRequestsComp._RotationRequests,
        [&](const FCk_Fragment_Transform_Requests::RotationRequestType& InRequest)
        {
            std::visit
            (
                [&](const auto& InRequestVariant)
                {
                    DoHandleRequest(InHandle, InComp, InRequestVariant);
                }, InRequest
            );
        });

        algo::ForEachRequest(InRequestsComp._ScaleRequests,
        [&](const FCk_Fragment_Transform_Requests::ScaleRequestType& InRequest)
        {
            InComp._Transform.SetScale3D(InRequest.Get_NewScale());
        });

        const auto& newTransform = InComp.Get_Transform();

        if (NOT previousTransform.Equals(newTransform))
        {
            ecs::VeryVerbose(TEXT("Updated Transform [Old: {} | New: {}] of Entity [{}]"), previousTransform, newTransform, InHandle);

            InComp._Transform = newTransform;
        }

        InHandle.Remove<MarkedDirtyBy>();
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
}

// --------------------------------------------------------------------------------------------------------------------
