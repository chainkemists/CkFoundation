#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_Acceleration_Setup : public TProcessor<
            FProcessor_Acceleration_Setup,
            FFragment_Acceleration_Params,
            FFragment_Acceleration_Current,
            FTag_Acceleration_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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
            FFragment_Acceleration_Current,
            FFragment_Acceleration_Target,
            FTag_AccelerationModifier,
            FTag_AccelerationModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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

    class CKPHYSICS_API FProcessor_AccelerationModifier_Teardown : public TProcessor<
            FProcessor_AccelerationModifier_Teardown,
            FFragment_Acceleration_Current,
            FFragment_Acceleration_Target,
            FTag_AccelerationModifier,
            CK_IF_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = CK_IF_PENDING_KILL;

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
            FFragment_BulkAccelerationModifier_Params,
            FTag_BulkAccelerationModifier_GlobalScope,
            FTag_BulkAccelerationModifier_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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
            FFragment_Acceleration_Params,
            FFragment_RecordOfAccelerationChannels,
            FTag_EntityJustCreated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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
            FFragment_BulkAccelerationModifier_Params,
            FFragment_BulkAccelerationModifier_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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
            FFragment_Acceleration_Current,
            TObjectPtr<UCk_Fragment_Acceleration_Rep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Acceleration_Rep>& InVelRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
