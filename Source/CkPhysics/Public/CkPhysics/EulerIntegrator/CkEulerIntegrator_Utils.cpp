#include "CkEulerIntegrator_Utils.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Start(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FFragment_EulerIntegrator_Current>();
    InHandle.Add<ck::FTag_EulerIntegrator_NeedsUpdate>();
    InHandle.Add<ck::FTag_EulerIntegrator_DoOnePredictiveUpdate>();
}

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Stop(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Remove<ck::FFragment_EulerIntegrator_Current>();
    InHandle.Remove<ck::FTag_EulerIntegrator_NeedsUpdate>();
}

// --------------------------------------------------------------------------------------------------------------------
