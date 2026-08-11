#include "CkIntentDebugHistory_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkIntent/CkIntentSampler_Utils.h"
#include "CkIntent/CkIntent_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_IntentDebugHistory_UE,
    FCk_Handle_IntentDebugHistory,
    ck::FFragment_IntentDebugHistory_Params,
    ck::FFragment_IntentDebugHistory_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentDebugHistory_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IntentDebugHistory_ParamsData& InParams)
    -> FCk_Handle_IntentDebugHistory
{
#if UE_BUILD_SHIPPING
    // Compiled out by design — a debug recording must cost a shipped game nothing (see the class doc).
    return {};
#else
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an IntentDebugHistory onto it"), InHandle)
    {}
    if (NOT HandleIsValid)
    { return {}; }

    const auto HandleHasSampler = UCk_Utils_IntentSampler_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HandleHasSampler,
        TEXT("Add: Handle [{}] carries no IntentSampler — a debug history records rows out of the sampler's "
             "ring, so there is nothing here to record"), InHandle)
    {}
    if (NOT HandleHasSampler)
    { return {}; }

    const auto EntityHasNoHistory = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityHasNoHistory,
        TEXT("Add: Handle [{}] already carries an IntentDebugHistory — one recording per source; retune its "
             "capacity instead of composing a second"), InHandle)
    {}
    if (NOT EntityHasNoHistory)
    { return {}; }

    const auto CapacityIsPositive = InParams.Get_Capacity() > 0;
    CK_ENSURE_IF_NOT(CapacityIsPositive,
        TEXT("IntentDebugHistory declaration on [{}] asks for a capacity of [{}] frames — a recording that "
             "retains nothing cannot be read back"), InHandle, InParams.Get_Capacity())
    {}
    if (NOT CapacityIsPositive)
    { return {}; }

    InHandle.Add<ck::FFragment_IntentDebugHistory_Params>(InParams);
    auto& Current = InHandle.Add<ck::FFragment_IntentDebugHistory_Current>();
    Current._Capacity = InParams.Get_Capacity();

    ck::intent::Verbose
    (
        TEXT("IntentDebugHistory composed onto InputSource [{}] retaining [{}] frames"),
        InHandle, InParams.Get_Capacity()
    );

    return CastChecked(InHandle);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentDebugHistory_UE::
    Get_Capacity(
        const FCk_Handle_IntentDebugHistory& InHistory)
    -> int32
{
    return InHistory.Get<ck::FFragment_IntentDebugHistory_Current>().Get_Capacity();
}

auto
    UCk_Utils_IntentDebugHistory_UE::
    Get_FrameCount(
        const FCk_Handle_IntentDebugHistory& InHistory)
    -> int32
{
    return InHistory.Get<ck::FFragment_IntentDebugHistory_Current>().Get_Rows().Num();
}

auto
    UCk_Utils_IntentDebugHistory_UE::
    TryGet_FrameAtOffset(
        const FCk_Handle_IntentDebugHistory& InHistory,
        int32 InOffset)
    -> FCk_Intent_FrameRecord
{
    const auto& Rows = InHistory.Get<ck::FFragment_IntentDebugHistory_Current>().Get_Rows();

    if (InOffset < 0 || InOffset >= Rows.Num())
    { return {}; }

    return Rows[Rows.Num() - 1 - InOffset];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentDebugHistory_UE::
    Request_SetCapacity(
        FCk_Handle_IntentDebugHistory& InHistory,
        const FCk_Request_IntentDebugHistory_SetCapacity& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IntentDebugHistory
{
    const auto HistoryIsValid = ck::IsValid(InHistory);
    CK_ENSURE_IF_NOT(HistoryIsValid,
        TEXT("Request_SetCapacity: invalid IntentDebugHistory Handle [{}]"), InHistory)
    {}
    if (NOT HistoryIsValid)
    {
        InDelegate.ExecuteIfBound(InHistory, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHistory;
    }

    const auto CapacityIsPositive = InRequest.Get_Capacity() > 0;
    CK_ENSURE_IF_NOT(CapacityIsPositive,
        TEXT("Request_SetCapacity on [{}] asks for a capacity of [{}] frames — a recording that retains nothing "
             "cannot be read back"), InHistory, InRequest.Get_Capacity())
    {}
    if (NOT CapacityIsPositive)
    {
        InDelegate.ExecuteIfBound(InHistory, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHistory;
    }

    auto& Current = InHistory.Get<ck::FFragment_IntentDebugHistory_Current>();
    Current._Capacity = InRequest.Get_Capacity();

    if (const auto Excess = Current._Rows.Num() - Current._Capacity; Excess > 0)
    { Current._Rows.RemoveAt(0, Excess); }

    InDelegate.ExecuteIfBound(InHistory, ECk_Request_OperationResult::Succeeded);
    return InHistory;
}

// --------------------------------------------------------------------------------------------------------------------
