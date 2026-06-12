#include "CkProjectile_Processor.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkProjectile/Homing/CkHoming_ProNav.h"
#include "CkVariables/CkUnrealVariables_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Projectile_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Projectile_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Projectile_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp)
        -> void
    {
        if (InIntegratorComp.Get_DistanceOffset().IsNearlyZero())
        { return; }

        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(InHandle);

        UCk_Utils_Transform_UE::Request_AddLocationOffset
        (
            HandleTransform,
            FCk_Request_Transform_AddLocationOffset{InIntegratorComp.Get_DistanceOffset()}.Set_LocalWorld(ECk_LocalWorld::World)
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Projectile_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Projectile_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Projectile_Requests& InRequests)
        -> void
    {
        DoHandleRequest(InDeltaT, InHandle, InRequests.Get_Request());

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_Projectile_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_Projectile_Requests::RequestType& InRequest)
        -> void
    {
        const auto& TargetVel = InRequest.Get_TargetVel();
        const auto& TargetLoc = [&]() -> FVector
        {
            switch (const auto& AimAheadPolicy = InRequest.Get_AimAheadPolicy())
            {
                case ECk_Projectile_AimAhead_Policy::AimAhead:
                {
                    return InRequest.Get_TargetLoc();
                }
                case ECk_Projectile_AimAhead_Policy::AimAheadPlusOneFrame:
                {
                    return InRequest.Get_TargetLoc() + (TargetVel * InDeltaT.Get_Seconds());
                }
                default:
                {
                    CK_INVALID_ENUM(AimAheadPolicy);
                    return InRequest.Get_TargetLoc();
                }
            }
        }();

        const auto& OptionalPayload = UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag, ECk_Recursion::NotRecursive, IgnoreSucceededFailed);

        const auto FiringSolution = pronav::Compute_FiringSolution(
            InRequest.Get_ProjectileStartingLoc(),
            InRequest.Get_ShooterVel(),
            TargetLoc,
            TargetVel,
            InRequest.Get_ProjectileSpeed(),
            InRequest.Get_InterceptPreference());

        UUtils_Signal_Projectile_OnAimAheadCalculated::Broadcast(
            InHandle, MakePayload(FiringSolution.Get_Result(), FiringSolution.Get_ImpactLocation(),
                FiringSolution.Get_TimeToImpact(), OptionalPayload));
    }
}

// --------------------------------------------------------------------------------------------------------------------