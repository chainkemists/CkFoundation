#pragma once

#include "CkTransform_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSEXT_API FProcessor_Transform_SyncFromActor : public TParallelProcessor<
            FProcessor_Transform_SyncFromActor,
            FCk_Handle_Transform,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            TReadOnly<FFragment_Transform_RootComponent>,
            FTag_Transform_Movable,
            TExclude<FTag_Transform_ExternallyDriven>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_Transform_RootComponent& InTransformRootComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_SyncFromMeshSocket : public TParallelProcessor<
            FProcessor_Transform_SyncFromMeshSocket,
            FCk_Handle_Transform,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            TReadOnly<FFragment_Transform_MeshSocket>,
            TExclude<FTag_Transform_ExternallyDriven>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        using RunAfter = TDepList<FProcessor_Transform_SyncFromActor>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_Transform_MeshSocket& InSocket) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The canonical request drain runs once in FGroup_Transform, then may be replayed by the scheduler's
    // local settle barrier after FGroup_Transform_Derived when a derived producer queued more requests.
    class CKECSEXT_API FProcessor_Transform_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Transform_HandleRequests,
            FCk_Handle_Transform,
            ck::TReadWrite<FFragment_Transform>,
            ck::TReadOnly<FFragment_Transform_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_Transform_InterpolateToGoal_Rotation>;
        using LocalSettleAfter = FGroup_Transform_Derived;
        static constexpr auto LocalSettleTrigger = true;
        using MarkedDirtyBy = FFragment_Transform_Requests;

        // The custom DoTick's normal drain, cancellation drain, and pool clear are all meaningful only
        // while at least one live request owner exists. This explicit contract lets the scheduler omit
        // the processor entirely from an idle main pass.
        using MainPassRequiredFragments = entt::type_list<FFragment_Transform_Requests>;

        using Super = ck_exp::TProcessor<FProcessor_Transform_HandleRequests,
            FCk_Handle_Transform,
            ck::TReadWrite<FFragment_Transform>,
            ck::TReadOnly<FFragment_Transform_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>;
        using TimeType = typename Super::TimeType;
        using HandleType = typename Super::HandleType;
        using EntityType = typename Super::EntityType;

    public:
        using Super::Super;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FFragment_Transform_Requests & InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetScale& InRequest) -> void;
    };

    class CKECSEXT_API FProcessor_Transform_SyncToActor : public ck_exp::TProcessor<
            FProcessor_Transform_SyncToActor,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_Transform_RootComponent>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            FTag_Transform_Movable,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Finalize;
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests>;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_RootComponent& InTransformRootComp,
            const FFragment_Transform& InComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_FireSignals : public ck_exp::TProcessor<
            FProcessor_Transform_FireSignals,
            FCk_Handle_Transform,
            ck::TReadWrite<FFragment_Signal_TransformUpdate>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Finalize;
        using RunAfter = TDepList<FProcessor_Transform_SyncToActor>;
        using MarkedDirtyBy = FTag_Transform_Updated;
        // Broadcasts UUtils_Signal_TransformUpdate from cached FFragment_Transform — pump would re-broadcast within the same frame.
        // FTag_Transform_Updated is end-of-frame cleanup (FProcessor_Transform_Cleanup), not consumed by this body.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Signal_TransformUpdate& InSignal,
            const FFragment_Transform& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_Cleanup : public TProcessorBase<FProcessor_Transform_Cleanup>
    {
    public:
        using RunAfter = TDepList<ck::FGroup_Physics>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_Transform_Cleanup(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Restore-rebase provenance crosses the PrePhysics -> PostPhysics partition: restored actor transforms publish
    // in Transform_SyncFrom, SceneNode propagates them, and static runtime representations consume them in Overlap.
    // A separate end-of-frame cleanup keeps that marker alive through the complete cross-partition transaction.
    class CKECSEXT_API FProcessor_Transform_RestoreRebaseCleanup
        : public TProcessorBase<FProcessor_Transform_RestoreRebaseCleanup>
    {
    public:
        using RunAfter = TDepList<ck::FGroup_Overlap>;
        static constexpr auto TickGroup = TG_PostPhysics;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_Transform_RestoreRebaseCleanup(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_Replicate : public ck_exp::TProcessor<
            FProcessor_Transform_Replicate,
            FCk_Handle_Transform,
            ck::TReadWrite<FFragment_Transform>,
            ck::TReadOnly<FFragment_ContainerRef_Location>,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InCurrent,
            const FFragment_ContainerRef_Location& InLocRef) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_InterpolateToGoal_Location : public ck_exp::TProcessor<
            FProcessor_Transform_InterpolateToGoal_Location,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_TransformInterpolation_Params>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadWrite<FFragment_TransformInterpolation_NewGoal_Location>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        using RunAfter = TDepList<FProcessor_Transform_SyncFromMeshSocket>;
        using MarkedDirtyBy = FFragment_TransformInterpolation_NewGoal_Location;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Location& InGoal) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_InterpolateToGoal_Rotation : public ck_exp::TProcessor<
            FProcessor_Transform_InterpolateToGoal_Rotation,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_TransformInterpolation_Params>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadWrite<FFragment_TransformInterpolation_NewGoal_Rotation>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        using RunAfter = TDepList<FProcessor_Transform_InterpolateToGoal_Location>;
        using MarkedDirtyBy = FFragment_TransformInterpolation_NewGoal_Rotation;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Rotation& InGoal) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
