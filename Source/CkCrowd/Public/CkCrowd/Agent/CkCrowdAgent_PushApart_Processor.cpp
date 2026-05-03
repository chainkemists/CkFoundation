#include "CkCrowdAgent_PushApart_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PushApart);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // Per dtCrowd's COLLISION_RESOLVE_FACTOR (DetourCrowd.cpp:1599). Sub-1 so we don't
        // overshoot the resolution; iterations converge on the right answer.
        constexpr auto COLLISION_RESOLVE_FACTOR = 0.7f;

        auto Get_IterationCount(ECk_PushApartMode InMode) -> int32
        {
            switch (InMode)
            {
                case ECk_PushApartMode::Disabled: return 0;
                case ECk_PushApartMode::Single:   return 1;
                case ECk_PushApartMode::Standard: return 4;
                default:                          return 4;
            }
        }
    }

    auto
        FProcessor_CrowdAgent_PushApart::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache)
        -> void
    {
        const auto Iterations = Get_IterationCount(UCk_Utils_Crowd_Settings_UE::Get_PushApartMode());
        if (Iterations <= 0)
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SelfTransform))
        { return; }

        const auto SelfRadius = InParams.Get_Radius();

        auto Displacement = FVector::ZeroVector;

        for (auto Iter = 0; Iter < Iterations; ++Iter)
        {
            for (const auto& Nbr : Neighbors)
            {
                // Use the cache's RelativeOffset directly — it's NbrLoc - SelfLoc as of NeighborSync.
                // After self has displaced by Displacement, the offset from new-self to neighbor is:
                //   NbrLoc - (SelfLoc + Displacement) = RelOffset - Displacement
                // We want the vector pointing FROM neighbor TO new-self (the push direction):
                //   (SelfLoc + Displacement) - NbrLoc = -RelOffset + Displacement
                const auto Diff = -Nbr.Get_RelativeOffset() + Displacement;
                const auto Dist = static_cast<float>(Diff.Size());
                const auto NeighborRadius = SelfRadius;  // approximation — neighbors share radius
                const auto CombinedRadius = SelfRadius + NeighborRadius;

                if (Dist >= CombinedRadius)
                { continue; }

                if (Dist < KINDA_SMALL_NUMBER)
                {
                    // Degenerate overlap (exact center match). Push along an arbitrary axis to
                    // unstick. Mirrors dtCrowd's degenerate-case handling.
                    Displacement += FVector(0.5f * CombinedRadius, 0.0f, 0.0f);
                    continue;
                }

                const auto Penetration = CombinedRadius - Dist;
                const auto Pen = (Penetration * 0.5f) * COLLISION_RESOLVE_FACTOR / Dist;
                Displacement += Diff * Pen;
            }
        }

        if (NOT Displacement.IsNearlyZero())
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset(
                SelfTransform,
                FCk_Request_Transform_AddLocationOffset{Displacement});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
