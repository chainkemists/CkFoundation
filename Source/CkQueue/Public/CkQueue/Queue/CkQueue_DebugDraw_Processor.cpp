#include "CkQueue/Queue/CkQueue_DebugDraw_Processor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_DebugDraw);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_queue_debug_draw_processor
{
    static TAutoConsoleVariable<int32> CVarQueueDebugDraw(
        TEXT("ck.Queue.DebugDraw"),
        0,
        TEXT("Draw queue origins, reservations, member-to-slot links, and formation state. 0 = off, 1 = on."),
        ECVF_Cheat);

    constexpr auto DurationOneFrame = 0.0f;
    constexpr auto LineThickness = 2.0f;
    constexpr auto OriginArrowLength = 120.0f;
    constexpr auto OriginArrowSize = 24.0f;
    constexpr auto TargetMarkerHeight = 140.0f;
    constexpr auto TargetMarkerRadius = 52.0f;
    constexpr auto SlotRadius = 32.0f;
    constexpr auto SlotSegments = 16;

    const auto OriginColor = FLinearColor{0.15f, 0.9f, 1.0f, 0.9f};
    const auto ReservationColor = FLinearColor{0.95f, 0.85f, 0.2f, 0.9f};
    const auto WaitingColor = FLinearColor{1.0f, 0.2f, 0.15f, 0.95f};

    auto
        GetStateColor(
            ECk_Queue_State InState)
        -> FLinearColor
    {
        return InState == ECk_Queue_State::Ready
            ? ReservationColor
            : WaitingColor;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Queue_DebugDraw::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Transform& InTransform,
            const FFragment_Queue_Params& InParams,
            const FFragment_Queue_Current& InCurrent)
        -> void
    {
        if (ck_queue_debug_draw_processor::CVarQueueDebugDraw.GetValueOnAnyThread() == 0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InQueue);
        if (NOT IsValid(World))
        { return; }

        const auto OwnerWorldTransform = InTransform.Get_Transform();
        const auto StateColor = ck_queue_debug_draw_processor::GetStateColor(InCurrent.Get_State());
        const auto CategoryText = InParams.Get_Category().IsValid()
            ? InParams.Get_Category().ToString()
            : FString{TEXT("Queue.Category.Unspecified")};
        UCk_Utils_DebugDraw_UE::DrawDebugString(
            World,
            OwnerWorldTransform.GetLocation() + FVector{0.0f, 0.0f, 150.0f},
            FString::Printf(
                TEXT("%s | %s | %d members | %s | rev %d retry %d"),
                *CategoryText,
                *InQueue.Get_DebugName().ToString(),
                InCurrent.Get_Members().Num(),
                *StaticEnum<ECk_Queue_State>()->GetNameStringByValue(static_cast<int64>(InCurrent.Get_State())),
                InCurrent.Get_Revision(),
                InCurrent.Get_RetryEpisode()),
            StateColor,
            ck_queue_debug_draw_processor::DurationOneFrame);

        for (auto OriginIndex = 0; OriginIndex < InCurrent.Get_Origins().Num(); ++OriginIndex)
        {
            const auto OriginWorld = InCurrent.Get_Origins()[OriginIndex].Get_LocalTransform() * OwnerWorldTransform;
            const auto OriginLocation = OriginWorld.GetLocation();
            const auto OriginForward = OriginWorld.GetUnitAxis(EAxis::X);
            UCk_Utils_DebugDraw_UE::DrawDebugCylinder(
                World,
                OriginLocation,
                OriginLocation + FVector{0.0f, 0.0f, ck_queue_debug_draw_processor::TargetMarkerHeight},
                ck_queue_debug_draw_processor::TargetMarkerRadius,
                16,
                ck_queue_debug_draw_processor::OriginColor,
                ck_queue_debug_draw_processor::DurationOneFrame,
                4.0f);
            UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                World,
                OriginLocation,
                ck_queue_debug_draw_processor::TargetMarkerRadius * 1.35f,
                ECk_Plane_Axis::XY,
                24,
                ck_queue_debug_draw_processor::OriginColor,
                ck_queue_debug_draw_processor::DurationOneFrame,
                4.0f);
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(
                World,
                OriginLocation,
                OriginLocation + OriginForward * ck_queue_debug_draw_processor::OriginArrowLength,
                ck_queue_debug_draw_processor::OriginArrowSize,
                ck_queue_debug_draw_processor::OriginColor,
                ck_queue_debug_draw_processor::DurationOneFrame,
                ck_queue_debug_draw_processor::LineThickness);
            UCk_Utils_DebugDraw_UE::DrawDebugString(
                World,
                OriginLocation + FVector{0.0f, 0.0f, ck_queue_debug_draw_processor::TargetMarkerHeight + 20.0f},
                FString::Printf(TEXT("QUEUE TARGET / ORIGIN %d | weight %d"),
                    OriginIndex,
                    InCurrent.Get_Origins()[OriginIndex].Get_Weight()),
                ck_queue_debug_draw_processor::OriginColor,
                ck_queue_debug_draw_processor::DurationOneFrame);
        }

        auto PreviousByOrigin = TArray<TOptional<FTransform>>{};
        PreviousByOrigin.SetNum(InCurrent.Get_Origins().Num());
        for (const auto& Member : InCurrent.Get_Members())
        {
            const auto HasTarget = Member.Get_AssignmentRevision() > 0
                && NOT Member.Get_TargetWorldTransform().ContainsNaN();
            if (NOT HasTarget)
            { continue; }

            const auto Target = Member.Get_TargetWorldTransform();
            const auto TargetLocation = Target.GetLocation();
            UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                World,
                TargetLocation,
                ck_queue_debug_draw_processor::SlotRadius,
                ECk_Plane_Axis::XY,
                ck_queue_debug_draw_processor::SlotSegments,
                StateColor,
                ck_queue_debug_draw_processor::DurationOneFrame,
                ck_queue_debug_draw_processor::LineThickness);
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(
                World,
                TargetLocation,
                TargetLocation + Target.GetUnitAxis(EAxis::X) * 55.0f,
                15.0f,
                StateColor,
                ck_queue_debug_draw_processor::DurationOneFrame,
                ck_queue_debug_draw_processor::LineThickness);
            UCk_Utils_DebugDraw_UE::DrawDebugString(
                World,
                TargetLocation + FVector{0.0f, 0.0f, 20.0f},
                FString::Printf(TEXT("o%d r%d t%lld %s"),
                    Member.Get_OriginIndex(),
                    Member.Get_Rank(),
                    Member.Get_Ticket(),
                    *StaticEnum<ECk_Queue_MemberState>()->GetNameStringByValue(static_cast<int64>(Member.Get_State()))),
                StateColor,
                ck_queue_debug_draw_processor::DurationOneFrame);

            if (PreviousByOrigin.IsValidIndex(Member.Get_OriginIndex())
                && PreviousByOrigin[Member.Get_OriginIndex()].IsSet())
            {
                UCk_Utils_DebugDraw_UE::DrawDebugLine(
                    World,
                    PreviousByOrigin[Member.Get_OriginIndex()]->GetLocation(),
                    TargetLocation,
                    StateColor,
                    ck_queue_debug_draw_processor::DurationOneFrame,
                    ck_queue_debug_draw_processor::LineThickness);
            }
            if (PreviousByOrigin.IsValidIndex(Member.Get_OriginIndex()))
            { PreviousByOrigin[Member.Get_OriginIndex()] = Target; }

            const auto Mover = Member.Get_Mover();
            const auto MoverHasTransform = ck::IsValid(Mover) && UCk_Utils_Transform_UE::Has(Mover);
            if (MoverHasTransform)
            {
                const auto MoverTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                    UCk_Utils_Transform_UE::CastChecked(Mover));
                UCk_Utils_DebugDraw_UE::DrawDebugDashedLine(
                    World,
                    MoverTransform.GetLocation(),
                    TargetLocation,
                    20.0f,
                    StateColor,
                    ck_queue_debug_draw_processor::DurationOneFrame,
                    ck_queue_debug_draw_processor::LineThickness);
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
