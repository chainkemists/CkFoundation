#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"
#include "CkProjectile/CkProjectile_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Projectile_HandleRequests;

    class CKPROJECTILE_API FProcessor_Projectile_Update : public TProcessor<
            FProcessor_Projectile_Update,
            ck::TReadOnly<FFragment_EulerIntegrator_Current>,
            FTag_EulerIntegrator_NeedsUpdate,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_Projectile_HandleRequests>;
        using MarkedDirtyBy = FTag_EulerIntegrator_NeedsUpdate;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROJECTILE_API FProcessor_Projectile_HandleRequests : public TProcessor<
            FProcessor_Projectile_HandleRequests,
            ck::TReadOnly<FFragment_Projectile_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FFragment_Projectile_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Projectile_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_Projectile_Requests::RequestType& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
