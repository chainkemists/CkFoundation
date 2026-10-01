#include "CkGait_Processor.h"
#include "CkGait/Gait/CkGait_Kernel.h"
#include "CkGait/CkGait_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "GameFramework/NavMovementComponent.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Gait_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Gait_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Gait_CancelPendingRequests);

auto
    ck::FProcessor_Gait_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Gait_Tunables& InTunables,
        FFragment_Gait& InGait,
        FFragment_Gait_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    algo::ForEachRequest(RequestsCopy, ck::Visitor([&](const auto& InRequest)
    {
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);
        if (InHandle.Has<FTag_DestroyEntity_Initiate>())
        {
            Result = ECk_Request_OperationResult::Failed_Cancelled;
            return;
        }

        gait::VeryVerbose(TEXT("Handling Gait request on [{}]"), InHandle);
        Result = DoHandleRequest(InHandle, InTunables, InGait, InRequest);
    }), policy::DontResetContainer{});
    if (InRequests._Requests.IsEmpty())
    { InHandle.Remove<MarkedDirtyBy>(); }
}

auto
    ck::FProcessor_Gait_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Gait_Tunables& InTunables,
        FFragment_Gait& InGait,
        const FCk_Request_Gait_UpdateSpec& InRequest)
    -> ECk_Request_OperationResult
{
    const auto IsValidSpec = gait::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Gait UpdateSpec rejected invalid spec on [{}]"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    InTunables = InRequest.Get_Spec();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Gait_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Gait_Tunables& InTunables,
        FFragment_Gait& InGait,
        const FCk_Request_Gait_EnableDisable& InRequest)
    -> ECk_Request_OperationResult
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InHandle.Try_Remove<FTag_Gait_Disabled>();
            break;
        }

        case ECk_EnableDisable::Disable:
        {
            InHandle.AddOrGet<FTag_Gait_Disabled>();
            break;
        }

        default:
        {
            CK_INVALID_ENUM(InRequest.Get_EnableDisable());
            return ECk_Request_OperationResult::Failed;
        }
    }

    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Gait_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Gait_Tunables& InTunables,
        FFragment_Gait& InGait,
        const FCk_Request_Gait_Reset& InRequest)
    -> ECk_Request_OperationResult
{
    // The landing counter and the last impact speed are untouched: consumers diff the counter against their own copy,
    // and a reset must not replay or swallow a landing.
    InGait._Clock = {};
    InGait._LastMotion = gait::Get_RestMotion();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Gait_CancelPendingRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Gait_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    request::FireCancelledForPending(InHandle, RequestsCopy);
}

auto
    ck::FProcessor_Gait_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Gait_Tunables& InTunables,
        FFragment_Gait& InGait)
    -> void
{
    auto Motion = gait::Get_RestMotion();
    if (NOT InHandle.Has<FTag_Gait_Disabled>())
    {
        if (auto* Movement = InTunables.Get_MovementComponent().Get(); ck::IsValid(Movement))
        {
            Motion = FCk_Gait_Motion{
                Movement->Velocity,
                Movement->IsFalling() ? ECk_Gait_Footing::Airborne : ECk_Gait_Footing::Grounded,
                Movement->IsCrouching() ? ECk_Gait_Stance::Crouched : ECk_Gait_Stance::Standing};

            const auto IsVelocityFinite = NOT Motion.Get_Velocity().ContainsNaN();
            CK_ENSURE_IF_NOT(IsVelocityFinite,
                TEXT("Gait [{}] sampled a non-finite velocity [{}]; the gait rests this frame"), InHandle, Movement->Velocity)
            {}
            if (NOT IsVelocityFinite)
            { Motion = gait::Get_RestMotion(); }
        }
        // else: a dead component after a valid Add means its owner went first in teardown; rest, no ensure.
    }

    if (const auto Landing = gait::Detect_Landing(InGait._LastMotion, Motion); Landing.IsSet())
    {
        InGait._LandingCount += 1;
        InGait._LastLandImpactSpeed = Landing.GetValue();
    }

    gait::Step_Clock(InGait._Clock, InTunables, Motion, static_cast<float>(InDeltaT.Get_Seconds()));
    InGait._LastMotion = Motion;
}
