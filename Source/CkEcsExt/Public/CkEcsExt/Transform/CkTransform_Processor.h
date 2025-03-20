#pragma once

#include "CkTransform_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSEXT_API FProcessor_Transform_SyncFromActor : public ck_exp::TProcessor<
            FProcessor_Transform_SyncFromActor,
            FCk_Handle_Transform,
            FFragment_Transform,
            FFragment_Transform_RootComponent,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            const FFragment_Transform_RootComponent& InTransformRootComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_SyncFromSkeletalMeshSocket : public ck_exp::TProcessor<
            FProcessor_Transform_SyncFromSkeletalMeshSocket,
            FCk_Handle_Transform,
            FFragment_Transform,
            FFragment_Transform_SkeletalMeshSocket,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            const FFragment_Transform_SkeletalMeshSocket& InSocket) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_SyncFromStaticMeshSocket : public ck_exp::TProcessor<
            FProcessor_Transform_SyncFromStaticMeshSocket,
            FCk_Handle_Transform,
            FFragment_Transform,
            FFragment_Transform_StaticMeshSocket,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            const FFragment_Transform_StaticMeshSocket& InSocket) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Transform_HandleRequests,
            FCk_Handle_Transform,
            FFragment_Transform,
            FFragment_Transform_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Transform_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InComp,
            FFragment_Transform_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Transform& InComp,
            const FCk_Request_Transform_SetScale& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_SyncToActor : public ck_exp::TProcessor<
            FProcessor_Transform_SyncToActor,
            FCk_Handle_Transform,
            FFragment_Transform_RootComponent,
            FFragment_Transform,
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
            const FFragment_Transform_RootComponent& InTransformRootComp,
            const FFragment_Transform& InComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_FireSignals : public ck_exp::TProcessor<
            FProcessor_Transform_FireSignals,
            FCk_Handle_Transform,
            FFragment_Signal_TransformUpdate,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

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
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_Transform_Cleanup(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_Replicate : public ck_exp::TProcessor<
            FProcessor_Transform_Replicate,
            FCk_Handle_Transform,
            FFragment_Transform,
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
            FFragment_Transform& InCurrent,
            const TObjectPtr<UCk_Fragment_Transform_Rep>& InComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_InterpolateToGoal_Location : public ck_exp::TProcessor<
            FProcessor_Transform_InterpolateToGoal_Location,
            FCk_Handle_Transform,
            FFragment_TransformInterpolation_Params,
            FFragment_Transform,
            FFragment_TransformInterpolation_NewGoal_Location,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_TransformInterpolation_NewGoal_Location;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Location& InGoal) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_Transform_InterpolateToGoal_Rotation : public ck_exp::TProcessor<
            FProcessor_Transform_InterpolateToGoal_Rotation,
            FCk_Handle_Transform,
            FFragment_TransformInterpolation_Params,
            FFragment_Transform,
            FFragment_TransformInterpolation_NewGoal_Rotation,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_TransformInterpolation_NewGoal_Rotation;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TransformInterpolation_Params& InParams,
            const FFragment_Transform& InCurrent,
            FFragment_TransformInterpolation_NewGoal_Rotation& InGoal) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
