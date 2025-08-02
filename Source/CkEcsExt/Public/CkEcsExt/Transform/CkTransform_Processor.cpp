#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkEcsExt/CkEcsExt_Log.h"
#include "CkEcsExt/Settings/CkEcsExt_Settings.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_SyncFromActor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            const FFragment_Transform_RootComponent& InTransformRootComp)
            -> void
    {
        const auto& RootComponent = InTransformRootComp.Get_RootComponent();

        if (ck::Is_NOT_Valid(RootComponent))
        { return; }

        const auto& PreviousTransform = InTransform.Get_Transform();
        {
            auto& PrevTransform = InHandle.AddOrGet<FFragment_Transform_Previous>();
            PrevTransform = FFragment_Transform_Previous{PreviousTransform};
        }

        if (const auto& RootCompTransform = RootComponent->GetComponentToWorld();
            NOT PreviousTransform.Equals(RootCompTransform))
        {
            UCk_Utils_Transform_UE::Request_SetTransform(InHandle, FCk_Request_Transform_SetTransform{RootCompTransform});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_SyncFromMeshSocket::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            const FFragment_Transform_MeshSocket& InSocket)
            -> void
    {
        // TODO: Differentiate between Socket and Bone and use the Bone Index to query the Bone transform more efficiently
        // TODO: REMINDER: Bone Index can change based on LODs

        const auto Component = InSocket.Get_Component();
        const auto SocketName = InSocket.Get_Socket();

        if (ck::Is_NOT_Valid(InSocket.Get_Component()))
        { return; }

        const auto& PreviousTransform = InTransform.Get_Transform();

        if (const auto& SocketTransform = Component->GetSocketTransform(SocketName);
            NOT PreviousTransform.Equals(SocketTransform))
        {
            UCk_Utils_Transform_UE::Request_SetTransform(InHandle, FCk_Request_Transform_SetTransform{SocketTransform});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_HandleRequests::
        DoTick(
            TimeType InDeltaT) -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Transform_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FFragment_Transform_Requests & InRequestsComp) const
        -> void
    {
        InComp.Set_ComponentsModified(ECk_TransformComponents::None);

        const auto PreviousTransform = InComp.Get_Transform();
        {
            auto& PrevTransform = InHandle.AddOrGet<FFragment_Transform_Previous>();
            PrevTransform = FFragment_Transform_Previous{PreviousTransform};
        }

        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_Transform_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._LocationRequests, Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InComp, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));

            algo::ForEachRequest(InRequests._RotationRequests, ck::Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InComp, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));

            algo::ForEachRequest(InRequests._ScaleRequests,
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InComp, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            });
        });

        const auto& NewTransform = InComp.Get_Transform();

        if (NOT NewTransform.GetLocation().Equals(PreviousTransform.GetLocation()))
        {
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Location);
        }

        if (NOT NewTransform.GetRotation().Equals(PreviousTransform.GetRotation()))
        {
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Rotation);
        }

        if (NOT NewTransform.GetScale3D().Equals(PreviousTransform.GetScale3D()))
        {
            InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_TransformComponents::Scale);
        }

        if (EnumHasAnyFlags(InComp.Get_ComponentsModified(), ECk_TransformComponents::Location | ECk_TransformComponents::Rotation | ECk_TransformComponents::Scale))
        {
            ecs_extension::VeryVerbose(TEXT("Updated Transform [Old: {} | New: {}] of Entity [{}]"), PreviousTransform, NewTransform, InHandle);
            UCk_Utils_Transform_UE::Request_TransformUpdated(InHandle);
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
            const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

            if (ck::Is_NOT_Valid(RootComponent))
            { return; }

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

            InComp._Transform.SetLocation(RootComponent->GetComponentLocation());

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
            const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

            if (ck::Is_NOT_Valid(RootComponent))
            { return; }

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
            InComp._Transform.SetLocation(RootComponent->GetComponentLocation());

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
            const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

            if (ck::Is_NOT_Valid(RootComponent))
            { return; }

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

            InComp._Transform.SetRotation(RootComponent->GetComponentRotation().Quaternion());

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
            const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

            if (ck::Is_NOT_Valid(RootComponent))
            { return; }

            constexpr auto Sweep = false;

            const auto NewQuat = DeltaRotation.Quaternion() * RootComponent->GetComponentRotation().Quaternion();
            const auto& NewRotation = NewQuat.Rotator();

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

            InComp._Transform.SetRotation(RootComponent->GetComponentRotation().Quaternion());

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
            const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

            if (ck::Is_NOT_Valid(RootComponent))
            { return; }

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

            InComp._Transform.SetScale3D(RootComponent->GetComponentScale());

            return;
        }

        InComp._Transform.SetScale3D(NewScale);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_SyncToActor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_RootComponent& InTransformRootComp,
            const FFragment_Transform& InComp)
            -> void
    {
        const auto RootComponent = InTransformRootComp.Get_RootComponent().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(RootComponent), TEXT("Entity [{}] does NOT have a valid RootComponent. Was it destroyed?"), InHandle)
        { return; }

        if (InComp.Get_Transform().Equals(RootComponent->GetComponentTransform()))
        { return; }

        constexpr auto Sweep = false;
        RootComponent->SetWorldTransform(InComp.Get_Transform(), Sweep);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Signal_TransformUpdate& InSignal,
            const FFragment_Transform& InCurrent)
        -> void
    {
        UUtils_Signal_TransformUpdate::Broadcast(InHandle, MakePayload(InHandle, InCurrent.Get_Transform()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Transform_Cleanup::
        FProcessor_Transform_Cleanup(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_Transform_Cleanup::
        DoTick(
            TimeType InDeltaT) -> void
    {
        _TransientEntity.Clear<FTag_Transform_Updated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp)
            -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle, [&](UCk_Fragment_Transform_Rep* InRepComp)
        {
            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Location))
            { InRepComp->Set_Location(InCurrent.Get_Transform().GetLocation()); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Rotation))
            { InRepComp->Set_Rotation(InCurrent.Get_Transform().GetRotation()); }

            if (EnumHasAnyFlags(InCurrent.Get_ComponentsModified(), ECk_TransformComponents::Scale))
            { InRepComp->Set_Scale(InCurrent.Get_Transform().GetScale3D()); }

            InCurrent.Set_ComponentsModified(ECk_TransformComponents::None);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_InterpolateToGoal_Location::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Location& InGoal)
            -> void
    {
        if (NOT UCk_Utils_EcsExt_Settings_UE::Get_EnableTransformSmoothing())
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

        const auto Fraction =  FMath::Clamp(InDeltaT / SmoothTime, 0.0f, 1.0f);
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
            const TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Rotation& InGoal)
            -> void
    {
        if (NOT UCk_Utils_EcsExt_Settings_UE::Get_EnableTransformSmoothing())
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

        const auto& Interpolation_Settings = InParams.Get_Data().Get_InterpolationSettings();
        const auto& SmoothTime = Interpolation_Settings.Get_SmoothRotationTime();

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Fraction =  FMath::Clamp(InDeltaT / SmoothTime, 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetRotation
        (
            InHandle,
            FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + GoalFraction}
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
