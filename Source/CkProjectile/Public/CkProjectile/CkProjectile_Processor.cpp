#include "CkProjectile_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"
#include "CkVariables/CkUnrealVariables_Utils.h"

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
        // Theory: https://www.gamedeveloper.com/programming/shooting-a-moving-target

        const auto& ProjectileStartingLoc = InRequest.Get_ProjectileStartingLoc();
        const auto& ProjectileSpeed = InRequest.Get_ProjectileSpeed();

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

        const auto& ToTarget = TargetLoc - ProjectileStartingLoc;
        const auto& ScaledProjectileSpeed = ProjectileSpeed * InDeltaT.Get_Seconds();

        const auto& A = FVector::DotProduct(TargetVel, TargetVel) - (ScaledProjectileSpeed * ScaledProjectileSpeed);
        const auto& B = FVector::DotProduct(TargetVel, ToTarget) * 2.0f;
        const auto& C = FVector::DotProduct(ToTarget, ToTarget);

        const auto& OptionalPayload = UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag, IgnoreSucceededFailed);

        // If the discriminant is negative, then there is no solution
        if (const auto& Discriminant = (B * B) - (4.0f * A * C);
            Discriminant > 0.0f)
        {
            // Calculate the time the bullet will collide if it's possible to hit the target.
            const auto& HitDeltaT = (2.0f * C) / (FMath::Sqrt(Discriminant) - B);
            const auto& TargetAimPoint = TargetLoc + (TargetVel * HitDeltaT);

            UUtils_Signal_Projectile_OnAimAheadCalculated::Broadcast(
                InHandle, MakePayload(ECk_SucceededFailed::Succeeded, TargetAimPoint, OptionalPayload));

            return;
        }

        UUtils_Signal_Projectile_OnAimAheadCalculated::Broadcast(
                InHandle, MakePayload(ECk_SucceededFailed::Failed, FVector::ZeroVector, OptionalPayload));
    }
}

// --------------------------------------------------------------------------------------------------------------------
