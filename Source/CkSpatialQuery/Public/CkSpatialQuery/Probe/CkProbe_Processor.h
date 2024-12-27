#pragma once

#include "CkProbe_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

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
            const RegistryType& InGameRegistry,
            const RegistryType& InPhysicsRegistry);

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
        static auto
            SensorContactStarted(
                entt::registry& registry,
                entt::entity entity)
                -> void;

    private:
        bool SetBodyToKinematic = true;
        RegistryType _PhysicsRegistry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Probe_UpdateTransform,
            FCk_Handle_Probe,
            FFragment_Probe_Current,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_RequiresSetup;

    public:
        FProcessor_Probe_UpdateTransform(
            const RegistryType& InGameRegistry,
            const RegistryType& InPhysicsRegistry);

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                FFragment_Probe_Current& InCurrent)
                -> void;

    private:
        RegistryType _PhysicsRegistry;
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
                const FCk_Request_Probe_ExampleRequest& InRequest)
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
