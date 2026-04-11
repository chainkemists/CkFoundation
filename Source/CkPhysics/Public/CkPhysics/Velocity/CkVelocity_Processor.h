#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_PredictedVelocity_Update;

    class CKPHYSICS_API FProcessor_Velocity_Setup : public TProcessor<
            FProcessor_Velocity_Setup,
            ck::TReadOnly<FFragment_Velocity_Params>,
            ck::TReadWrite<FFragment_Velocity_Current>,
            FTag_Velocity_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkVelocityModifier_HandleRequests>;
        using MarkedDirtyBy = FTag_Velocity_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Params& InParams,
            FFragment_Velocity_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_Velocity_Clamp : public TProcessor<
            FProcessor_Velocity_Clamp,
            ck::TReadWrite<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_Velocity_MinMax>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_PredictedVelocity_Update>;
        using MarkedDirtyBy = FFragment_Velocity_MinMax;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Velocity_Current& InCurrent,
            const FFragment_Velocity_MinMax& InMinMax) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_VelocityModifier_Setup : public TProcessor<
            FProcessor_VelocityModifier_Setup,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_Velocity_Target>,
            FTag_VelocityModifier,
            FTag_VelocityModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_Velocity_Setup>;
        using MarkedDirtyBy = FTag_VelocityModifier_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FFragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_VelocityModifier_EndPlay : public TProcessor<
            FProcessor_VelocityModifier_EndPlay,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_Velocity_Target>,
            FTag_VelocityModifier,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FFragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkVelocityModifier_Setup : public TProcessor<
            FProcessor_BulkVelocityModifier_Setup,
            ck::TReadOnly<FFragment_BulkVelocityModifier_Params>,
            FTag_BulkVelocityModifier_GlobalScope,
            FTag_BulkVelocityModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FTag_BulkVelocityModifier_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkVelocityModifier_AddNewTargets : public TProcessor<
            FProcessor_BulkVelocityModifier_AddNewTargets,
            ck::TReadOnly<FFragment_Velocity_Params>,
            ck::TReadOnly<FFragment_RecordOfVelocityChannels>,
            FTag_EntityJustCreated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkVelocityModifier_Setup>;
        using MarkedDirtyBy = FTag_EntityJustCreated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Params& InParams,
            const FFragment_RecordOfVelocityChannels& InVelocityChannels) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkVelocityModifier_HandleRequests : public TProcessor<
            FProcessor_BulkVelocityModifier_HandleRequests,
            ck::TReadOnly<FFragment_BulkVelocityModifier_Params>,
            ck::TReadWrite<FFragment_BulkVelocityModifier_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkVelocityModifier_AddNewTargets>;
        using MarkedDirtyBy = FFragment_BulkVelocityModifier_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            FFragment_BulkVelocityModifier_Requests& InRequests) const -> void;

    private:
        static auto DoHandleRequest(
            const HandleType& InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            const FCk_Request_BulkVelocityModifier_AddTarget& InRequest) -> void;

        static auto DoHandleRequest(
            const HandleType&                                    InHandle,
            const FFragment_BulkVelocityModifier_Params&         InParams,
            const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_Velocity_Replicate : public TProcessor<
            FProcessor_Velocity_Replicate,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_ContainerRef_Velocity>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InCurrent,
            const FFragment_ContainerRef_Velocity& InContainerRef) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
