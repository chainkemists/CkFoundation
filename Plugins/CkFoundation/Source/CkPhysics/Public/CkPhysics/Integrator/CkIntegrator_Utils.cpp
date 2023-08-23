#include "CkIntegrator_Utils.h"

#include "CkPhysics/Integrator/CkIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Integrator_UE::
    Request_Start(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_Integrator_Current>();
    InHandle.Add<ck::FCk_Tag_Integrator_Update>();
}

auto
    UCk_Utils_Integrator_UE::
    Request_Stop(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Remove<ck::FCk_Fragment_Integrator_Current>();
    InHandle.Remove<ck::FCk_Tag_Integrator_Update>();
}

// --------------------------------------------------------------------------------------------------------------------
