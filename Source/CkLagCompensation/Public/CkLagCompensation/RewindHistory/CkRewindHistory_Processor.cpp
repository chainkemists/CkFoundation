#include "CkRewindHistory_Processor.h"

#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <DrawDebugHelpers.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_RewindHistory_Record);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::rewind_history_cvars
{
    static TAutoConsoleVariable<float> CVarRecordInterval(
        TEXT("Ck.LagComp.RecordInterval"),
        0.03f,
        TEXT("Seconds between recorded hitbox frames for lag compensation (per RewindHistory entity, ")
        TEXT("unless the entity's params override it)."),
        ECVF_Default);

    static TAutoConsoleVariable<float> CVarRetainSeconds(
        TEXT("Ck.LagComp.RetainSeconds"),
        0.6f,
        TEXT("How far back hitbox history is kept, in seconds. Should comfortably exceed the highest ")
        TEXT("ping you intend to compensate."),
        ECVF_Default);

    static TAutoConsoleVariable<bool> CVarDebugDraw(
        TEXT("Ck.LagComp.DebugDraw"),
        false,
        TEXT("Draw the OLDEST recorded hitbox frame of every RewindHistory entity — visualizes how ")
        TEXT("far behind the present the rewind window reaches."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::rewind_history_detail
{
    auto
    Get_EffectiveRecordInterval(
        const FFragment_RewindHistory_Params& InParams) -> FCk_Time
    {
        if (InParams.Get_RecordInterval() > FCk_Time::ZeroSecond())
        { return InParams.Get_RecordInterval(); }

        return FCk_Time{static_cast<double>(rewind_history_cvars::CVarRecordInterval.GetValueOnGameThread())};
    }

    auto
    Get_EffectiveRetentionPeriod(
        const FFragment_RewindHistory_Params& InParams) -> FCk_Time
    {
        if (InParams.Get_RetentionPeriod() > FCk_Time::ZeroSecond())
        { return InParams.Get_RetentionPeriod(); }

        return FCk_Time{static_cast<double>(rewind_history_cvars::CVarRetainSeconds.GetValueOnGameThread())};
    }

    auto
    Get_FrameCapacity(
        const FFragment_RewindHistory_Params& InParams) -> int32
    {
        const auto Interval = Get_EffectiveRecordInterval(InParams).Get_Seconds();
        const auto Retention = Get_EffectiveRetentionPeriod(InParams).Get_Seconds();

        return FMath::CeilToInt32(Retention / FMath::Max(Interval, 0.001)) + 2;
    }

#if !UE_BUILD_SHIPPING
    static auto
    DoDraw_OldestFrame(
        const UWorld* InWorld,
        const FCk_LagComp_FrameBuffer& InFrames,
        FCk_Time InDuration) -> void
    {
        if (InFrames.Get_Count() == 0 || ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        constexpr auto DepthPriority = 0;
        constexpr auto PersistentLines = false;
        const auto Duration = static_cast<float>(InDuration.Get_Seconds());

        for (const auto& Snapshot : InFrames.Get_FrameAt(0).Get_Snapshots())
        {
            const auto& ShapeTM = Snapshot.Get_WorldTransform();
            const auto& Shape = Snapshot.Get_Shape();

            switch (Shape.Get_ShapeType())
            {
                case ECk_Shape_Type::Sphere:
                {
                    DrawDebugSphere(InWorld, ShapeTM.GetLocation(), Shape.Get_Sphere().Get_Radius(),
                        16, FColor::Black, PersistentLines, Duration, DepthPriority);
                    break;
                }
                case ECk_Shape_Type::Capsule:
                case ECk_Shape_Type::Cylinder:
                {
                    const auto HalfHeight = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                        ? Shape.Get_Capsule().Get_HalfHeight()
                        : Shape.Get_Cylinder().Get_HalfHeight();
                    const auto Radius = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                        ? Shape.Get_Capsule().Get_Radius()
                        : Shape.Get_Cylinder().Get_Radius();

                    DrawDebugCapsule(InWorld, ShapeTM.GetLocation(), HalfHeight, Radius,
                        ShapeTM.GetRotation(), FColor::Black, PersistentLines, Duration, DepthPriority);
                    break;
                }
                case ECk_Shape_Type::Box:
                {
                    DrawDebugBox(InWorld, ShapeTM.GetLocation(), FVector{Shape.Get_Box().Get_HalfExtents()},
                        ShapeTM.GetRotation(), FColor::Black, PersistentLines, Duration, DepthPriority);
                    break;
                }
                case ECk_Shape_Type::None:
                default:
                {
                    break;
                }
            }
        }
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RewindHistory_Record::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RewindHistory_Params& InParams,
            FFragment_RewindHistory_Current& InCurrent)
        -> void
    {
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto Now = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();

        const auto ForceRecord = InHandle.Has<FTag_RewindHistory_ForceRecord>();
        const auto IntervalElapsed =
            (Now - InCurrent.Get_LastRecordTime()) >= rewind_history_detail::Get_EffectiveRecordInterval(InParams);

        if (NOT ForceRecord && NOT IntervalElapsed && InCurrent.Get_Frames().Get_Count() > 0)
        { return; }

        InHandle.Try_Remove<FTag_RewindHistory_ForceRecord>();

        const auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        const auto EntityTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);

        auto Snapshots = TArray<FCk_LagComp_HitShapeSnapshot>{};
        Snapshots.Reserve(InParams.Get_HitShapes().Num());

        auto Bounds = FBox{ForceInit};

        for (const auto& HitShape : InParams.Get_HitShapes())
        {
            const auto ShapeWorldTM = HitShape.Get_LocalOffset() * EntityTransform;
            const auto& Snapshot = Snapshots.Emplace_GetRef(HitShape.Get_Label(), HitShape.Get_Shape(), ShapeWorldTM);

            Bounds += lag_comp::Get_SnapshotWorldBounds(Snapshot);
        }

        InCurrent._Frames.Push(FCk_LagComp_RewindFrame{Now, MoveTemp(Snapshots), Bounds});
        InCurrent._LastRecordTime = Now;

#if !UE_BUILD_SHIPPING
        if (rewind_history_cvars::CVarDebugDraw.GetValueOnGameThread())
        {
            rewind_history_detail::DoDraw_OldestFrame(World, InCurrent.Get_Frames(),
                rewind_history_detail::Get_EffectiveRecordInterval(InParams));
        }
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
