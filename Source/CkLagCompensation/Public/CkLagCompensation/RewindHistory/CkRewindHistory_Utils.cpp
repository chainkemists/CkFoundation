#include "CkRewindHistory_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment.h"
#include "CkLagCompensation/RewindHistory/CkRewindHistory_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::rewind_history_detail
{
    // Defined in CkRewindHistory_Processor.cpp
    auto Get_FrameCapacity(const FFragment_RewindHistory_Params& InParams) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RewindHistory_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_RewindHistory_ParamsData& InParams)
    -> FCk_Handle_RewindHistory
{
    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Cannot Add RewindHistory to Entity [{}] because it does NOT have a Transform"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InParams.Get_HitShapes().IsEmpty(),
        TEXT("Cannot Add RewindHistory to Entity [{}] with NO hit shapes"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_RewindHistory_Params>(InParams);

    auto& Current = InHandle.Add<ck::FFragment_RewindHistory_Current>();
    Current._Frames.Init(ck::rewind_history_detail::Get_FrameCapacity(InParams));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RewindHistory_UE, FCk_Handle_RewindHistory,
    ck::FFragment_RewindHistory_Params, ck::FFragment_RewindHistory_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RewindHistory_UE::
    Get_RecordedFrameCount(
        const FCk_Handle_RewindHistory& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_RewindHistory_Current>().Get_Frames().Get_Count();
}

auto
    UCk_Utils_RewindHistory_UE::
    Get_NewestFrameTime(
        const FCk_Handle_RewindHistory& InHandle)
    -> FCk_Time
{
    const auto& Frames = InHandle.Get<ck::FFragment_RewindHistory_Current>().Get_Frames();

    if (Frames.Get_Count() == 0)
    { return FCk_Time::ZeroSecond(); }

    return Frames.Get_FrameAt(Frames.Get_Count() - 1).Get_WorldTime();
}

auto
    UCk_Utils_RewindHistory_UE::
    Get_OldestFrameTime(
        const FCk_Handle_RewindHistory& InHandle)
    -> FCk_Time
{
    const auto& Frames = InHandle.Get<ck::FFragment_RewindHistory_Current>().Get_Frames();

    if (Frames.Get_Count() == 0)
    { return FCk_Time::ZeroSecond(); }

    return Frames.Get_FrameAt(0).Get_WorldTime();
}

auto
    UCk_Utils_RewindHistory_UE::
    Get_InterpolatedSnapshotsAtTime(
        const FCk_Handle_RewindHistory& InHandle,
        FCk_Time InWorldTime)
    -> TArray<FCk_LagComp_HitShapeSnapshot>
{
    return ck::lag_comp::Get_InterpolatedSnapshots(
        InHandle.Get<ck::FFragment_RewindHistory_Current>().Get_Frames(), InWorldTime);
}

auto
    UCk_Utils_RewindHistory_UE::
    Sweep_AgainstHistory(
        const FCk_Handle_RewindHistory& InHandle,
        const FVector& InSegmentStart,
        const FVector& InSegmentEnd,
        FCk_Time InTimeStart,
        FCk_Time InTimeEnd,
        float InSweepRadius,
        FCk_LagComp_RewindHit& OutHit)
    -> bool
{
    const auto Hit = ck::lag_comp::Sweep_SegmentVsHistory(
        InHandle.Get<ck::FFragment_RewindHistory_Current>().Get_Frames(),
        InSegmentStart, InSegmentEnd, InTimeStart, InTimeEnd, InSweepRadius);

    if (NOT Hit.IsSet())
    { return false; }

    OutHit = *Hit;
    OutHit.Set_TargetEntity(InHandle);

    return true;
}

auto
    UCk_Utils_RewindHistory_UE::
    Request_ForceRecordFrame(
        FCk_Handle_RewindHistory& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RewindHistory
{
    InHandle.AddOrGet<ck::FTag_RewindHistory_ForceRecord>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
