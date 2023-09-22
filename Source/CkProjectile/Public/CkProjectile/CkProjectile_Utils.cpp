#include "CkProjectile_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"
#include "CkProjectile/CkProjectile_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Projectile_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Projectile_ParamsData& InParams)
    -> void
{
    UCk_Utils_Velocity_UE::Add(InHandle, InParams.Get_VelocityParams());
    UCk_Utils_Acceleration_UE::Add(InHandle, InParams.Get_AccelerationParams());

    UCk_Utils_EulerIntegrator_UE::Request_Start(InHandle);

    if (InParams.Get_ReorientPolicy() == ECk_Projectile_ReorientPolicy::OrientTowardsVelocity)
    {
        InHandle.Add<ck::FTag_Projectile_AutoReorient>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
