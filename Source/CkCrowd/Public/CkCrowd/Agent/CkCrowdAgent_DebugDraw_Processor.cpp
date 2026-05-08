#include "CkCrowdAgent_DebugDraw_Processor.h"

#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DebugDraw);

// --------------------------------------------------------------------------------------------------------------------

// CVar `ck.Crowd.Debug` is now declared via UPROPERTY in UCk_Crowd_DebugSettings_UE
// (CkCrowd/Settings/CkCrowd_DebugSettings.h) so it persists across editor sessions via
// EditorPerProjectUserSettings.ini. Read it through the settings BPFL.

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce)
        -> void
    {
        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawSeparation())
        { return; }

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SelfTransform))
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto Feet = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(SelfTransform);
        const auto Center = Feet + FVector{0.0, 0.0, InParams.Get_Height() * 0.5};

        // --- Separation radius circle (yellow, on the ground at agent feet) -------------------
        // Anchored at feet so it visually represents the floor-projected zone where neighbors
        // start contributing force. NumSegments=24 keeps the curve smooth without spamming line
        // calls.
        constexpr auto CircleSegments = 24;
        constexpr auto LineThickness = 1.5f;
        constexpr auto Duration_OneFrame = 0.0f;
        UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
            World,
            Feet,
            InParams.Get_SeparationRadius(),
            ECk_Plane_Axis::XY,
            CircleSegments,
            FLinearColor{1.0f, 0.85f, 0.0f, 0.6f},
            Duration_OneFrame,
            LineThickness);

        // --- Lines to each neighbor (cyan, agent center to neighbor center) -------------------
        // Drawn before the force arrow so the arrow paints on top. Using neighbor-relative offset
        // (already cached) avoids re-querying the neighbor's Transform.
        for (const auto& Nbr : InNeighborCache.Get_Neighbors())
        {
            const auto NeighborCenter = Center + Nbr.Get_RelativeOffset();
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                World,
                Center,
                NeighborCenter,
                FLinearColor{0.42f, 0.85f, 1.0f, 0.7f},
                Duration_OneFrame,
                LineThickness);
        }

        // --- Separation force arrow (orange, from agent center) -------------------------------
        // Length scaled by magnitude with a sane visual cap so a multi-neighbor pile-up doesn't
        // produce an arrow longer than the room. The 0.5 multiplier is purely aesthetic — force
        // magnitudes can hit several hundred at default weight=2.0 with overlapping neighbors,
        // which would dwarf the agent at 1:1 scale.
        const auto& Force = InSeparationForce.Get_Force();
        const auto ForceMag = Force.Size();
        if (ForceMag > 1.0)
        {
            constexpr auto VisualScale = 0.5f;
            constexpr auto MaxArrowLength = 300.0f;
            const auto ArrowLength = FMath::Min<double>(ForceMag * VisualScale, MaxArrowLength);
            const auto ArrowEnd = Center + (Force / ForceMag) * ArrowLength;

            constexpr auto ArrowSize = 20.0f;
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(
                World,
                Center,
                ArrowEnd,
                ArrowSize,
                FLinearColor{1.0f, 0.55f, 0.15f, 0.9f},
                Duration_OneFrame,
                LineThickness * 1.5f);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
