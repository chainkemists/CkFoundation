#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkEcsBasics/CkEcsBasics_Log.h"
#include "CkEcsBasics/Settings/CkEcsBasics_Settings.h"
#include "CkEcsBasics/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Setup::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        Super::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Transform_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform_Params& InParams) const
        -> void
    {
        if (UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            const auto Actor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);
            if (ck::Is_NOT_Valid(Actor))
            {
                const auto RootComponent = Actor->GetRootComponent();
                InHandle.Add<FFragment_Transform_RootComponent>(RootComponent);
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_HandleRequests::
        Tick(TimeType InDeltaT) -> void
    {
        _TransientEntity.Clear<FTag_Transform_Updated>();

        TProcessor::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Transform_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InComp,
            FFragment_Transform_Requests& InRequestsComp) const
        -> void
    {
        InComp.Set_ComponentsModified(ECk_TransformComponents::None);

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
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Scale);
        });

        const auto& NewTransform = InComp.Get_Transform();

        if (NOT PreviousTransform.Equals(NewTransform))
        {
            ecs_basics::VeryVerbose(TEXT("Updated Transform [Old: {} | New: {}] of Entity [{}]"), PreviousTransform, NewTransform, InHandle);
            InHandle.Add<ck::FTag_Transform_Updated>();
        }
        else
        {
            InComp.Set_ComponentsModified(ECk_TransformComponents::None);
        }
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetLocation& InRequest)
        -> void
    {
        const auto& NewLocation = InRequest.Get_NewLocation();

        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            const auto RootComponent = RootComponentFragment.Get_RootComponent();
            constexpr auto Sweep = false;

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeLocation(NewLocation, Sweep);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldLocation(NewLocation, Sweep);
                    break;
                }
            }

            return;
        }

        InComp._Transform.SetLocation(NewLocation);
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest)
        -> void
    {
        const auto& DeltaLocation = InRequest.Get_DeltaLocation();

        if (DeltaLocation.IsZero())
        { return; }

        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            const auto RootComponent = RootComponentFragment.Get_RootComponent();
            constexpr auto Sweep = false;

            const auto NewLocation = DeltaLocation + RootComponent->GetComponentLocation();

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeLocation(NewLocation, Sweep);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldLocation(NewLocation, Sweep);
                    break;
                }
            }
            return;
        }

        InComp._Transform.AddToTranslation(DeltaLocation);
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetRotation& InRequest)
        -> void
    {
        const auto& NewRotation = InRequest.Get_NewRotation();

        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            const auto RootComponent = RootComponentFragment.Get_RootComponent();
            constexpr auto Sweep = false;

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeRotation(NewRotation, Sweep);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldRotation(NewRotation, Sweep);
                    break;
                }
            }

            return;
        }

        InComp._Transform.SetRotation(NewRotation.Quaternion());
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest)
        -> void
    {
        const auto& DeltaRotation = InRequest.Get_DeltaRotation();

        if (DeltaRotation.IsZero())
        { return; }

        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            const auto RootComponent = RootComponentFragment.Get_RootComponent();
            constexpr auto Sweep = false;

            const auto NewRotation = DeltaRotation + RootComponent->GetComponentRotation();

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeRotation(NewRotation, Sweep);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldRotation(NewRotation, Sweep);
                    break;
                }
            }

            return;
        }

        InComp._Transform.ConcatenateRotation(DeltaRotation.Quaternion());
    }

    auto
        FProcessor_Transform_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetScale& InRequest)
        -> void
    {
        const auto& NewScale = InRequest.Get_NewScale();

        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            const auto RootComponent = RootComponentFragment.Get_RootComponent();

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeScale3D(InRequest.Get_NewScale());
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldScale3D(InRequest.Get_NewScale());
                    break;
                }
            }

            return;
        }

        InComp._Transform.SetScale3D(NewScale);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Actor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActor,
            const FFragment_Transform& InComp) const
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
            const FFragment_Transform& InCurrent) const
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
            FFragment_Transform& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle, [&](UCk_Fragment_Transform_Rep* InRepComp)
        {
            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Location))
            { InRepComp->_Location = InCurrent.Get_Transform().GetLocation(); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Rotation))
            { InRepComp->_Rotation = InCurrent.Get_Transform().GetRotation(); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Scale))
            { InRepComp->_Scale = InCurrent.Get_Transform().GetScale3D(); }

            InCurrent.Set_ComponentsModified(ECk_TransformComponents::None);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_InterpolateToGoal_Location::
        ForEachEntity(TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_Transform_NewGoal_Location& InGoal) const
        -> void
    {
        if (NOT UCk_Utils_EcsBasics_Settings_UE::Get_EnableTransformSmoothing())
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
            FFragment_Transform& InCurrent,
            FFragment_Transform_NewGoal_Rotation& InGoal) const
        -> void
    {
        if (NOT UCk_Utils_EcsBasics_Settings_UE::Get_EnableTransformSmoothing())
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
