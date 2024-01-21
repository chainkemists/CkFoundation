#pragma once

#include "CkTransform_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSBASICS_API FProcessor_Transform_HandleRequests : public TProcessor<
            FProcessor_Transform_HandleRequests,
            FFragment_Transform_Current,
            FFragment_Transform_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Transform_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            FFragment_Transform_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetScale& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSBASICS_API FProcessor_Transform_Actor : public TProcessor<
            FProcessor_Transform_Actor,
            FFragment_OwningActor_Current,
            FFragment_Transform_Current,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActor,
            const FFragment_Transform_Current& InComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSBASICS_API FProcessor_Transform_FireSignals : public TProcessor<
            FProcessor_Transform_FireSignals,
            FFragment_Signal_TransformUpdate,
            FFragment_Transform_Current,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Signal_TransformUpdate& InSignal,
            const FFragment_Transform_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSBASICS_API FProcessor_Transform_Replicate : public TProcessor<
            FProcessor_Transform_Replicate,
            FFragment_Transform_Current,
            TObjectPtr<UCk_Fragment_Transform_Rep>,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSBASICS_API FProcessor_Transform_InterpolateToGoal_Location : public TProcessor<
            FProcessor_Transform_InterpolateToGoal_Location,
            FFragment_Transform_Params,
            FFragment_Transform_Current,
            FFragment_Transform_NewGoal_Location,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Transform_NewGoal_Location;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Params& InParams,
            FFragment_Transform_Current& InCurrent,
            FFragment_Transform_NewGoal_Location& InGoal) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSBASICS_API FProcessor_Transform_InterpolateToGoal_Rotation : public TProcessor<
            FProcessor_Transform_InterpolateToGoal_Rotation,
            FFragment_Transform_Params,
            FFragment_Transform_Current,
            FFragment_Transform_NewGoal_Rotation,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Transform_NewGoal_Rotation;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform_Params& InParams,
            FFragment_Transform_Current& InCurrent,
            FFragment_Transform_NewGoal_Rotation& InGoal) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
