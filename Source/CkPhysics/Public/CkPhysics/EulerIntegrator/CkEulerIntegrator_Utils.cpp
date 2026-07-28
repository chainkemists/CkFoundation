#include "CkEulerIntegrator_Utils.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Start(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    InHandle.Add<ck::FFragment_EulerIntegrator_Current>();
    InHandle.Add<ck::FTag_EulerIntegrator_NeedsUpdate>();
    InHandle.Add<ck::FTag_EulerIntegrator_DoOnePredictiveUpdate>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);
}

auto
    UCk_Utils_EulerIntegrator_UE::
    Request_Stop(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    InHandle.Remove<ck::FFragment_EulerIntegrator_Current>();
    InHandle.Remove<ck::FTag_EulerIntegrator_NeedsUpdate>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------
