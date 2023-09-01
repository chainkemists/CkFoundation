#include "CkNetTimeSync_Utils.h"

#include "CkNetTimeSync_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NetTimeSync_UE::
    Add_TimeSync_Rep(FCk_Handle InHandle)
    -> void
{
    TryAddReplicatedFragment<UCk_Fragment_TimeSync_Rep>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NetTimeSync_UE::
    Add(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FFragment_TimeSync>();
}

auto
    UCk_Utils_NetTimeSync_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_TimeSync>();
}

auto
    UCk_Utils_NetTimeSync_UE::
    Ensure(FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have [{}]"), InHandle, ctti::nameof_v<ck::FFragment_TimeSync>)
    { return false; }

    return true;
}

auto
    UCk_Utils_NetTimeSync_UE::
    Request_NewTimeSync(
        FCk_Handle InHandle,
        FCk_Request_NetTimeSync_NewSync InRequest) -> void
{
    auto& RequestsFragment = InHandle.AddOrGet<ck::FFragment_TimeSync_Requests>();
    RequestsFragment._TimeSyncRequests.Emplace(InRequest);
}

auto
    UCk_Utils_NetTimeSync_UE::
    Get_RoundTripTime(
        FCk_Handle InHandle)
    -> FCk_Time
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_TimeSync>().Get_RoundTripTime();
}

auto
    UCk_Utils_NetTimeSync_UE::
    Get_Latency(
        FCk_Handle InHandle)
    -> FCk_Time
{
    return FCk_Time{ Get_RoundTripTime(InHandle).Get_Seconds() / 2.0f };
}

auto
    UCk_Utils_NetTimeSync_UE::
    Get_PlayerRoundTripTime(
        APlayerController* InPlayerController,
        FCk_Handle InHandle)
    -> FCk_Time
{
    if (NOT Ensure(InHandle))
    { return {}; }

    const auto ValueFound = InHandle.Get<ck::FFragment_TimeSync>().Get_PlayerRoundTripTimes().Find(InPlayerController);

    CK_ENSURE_IF_NOT(ck::IsValid(ValueFound, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Could not find PlayerController [{}] using Entity [{}]. Possible syncing error OR Player disconnected"),
        InPlayerController, InHandle)
    { return {}; }

    return *ValueFound;
}

auto
    UCk_Utils_NetTimeSync_UE::
    Get_PlayerLatency(
        APlayerController* InPlayerController,
        FCk_Handle InHandle)
    -> FCk_Time
{
    return FCk_Time{ Get_PlayerRoundTripTime(InPlayerController, InHandle).Get_Seconds() / 2.0f };
}

// --------------------------------------------------------------------------------------------------------------------
