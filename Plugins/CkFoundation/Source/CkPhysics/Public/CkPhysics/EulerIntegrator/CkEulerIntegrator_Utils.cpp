#include "CkEulerIntegrator_Utils.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Start(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_EulerIntegrator_Current>();
    InHandle.Add<ck::FCk_Tag_EulerIntegrator_Update>();
    InHandle.Add<ck::FCk_Tag_EulerIntegrator_Setup>();
}

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Stop(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Remove<ck::FCk_Fragment_EulerIntegrator_Current>();
    InHandle.Remove<ck::FCk_Tag_EulerIntegrator_Update>();
}

// --------------------------------------------------------------------------------------------------------------------
