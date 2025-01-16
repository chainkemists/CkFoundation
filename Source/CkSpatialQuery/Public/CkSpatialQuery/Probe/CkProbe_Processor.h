#pragma once

#include "CkProbe_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class PhysicsSystem;
}

namespace ck
{
    class CKSPATIALQUERY_API FProcessor_Probe_Setup : public ck_exp::TProcessor<
            FProcessor_Probe_Setup,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_RequiresSetup;

    public:
        FProcessor_Probe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
            DoTick(
                TimeType InDeltaT)
                -> void;

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent)
                -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Probe_UpdateTransform,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_RequiresSetup;

    public:
        FProcessor_Probe_UpdateTransform(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent)
                -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Probe_HandleRequests,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FFragment_Probe_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Probe_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
            DoTick(
                TimeType InDeltaT)
                -> void;

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent,
                FFragment_Probe_Requests& InRequestsComp) const
                -> void;

    private:
        static auto
            DoHandleRequest(
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent,
                const FCk_Request_Probe_BeginOverlap& InRequest)
                -> void;

        static auto
            DoHandleRequest(
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent,
                const FCk_Request_Probe_EndOverlap& InRequest)
                -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_Teardown : public ck_exp::TProcessor<
            FProcessor_Probe_Teardown,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                const FFragment_Probe_Params& InParams,
                FFragment_Probe_Current& InCurrent) const
                -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_Replicate : public ck_exp::TProcessor<
            FProcessor_Probe_Replicate,
            FCk_Handle_Probe,
            FFragment_Probe_Current,
            TObjectPtr<UCk_Fragment_Probe_Rep>,
            FTag_Probe_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                FFragment_Probe_Current& InCurrent,
                const TObjectPtr<UCk_Fragment_Probe_Rep>& InRepComp) const
                -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
