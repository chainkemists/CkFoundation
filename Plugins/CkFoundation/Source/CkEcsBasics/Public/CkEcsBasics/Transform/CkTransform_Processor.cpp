#include "CkTransform_Processor.h"

#include "CkTransform_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsBasics/CkEcsBasics_Log.h"

#include "CkNet/CkNet_Fragment.h"
#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Transform_HandleRequests::
        Tick(TimeType InDeltaT) -> void
    {
        _Registry.Clear<FTag_Transform_Updated>();

        TProcessor::Tick(InDeltaT);

        _Registry.Clear<MarkedDirtyBy>();
    }

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
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Location);
        }));

        algo::ForEachRequest(InRequestsComp._RotationRequests, ck::Visitor(
        [&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InComp, InRequestVariant);
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Rotation);
        }));

        algo::ForEachRequest(InRequestsComp._ScaleRequests,
        [&](const FCk_Fragment_Transform_Requests::ScaleRequestType& InRequest)
        {
            InComp._Transform.SetScale3D(InRequest.Get_NewScale());
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Scale);
        });

        const auto& NewTransform = InComp.Get_Transform();

        if (NOT PreviousTransform.Equals(NewTransform))
        {
            ecs_basics::VeryVerbose(TEXT("Updated Transform [Old: {} | New: {}] of Entity [{}]"), PreviousTransform, NewTransform, InHandle);
            InComp._Transform = NewTransform;
            InHandle.Add<ck::FTag_Transform_Updated>();
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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_OwningActor_Current& InOwningActor,
            const FCk_Fragment_Transform_Current& InComp) const
        -> void
    {
        const auto EntityOwningActor = InOwningActor.Get_EntityOwningActor();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityOwningActor), TEXT("Entity [{}] does NOT have a valid Owning Actor. Was it destroyed?"), InHandle)
        { return; }

        EntityOwningActor->SetActorTransform(InComp.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Transform_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Transform_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle, [&](UCk_Fragment_Transform_Rep* InRepComp)
        {
            if ((InCurrent.Get_ComponentsModified() & ECk_TransformComponents::Location) == ECk_TransformComponents::Location)
            { InRepComp->_Location = InCurrent.Get_Transform().GetLocation(); }

            if ((InCurrent.Get_ComponentsModified() & ECk_TransformComponents::Rotation) == ECk_TransformComponents::Rotation)
            { InRepComp->_Rotation = InCurrent.Get_Transform().GetRotation(); }

            if ((InCurrent.Get_ComponentsModified() & ECk_TransformComponents::Scale) == ECk_TransformComponents::Scale)
            { InRepComp->_Scale = InCurrent.Get_Transform().GetScale3D(); }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Transform_InterpolateToGoal::
        ForEachEntity(TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Interpolation_Params& InParams,
            FCk_Fragment_Transform_Current& InCurrent,
            FCk_Fragment_Transform_NewGoal_Location& InGoal) const
        -> void
    {
        // TODO: pre-calculate when creating FCk_Fragment_Transform_NewGoal to avoid this expensive operation
        const auto GoalDistance = InGoal.Get_InterpolationOffset().Length();
        InGoal.Set_DeltaT(InGoal.Get_DeltaT() + InDeltaT);

        if (GoalDistance > InParams.Get_MaxSmoothUpdateDistance())
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset
            (
                InHandle,
                FCk_Request_Transform_AddLocationOffset{}.Set_DeltaLocation(InGoal.Get_InterpolationOffset())
            );

            InHandle.Remove<FCk_Fragment_Transform_NewGoal_Location>();
            return;
        }

        // algorithm:
        // - calculate the fraction of the goal we need to interpolate this frame
        // - add the fraction of the goal to the current location

        const auto SmoothTime = InParams.Get_SmoothLocationTime();

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            InHandle.Remove<FCk_Fragment_Transform_NewGoal_Location>();
            return;
        }

        const auto Fraction =  FMath::Clamp((InDeltaT / SmoothTime).Get_Seconds(), 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetLocation
        (
            InHandle,
            FCk_Request_Transform_SetLocation{}.Set_NewLocation(InCurrent.Get_Transform().GetLocation() + GoalFraction)
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
