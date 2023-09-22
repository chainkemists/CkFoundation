#include "CkProjectile_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Projectile_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp) const
        -> void
    {
        if (InIntegratorComp.Get_DistanceOffset().IsNearlyZero())
        { return; }

        UCk_Utils_Transform_UE::Request_AddLocationOffset
        (
            InHandle,
            FCk_Request_Transform_AddLocationOffset{InIntegratorComp.Get_DistanceOffset()}.Set_LocalWorld(ECk_LocalWorld::World)
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Projectile_AutoReorient::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocityCurrent,
            const FFragment_Transform_Current& InTransformCurrent) const
        -> void
    {
        const auto& VelocityDir = InVelocityCurrent.Get_CurrentVelocity().GetSafeNormal();

        if (VelocityDir.IsZero())
        { return; }

        const auto& RotationMatrix = FRotationMatrix::MakeFromX(VelocityDir);

        const auto& DesiredRotation = RotationMatrix.Rotator();
        const auto& CurrentRotation = InTransformCurrent.Get_Transform().Rotator();
        const auto& RotationOffset = (DesiredRotation - CurrentRotation).GetNormalized();

        // TODO: Lerp/Smooth reorient ?

        UCk_Utils_Transform_UE::Request_AddRotationOffset(InHandle, FCk_Request_Transform_AddRotationOffset{RotationOffset});
    }
}

// --------------------------------------------------------------------------------------------------------------------
