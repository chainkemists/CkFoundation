#include "CkTransform_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/CkEcsExt_Log.h"
#include "CkEcsExt/Settings/CkEcsExt_Settings.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkTransform"), STATGROUP_CkTransform, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Transform HandleRequests::Drain"), STAT_CkTransform_HandleRequests_Drain, STATGROUP_CkTransform);
DECLARE_CYCLE_STAT(TEXT("Transform HandleRequests::CancelUndrained"), STAT_CkTransform_HandleRequests_CancelUndrained, STATGROUP_CkTransform);
DECLARE_CYCLE_STAT(TEXT("Transform HandleRequests::Clear"), STAT_CkTransform_HandleRequests_Clear, STATGROUP_CkTransform);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_SyncFromActor);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_SyncFromMeshSocket);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_InterpolateToGoal_Location);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_InterpolateToGoal_Rotation);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_SyncToActor);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_Cleanup);

namespace ck_transform
{
    auto
        CalculateTeleportType(
            const USceneComponent* InComponent)
        -> ETeleportType
    {
        if (const auto PrimitiveComponent = Cast<const UPrimitiveComponent>(InComponent);
            ck::IsValid(PrimitiveComponent) && PrimitiveComponent->IsSimulatingPhysics())
        {
            return ETeleportType::TeleportPhysics;
        }

        return ETeleportType::None;
    }

    template <typename THandle>
    auto
        ResolveTeleportType(
            THandle& InHandle,
            const ck::FFragment_Transform_RootComponent& InRootComponent)
        -> ETeleportType
    {
        if (InHandle.template Has<ck::FFragment_Transform_RootComponentTeleportType>())
        {
            return InHandle.template Get<ck::FFragment_Transform_RootComponentTeleportType>().Get_TeleportType();
        }

        const auto RootComponent = InRootComponent.Get_RootComponent().Get();
        if (ck::Is_NOT_Valid(RootComponent))
        {
            return ETeleportType::None;
        }

        const auto TeleportType = CalculateTeleportType(RootComponent);
        InHandle.template AddOrGet<ck::FFragment_Transform_RootComponentTeleportType>() =
            ck::FFragment_Transform_RootComponentTeleportType{TeleportType};

        return TeleportType;
    }

    /// Returns true when the component path was taken (i.e. the caller must NOT also write the fragment).
    template <typename THandle, typename TApplyFn>
    auto
        WithRootComponent(
            THandle& InHandle,
            TApplyFn&& InApplyFn)
        -> bool
    {
        if (NOT InHandle.template Has<ck::FFragment_Transform_RootComponent>())
        {
            return false;
        }

        const auto& RootComponentFragment = InHandle.template Get<ck::FFragment_Transform_RootComponent>();
        const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();

        if (ck::Is_NOT_Valid(RootComponent))
        { return true; }

        const auto TeleportType = ResolveTeleportType(InHandle, RootComponentFragment);

        InApplyFn(RootComponent, TeleportType);

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_SyncFromActor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_Transform_RootComponent& InTransformRootComp)
            -> void
    {
        const auto& RootComponent = InTransformRootComp.Get_RootComponent();

        if (ck::Is_NOT_Valid(RootComponent))
        { return; }

        const auto& RootCompTransform = RootComponent->GetComponentToWorld();

        if (InTransform.Get_Transform().Equals(RootCompTransform))
        { return; }

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, RootCompTransform);

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_SyncFromMeshSocket::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_Transform_MeshSocket& InSocket)
            -> void
    {
        // TODO: Differentiate between Socket and Bone and use the Bone Index to query the Bone transform more efficiently
        // TODO: REMINDER: Bone Index can change based on LODs

        const auto Component = InSocket.Get_Component();

        if (ck::Is_NOT_Valid(Component))
        { return; }

        const auto SocketTransform = Component->GetSocketTransform(InSocket.Get_Socket());

        if (InTransform.Get_Transform().Equals(SocketTransform))
        { return; }

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, SocketTransform);

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_HandleRequests::
        DoTick(
            TimeType InDeltaT) -> void
    {
        // The scheduler skips the main-pass dispatch via MainPassRequiredFragments. Keep this
        // tombstone-aware guard as the pump-path backstop: a once-used in_place_delete pool can remain
        // physically non-empty after its last live request owner is gone.
        if (NOT this->_TransientEntity.Get_RegistryView().Has_AnyLiveEntityWith<FFragment_Transform_Requests>())
        {
            this->_LastVisitedCount = 0;
            return;
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkTransform_HandleRequests_Drain);
            Super::DoTick(InDeltaT);
        }

        // Whatever the drain did not reach — an owner mid-destroy, or one whose RootComponent is not
        // Movable — is about to be discarded by the Clear below. Complete those entries here, or a
        // caller awaiting completion never hears back. This is where the cancellation must live: a
        // separate FGroup_EndPlay processor would find the pool already cleared by this same tick.
        // The owners are collected first because a completion handler is free to enqueue, and the pool
        // being iterated must not grow underneath the view.
        {
            SCOPE_CYCLE_COUNTER(STAT_CkTransform_HandleRequests_CancelUndrained);

            auto UndrainedOwners = TArray<EntityType>{};

            this->_TransientEntity.View<FFragment_Transform_Requests>().ForEach(
            [&](EntityType InEntity, const FFragment_Transform_Requests&)
            {
                UndrainedOwners.Emplace(InEntity);
            });

            for (const auto& UndrainedEntity : UndrainedOwners)
            {
                auto Owner = MakeHandle(UndrainedEntity, this->_TransientEntity);

                // The undrained set is BY DEFINITION the owners the main pass refused, and the common
                // case is an Entity mid-destroy (the drain carries TExclude<FTag_DestroyEntity_Initiate>).
                // A default-policy Get treats a Teardown Entity as invalid: it ensures, and hands back the
                // shared static Invalid_Fragment -- so the cancellations below would run over an EMPTY pool
                // and the very callers this pass exists to serve would never hear back. IncludePendingKill
                // is the policy that matches this pass's own contract.
                const auto& Requests = Owner.Get<FFragment_Transform_Requests, ck::IsValid_Policy_IncludePendingKill>();

                request::FireCancelledForPending(Owner, Requests.Get_LocationRequests());
                request::FireCancelledForPending(Owner, Requests.Get_RotationRequests());

                algo::ForEachRequest(Requests.Get_ScaleRequests(),
                [&](const FCk_Request_Transform_SetScale& InRequest)
                {
                    InRequest.TryFireCompletion(Owner, ECk_Request_OperationResult::Failed_Cancelled);
                }, policy::DontResetContainer{});

                algo::ForEachRequest(Requests.Get_ForceRefreshRequests(),
                [&](const FCk_Request_Transform_ForceRefresh& InRequest)
                {
                    InRequest.TryFireCompletion(Owner, ECk_Request_OperationResult::Failed_Cancelled);
                }, policy::DontResetContainer{});
            }
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkTransform_HandleRequests_Clear);
            this->_TransientEntity.Clear<MarkedDirtyBy>();
        }
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

        const auto HasStaticRootComponent = InHandle.Has<FFragment_Transform_RootComponent>() && NOT InHandle.Has<FTag_Transform_Movable>();

        CK_ENSURE_IF_NOT(NOT HasStaticRootComponent,
            TEXT("Transform request on Entity [{}] with a non-movable RootComponent. "
                 "The request will be discarded to avoid desyncs. "
                 "The USceneComponent has Static or Stationary mobility."),
            InHandle)
        { return; }

        const auto PreviousTransform = InComp.Get_Transform();
        {
            auto& PrevTransform = InHandle.AddOrGet<FFragment_Transform_Previous>();
            PrevTransform = FFragment_Transform_Previous{PreviousTransform};
        }

        auto ScopedMovement = TOptional<FScopedMovementUpdate>{};
        if (InHandle.Has<FFragment_Transform_RootComponent>())
        {
            const auto& RootComponentFragment = InHandle.Get<FFragment_Transform_RootComponent>();
            if (const auto RootComponent = RootComponentFragment.Get_RootComponent().Get();
                ck::IsValid(RootComponent))
            {
                ScopedMovement.Emplace(RootComponent, EScopedUpdate::DeferredUpdates);
            }
        }

        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_Transform_Requests& InRequests)
        {
            // Every DoHandleRequest overload below is void and has no rejection path, so reaching the
            // line after the call IS the success condition.
            algo::ForEachRequest(InRequests._LocationRequests, Visitor(
            [&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InComp, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));

            algo::ForEachRequest(InRequests._RotationRequests, ck::Visitor(
            [&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InComp, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));

            algo::ForEachRequest(InRequests._ScaleRequests,
            [&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InComp, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            });

            // Tagging here (inside HandleRequests' tick) lands in time for FireSignals to pick it up
            // later in the same frame's Transform_Finalize group.
            if (NOT InRequests._ForceRefreshRequests.IsEmpty())
            {
                InHandle.AddOrGet<FTag_Transform_Updated>();

                algo::ForEachRequest(InRequests._ForceRefreshRequests,
                [&](const FCk_Request_Transform_ForceRefresh& InRequest)
                {
                    InRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Succeeded);
                }, policy::DontResetContainer{});

                InRequests._ForceRefreshRequests.Reset();
            }
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

        if (ck_transform::WithRootComponent(InHandle, [&](USceneComponent* RootComponent, ETeleportType TeleportType)
        {
            constexpr auto Sweep = false;

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeLocation(NewLocation, Sweep, nullptr, TeleportType);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldLocation(NewLocation, Sweep, nullptr, TeleportType);
                    break;
                }
            }

            InComp._Transform.SetLocation(RootComponent->GetComponentLocation());
        }))
        { return; }

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

        if (ck_transform::WithRootComponent(InHandle, [&](USceneComponent* RootComponent, ETeleportType TeleportType)
        {
            constexpr auto Sweep = false;
            const auto NewLocation = DeltaLocation + RootComponent->GetComponentLocation();

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeLocation(NewLocation, Sweep, nullptr, TeleportType);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldLocation(NewLocation, Sweep, nullptr, TeleportType);
                    break;
                }
            }

            InComp._Transform.SetLocation(RootComponent->GetComponentLocation());
        }))
        { return; }

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

        if (ck_transform::WithRootComponent(InHandle, [&](USceneComponent* RootComponent, ETeleportType TeleportType)
        {
            constexpr auto Sweep = false;

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeRotation(NewRotation, Sweep, nullptr, TeleportType);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldRotation(NewRotation, Sweep, nullptr, TeleportType);
                    break;
                }
            }

            InComp._Transform.SetRotation(RootComponent->GetComponentRotation().Quaternion());
        }))
        { return; }

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

        if (ck_transform::WithRootComponent(InHandle, [&](USceneComponent* RootComponent, ETeleportType TeleportType)
        {
            constexpr auto Sweep = false;
            const auto NewQuat = DeltaRotation.Quaternion() * RootComponent->GetComponentRotation().Quaternion();
            const auto& NewRotation = NewQuat.Rotator();

            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeRotation(NewRotation, Sweep, nullptr, TeleportType);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldRotation(NewRotation, Sweep, nullptr, TeleportType);
                    break;
                }
            }

            InComp._Transform.SetRotation(RootComponent->GetComponentRotation().Quaternion());
        }))
        { return; }

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

        if (ck_transform::WithRootComponent(InHandle, [&](USceneComponent* RootComponent, ETeleportType /*TeleportType*/)
        {
            switch (InRequest.Get_LocalWorld())
            {
                case ECk_LocalWorld::Local:
                {
                    RootComponent->SetRelativeScale3D(NewScale);
                    break;
                }
                case ECk_LocalWorld::World:
                {
                    RootComponent->SetWorldScale3D(NewScale);
                    break;
                }
            }

            InComp._Transform.SetScale3D(RootComponent->GetComponentScale());
        }))
        { return; }

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
        const auto TeleportType = ck_transform::ResolveTeleportType(InHandle, InTransformRootComp);
        RootComponent->SetWorldTransform(InComp.Get_Transform(), Sweep, nullptr, TeleportType);
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
        // Unconditional, for the same reason as FTag_EntityJustCreated: this marks THIS frame's transform writes,
        // and the load writes restored transforms, so preserving it would leave every restored entity permanently
        // dirty to every consumer that polls it.
        _TransientEntity.Clear_Unconditional<FTag_Transform_Updated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transform_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InCurrent,
            const FFragment_ContainerRef_Location& InLocRef)
            -> void
    {
        auto* Driver = InLocRef.Get_Driver().Get();
        if (ck::Is_NOT_Valid(Driver))
        { return; }

        const auto Mod = InCurrent.Get_ComponentsModified();

        if (EnumHasAnyFlags(Mod, ECk_TransformComponents::Location))
        { Driver->SetFragmentData<FCk_RepData_Location>(FCk_RepData_Location{InCurrent.Get_Transform().GetLocation()}); }

        if (EnumHasAnyFlags(Mod, ECk_TransformComponents::Rotation))
        { Driver->SetFragmentData<FCk_RepData_Rotation>(FCk_RepData_Rotation{InCurrent.Get_Transform().GetRotation()}); }

        if (EnumHasAnyFlags(Mod, ECk_TransformComponents::Scale))
        { Driver->SetFragmentData<FCk_RepData_Scale>(FCk_RepData_Scale{InCurrent.Get_Transform().GetScale3D()}); }

        InCurrent.Set_ComponentsModified(ECk_TransformComponents::None);
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
                FCk_Request_Transform_SetLocation{InCurrent.Get_Transform().GetLocation() + InGoal.Get_InterpolationOffset()},
                {}
            );

            InHandle.Remove<MarkedDirtyBy>();

            return;
        }

        const auto& InterpSettings = InParams.Get_Data().Get_InterpolationSettings();
        const auto& SmoothTime = InterpSettings.Get_SmoothLocationTime();

        const auto PrevDeltaT = InGoal.Get_DeltaT();
        InGoal.Set_DeltaT(PrevDeltaT + InDeltaT);

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            // Terminate-and-snap: previous frames accumulated Offset * (PrevDeltaT / SmoothTime).
            // The remaining (1 - PrevDeltaT/SmoothTime) is what we owe to land exactly on
            // start+offset, regardless of float-accumulation drift in the per-frame sum.
            const auto PrevFraction = FMath::Clamp(PrevDeltaT / SmoothTime, 0.0f, 1.0f);
            const auto RemainingFraction = 1.0f - PrevFraction;
            if (RemainingFraction > 0.0f)
            {
                const auto FinalOffset = InGoal.Get_InterpolationOffset() * RemainingFraction;
                UCk_Utils_Transform_UE::Request_SetLocation
                (
                    InHandle,
                    FCk_Request_Transform_SetLocation{InCurrent.Get_Transform().GetLocation() + FinalOffset},
                    {}
                );
            }
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Fraction =  FMath::Clamp(InDeltaT / SmoothTime, 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetLocation
        (
            InHandle,
            FCk_Request_Transform_SetLocation{InCurrent.Get_Transform().GetLocation() + GoalFraction},
            {}
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
                FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + InGoal.Get_InterpolationOffset()},
                {}
            );

            InHandle.Remove<MarkedDirtyBy>();

            return;
        }

        const auto& Interpolation_Settings = InParams.Get_Data().Get_InterpolationSettings();
        const auto& SmoothTime = Interpolation_Settings.Get_SmoothRotationTime();

        const auto PrevDeltaT = InGoal.Get_DeltaT();
        InGoal.Set_DeltaT(PrevDeltaT + InDeltaT);

        if (InGoal.Get_DeltaT() > SmoothTime)
        {
            // Terminate-and-snap: see location processor's matching branch.
            const auto PrevFraction = FMath::Clamp(PrevDeltaT / SmoothTime, 0.0f, 1.0f);
            const auto RemainingFraction = 1.0f - PrevFraction;
            if (RemainingFraction > 0.0f)
            {
                const auto FinalOffset = InGoal.Get_InterpolationOffset() * RemainingFraction;
                UCk_Utils_Transform_UE::Request_SetRotation
                (
                    InHandle,
                    FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + FinalOffset},
                    {}
                );
            }
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Fraction =  FMath::Clamp(InDeltaT / SmoothTime, 0.0f, 1.0f);
        const auto GoalFraction = InGoal.Get_InterpolationOffset() * Fraction;

        UCk_Utils_Transform_UE::Request_SetRotation
        (
            InHandle,
            FCk_Request_Transform_SetRotation{InCurrent.Get_Transform().GetRotation().Rotator() + GoalFraction},
            {}
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
