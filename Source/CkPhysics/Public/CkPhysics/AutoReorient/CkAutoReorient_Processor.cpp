#include "CkAutoReorient_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AutoReorient_OrientTowardsVelocity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocityCurrent,
            const FFragment_Transform& InTransformCurrent,
            const FFragment_AutoReorient_Params& InAutoReorientParams) const
        -> void
    {
        const auto& VelocityDir = InVelocityCurrent.Get_CurrentVelocity().GetSafeNormal();

        if (VelocityDir.IsZero())
        { return; }

        const auto& RotationMatrix = FRotationMatrix::MakeFromX(VelocityDir);

        const auto& DesiredRotation = RotationMatrix.Rotator();
        const auto& CurrentRotation = InTransformCurrent.Get_Transform().Rotator();
        const auto& RotationOffset = (DesiredRotation - CurrentRotation).GetNormalized();

        // TODO: Lerp/Smooth reorient

        UCk_Utils_Transform_UE::Request_AddRotationOffset(InHandle, FCk_Request_Transform_AddRotationOffset{RotationOffset});
    }
}

// --------------------------------------------------------------------------------------------------------------------