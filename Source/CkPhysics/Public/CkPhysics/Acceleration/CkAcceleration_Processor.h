#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_VelocityModifier_Setup;

    class CKPHYSICS_API FProcessor_Acceleration_Setup : public TProcessor<
            FProcessor_Acceleration_Setup,
            ck::TReadOnly<FFragment_Acceleration_Params>,
            ck::TReadWrite<FFragment_Acceleration_Current>,
            FTag_Acceleration_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkAccelerationModifier_HandleRequests>;
        using MarkedDirtyBy = FTag_Acceleration_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            FFragment_Acceleration_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_AccelerationModifier_Setup : public TProcessor<
            FProcessor_AccelerationModifier_Setup,
            ck::TReadOnly<FFragment_Acceleration_Current>,
            ck::TReadOnly<FFragment_Acceleration_Target>,
            FTag_AccelerationModifier,
            FTag_AccelerationModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_Acceleration_Setup>;
        using MarkedDirtyBy = FTag_AccelerationModifier_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_AccelerationModifier_EndPlay : public TProcessor<
            FProcessor_AccelerationModifier_EndPlay,
            ck::TReadOnly<FFragment_Acceleration_Current>,
            ck::TReadOnly<FFragment_Acceleration_Target>,
            FTag_AccelerationModifier,
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
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkAccelerationModifier_Setup : public TProcessor<
            FProcessor_BulkAccelerationModifier_Setup,
            ck::TReadOnly<FFragment_BulkAccelerationModifier_Params>,
            FTag_BulkAccelerationModifier_GlobalScope,
            FTag_BulkAccelerationModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_VelocityModifier_Setup>;
        using MarkedDirtyBy = FTag_BulkAccelerationModifier_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkAccelerationModifier_AddNewTargets : public TProcessor<
            FProcessor_BulkAccelerationModifier_AddNewTargets,
            ck::TReadOnly<FFragment_Acceleration_Params>,
            ck::TReadOnly<FFragment_RecordOfAccelerationChannels>,
            FTag_EntityJustCreated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkAccelerationModifier_Setup>;
        using MarkedDirtyBy = FTag_EntityJustCreated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            const FFragment_RecordOfAccelerationChannels& InAccelerationChannels) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_BulkAccelerationModifier_HandleRequests : public TProcessor<
            FProcessor_BulkAccelerationModifier_HandleRequests,
            ck::TReadOnly<FFragment_BulkAccelerationModifier_Params>,
            ck::TReadWrite<FFragment_BulkAccelerationModifier_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BulkAccelerationModifier_AddNewTargets>;
        using MarkedDirtyBy = FFragment_BulkAccelerationModifier_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            FFragment_BulkAccelerationModifier_Requests& InRequests) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_Acceleration_Replicate : public TProcessor<
            FProcessor_Acceleration_Replicate,
            ck::TReadOnly<FFragment_Acceleration_Current>,
            ck::TReadOnly<FFragment_ContainerRef_Acceleration>,
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
            const FFragment_Acceleration_Current& InCurrent,
            const FFragment_ContainerRef_Acceleration& InContainerRef) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
