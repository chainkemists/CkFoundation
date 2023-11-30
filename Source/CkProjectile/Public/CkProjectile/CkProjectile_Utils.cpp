#include "CkProjectile_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Utils.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Utils.h"
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
    UCk_Utils_AutoReorient_UE::Add(InHandle, InParams.Get_AutoReorientParams());

    UCk_Utils_EulerIntegrator_UE::Request_Start(InHandle);
    UCk_Utils_AutoReorient_UE::Request_Start(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
