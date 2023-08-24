#include "CkProjectile_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/Integrator/CkIntegrator_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

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

    UCk_Utils_Integrator_UE::Request_Start(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
