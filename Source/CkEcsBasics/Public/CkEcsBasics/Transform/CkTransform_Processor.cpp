#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkEcsBasics/CkEcsBasics_Log.h"
#include "CkEcsBasics/Transform/CkTransform_Settings.h"
#include "CkEcsBasics/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_HandleRequests::
        Tick(TimeType InDeltaT) -> void
    {
        _Registry.Clear<FTag_Transform_Updated>();

        TProcessor::Tick(InDeltaT);

        _Registry.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Transform_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            FFragment_Transform_Requests& InRequestsComp) const
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
        [&](const FFragment_Transform_Requests::ScaleRequestType& InRequest)
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
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) const
        -> void
    {
        const auto& newLocation = InRequest.Get_NewLocation();

        if (InRequest.Get_RelativeAbsolute() == ECk_RelativeAbsolute::Relative && UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            const auto& basicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);
            basicDetails.Get_Actor()->SetActorRelativeLocation(newLocation);

            InComp._Transform.SetLocation(basicDetails.Get_Actor()->GetActorLocation());

            return;
        }

        InComp._Transform.SetLocation(newLocation);
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) const
        -> void
    {
        const auto& deltaLocation = InRequest.Get_DeltaLocation();

        if (deltaLocation.IsZero())
        { return; }

        if (InRequest.Get_LocalWorld() == ECk_LocalWorld::Local && UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            const auto& basicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);
            basicDetails.Get_Actor()->AddActorLocalOffset(deltaLocation);

            InComp._Transform.SetLocation(basicDetails.Get_Actor()->GetActorLocation());

            return;
        }

        InComp._Transform.AddToTranslation(deltaLocation);
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) const
        -> void
    {
        const auto& newRotation = InRequest.Get_NewRotation();

        if (InRequest.Get_RelativeAbsolute() == ECk_RelativeAbsolute::Relative && UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            const auto& basicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);
            basicDetails.Get_Actor()->SetActorRelativeRotation(newRotation);

            InComp._Transform.SetRotation(basicDetails.Get_Actor()->GetActorRotation().Quaternion());

            return;
        }

        InComp._Transform.SetRotation(newRotation.Quaternion());
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) const
        -> void
    {
        const auto& deltaRotation = InRequest.Get_DeltaRotation();

        if (deltaRotation.IsZero())
        { return; }

        if (InRequest.Get_LocalWorld() == ECk_LocalWorld::Local && UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            const auto& basicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);
            basicDetails.Get_Actor()->AddActorLocalRotation(deltaRotation);

            InComp._Transform.SetRotation(basicDetails.Get_Actor()->GetActorRotation().Quaternion());

            return;
        }

        InComp._Transform.ConcatenateRotation(deltaRotation.Quaternion());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Actor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActor,
            const FFragment_Transform_Current& InComp) const
        -> void
    {
        const auto EntityOwningActor = InOwningActor.Get_EntityOwningActor();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityOwningActor), TEXT("Entity [{}] does NOT have a valid Owning Actor. Was it destroyed?"), InHandle)
        { return; }

        EntityOwningActor->SetActorTransform(InComp.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Signal_TransformUpdate& InSignal,
            const FFragment_Transform_Current& InCurrent) const
        -> void
    {
        UUtils_Signal_TransformUpdate::Broadcast(InHandle, MakePayload(InHandle, InCurrent.Get_Transform()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle, [&](UCk_Fragment_Transform_Rep* InRepComp)
        {
            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Location))
            { InRepComp->_Location = InCurrent.Get_Transform().GetLocation(); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Rotation))
            { InRepComp->_Rotation = InCurrent.Get_Transform().GetRotation(); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Scale))
            { InRepComp->_Scale = InCurrent.Get_Transform().GetScale3D(); }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_InterpolateToGoal_Location::
        ForEachEntity(TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Params& InParams,
            FFragment_Transform_Current& InCurrent,
            FFragment_Transform_NewGoal_Location& InGoal) const
        -> void
    {
        if (NOT UCk_Utils_Transform_UserSettings_UE::Get_EnableTransformSmoothing())
        {
            UCk_Utils_Transform_UE::Request_SetLocation
            (
                InHandle,
                FCk_Request_Transform_SetLocation{InCurrent.Get_Transform().GetLocation() + InGoal.Get_InterpolationOffset()}
            );

            InHandle.Remove<MarkedDirtyBy>();

            return;
        }

        // TODO: pre-calculate when creating FFragment_Transform_NewGoal_Location to avoid this expensive operation
        const auto GoalDistance = InGoal.Get_InterpolationOffset().Length();
        InGoal.Set_DeltaT(InGoal.Get_DeltaT() + InDeltaT);

        const auto& InterpSettings = InParams.Get_Data().Get_InterpolationSettings();

        if (GoalDistance > InterpSettings.Get_MaxSmoothUpdateDistance())
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset
            (
                InHandle,
                FCk_Request_Transform_AddLocationOffset{InGoal.Get_InterpolationOffset()}
            );

            InHandle.Remove<MarkedDirtyBy>();

            return;
        }

        // algorithm:
        // - calculate the fraction of the goal we need to interpolate this frame
        // - add the fraction of the goal to the current location

        const auto& SmoothTime = InterpSettings.Get_SmoothLocationTime();

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Fraction =  FMath::Clamp((InDeltaT / SmoothTime).Get_Seconds(), 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetLocation
        (
            InHandle,
            FCk_Request_Transform_SetLocation{InCurrent.Get_Transform().GetLocation() + GoalFraction}
        );
    }

    auto
        FProcessor_Transform_InterpolateToGoal_Rotation::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Params& InParams,
            FFragment_Transform_Current& InCurrent,
            FFragment_Transform_NewGoal_Rotation& InGoal) const
        -> void
    {
        if (NOT UCk_Utils_Transform_UserSettings_UE::Get_EnableTransformSmoothing())
        {
            UCk_Utils_Transform_UE::Request_SetRotation
            (
                InHandle,
                FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + InGoal.Get_InterpolationOffset()}
            );

            InHandle.Remove<MarkedDirtyBy>();

            return;
        }

        InGoal.Set_DeltaT(InGoal.Get_DeltaT() + InDeltaT);

        const auto& InterpSettings = InParams.Get_Data().Get_InterpolationSettings();
        const auto& SmoothTime = InterpSettings.Get_SmoothRotationTime();

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Fraction =  FMath::Clamp((InDeltaT / SmoothTime).Get_Seconds(), 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetRotation
        (
            InHandle,
            FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + GoalFraction}
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
