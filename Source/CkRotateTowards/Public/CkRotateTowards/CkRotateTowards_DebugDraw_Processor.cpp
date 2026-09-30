#include "CkRotateTowards_DebugDraw_Processor.h"

#include "CkRotateTowards/CkRotateTowards_Kernel.h"
#include "CkRotateTowards/Settings/CkRotateTowards_DebugSettings.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_RotateTowards_DebugDraw);

namespace ck
{
    auto
        FProcessor_RotateTowards_DebugDraw::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_RotateTowards_DebugSettings_UE::Get_DrawTargets())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_RotateTowards_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RotateTowards_Tunables& InTunables,
            const FFragment_RotateTowards& InRotateTowards)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto Self = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(Self))
        { return; }

        const auto& Target = InRotateTowards.Get_Target();
        const auto Color = [&]() -> FLinearColor
        {
            if (InHandle.Has<FTag_RotateTowards_Disabled>())
            { return FLinearColor::Red; }

            if (ck::Is_NOT_Valid(Target))
            { return FLinearColor::Gray; }

            if (InHandle.Has<FTag_RotateTowards_AtTarget>())
            { return FLinearColor::Green; }

            return FLinearColor::Yellow;
        }();

        const auto Location = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Self);
        const auto Forward = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(Self).Vector();

        UCk_Utils_DebugDraw_UE::DrawDebugArrow(World, Location, Location + Forward * 60.0, 10.0f, Color, 0.0f, 2.0f);

        // The SolveResult fragment exists only while a target is set; an untargeted entity still gets its forward arrow.
        const auto HasSolveResult = InHandle.Has<FFragment_RotateTowards_SolveResult>();
        if (ck::IsValid(Target) && HasSolveResult)
        {
            const auto& Result = InHandle.Get<FFragment_RotateTowards_SolveResult>();
            UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Location, UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Target), Color, 0.0f, 1.0f);
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(World, Location, Location + Result.Get_DesiredRotation().Vector() * 60.0,
                10.0f, FLinearColor::White, 0.0f, 1.0f);
        }

        if (InHandle.Has<FFragment_RotateTowards_RangeClamp>())
        {
            const auto& RangeClamp = InHandle.Get<FFragment_RotateTowards_RangeClamp>();
            const auto& RestReferencePoint = RangeClamp.Get_RestReferencePoint();
            if (ck::IsValid(RestReferencePoint))
            {
                const auto RestLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(RestReferencePoint);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Location, Location + (RestLocation - Location).GetSafeNormal() * 80.0,
                    FLinearColor::Blue, 0.0f, 1.0f);

                const auto Rest = rotate_towards::Compute_LookAtRotation(Location, RestLocation);
                const auto& YawRange = RangeClamp.Get_Yaw();
                if (Rest.IsSet() && YawRange.Get_Enabled() == ECk_EnableDisable::Enable)
                {
                    const auto RestYaw = Rest.GetValue().Yaw;
                    const auto MinEdge = FRotator{0.0, RestYaw + YawRange.Get_RangeDeg().Get_Min(), 0.0}.Vector();
                    const auto MaxEdge = FRotator{0.0, RestYaw + YawRange.Get_RangeDeg().Get_Max(), 0.0}.Vector();
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Location, Location + MinEdge * 80.0, FLinearColor{0.0f, 1.0f, 1.0f}, 0.0f, 1.0f);
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Location, Location + MaxEdge * 80.0, FLinearColor{0.0f, 1.0f, 1.0f}, 0.0f, 1.0f);
                }
            }
        }

        if (NOT HasSolveResult)
        {
            UCk_Utils_DebugDraw_UE::DrawDebugString(World, Location + FVector{0.0, 0.0, 20.0}, TEXT("No target"), Color, 0.0f);
            return;
        }

        const auto& Result = InHandle.Get<FFragment_RotateTowards_SolveResult>();
        const auto& Desired = Result.Get_DesiredRotation();
        const auto& Remaining = Result.Get_RemainingDelta();
        const auto Text = ck::Format_UE(
            TEXT("Des ({:.1f},{:.1f},{:.1f}) Rem ({:.1f},{:.1f},{:.1f})"),
            Desired.Pitch, Desired.Yaw, Desired.Roll,
            Remaining.Pitch, Remaining.Yaw, Remaining.Roll);
        UCk_Utils_DebugDraw_UE::DrawDebugString(World, Location + FVector{0.0, 0.0, 20.0}, Text, Color, 0.0f);
    }
}
