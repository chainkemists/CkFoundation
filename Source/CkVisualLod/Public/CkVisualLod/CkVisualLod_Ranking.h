#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Math/Vector.h>

// --------------------------------------------------------------------------------------------------------------------
// Local-view resolution + ranked promote selection. Pure of ECS mutation — the arbiter collects
// rank entries, these decide. Exercised directly by the CkVisualLod automation tests.

namespace ck
{
    struct CKVISUALLOD_API FVisualLod_LocalView
    {
    public:
        CK_GENERATED_BODY(FVisualLod_LocalView);

    public:
        bool _IsValid = false;

        FVector _Location = FVector::ZeroVector;

        FVector _Forward = FVector::ForwardVector;

        // Cosine of the half-cone, so the view test is a dot-product compare with no trig per entity
        float _CosHalfCone = -1.0f;
    };

    struct CKVISUALLOD_API FVisualLod_RankEntry
    {
    public:
        CK_GENERATED_BODY(FVisualLod_RankEntry);

    public:
        // Caller-owned index (into the arbiter's per-tick entity scratch list), handed straight
        // back in the selection
        int32 _Index = INDEX_NONE;

        float _Distance = 0.0f;

        bool _InView = false;
    };

    struct CKVISUALLOD_API FVisualLod_Selection
    {
    public:
        CK_GENERATED_BODY(FVisualLod_Selection);

    public:
        TArray<int32> _PromoteIndices;

        // Preempted incumbents are only DEMOTED — their slot frees at fade end, so the challenger
        // wins it through the ordinary budget path on a later tick
        TArray<int32> _PreemptDemoteIndices;
    };

    // Value-only projection of FCk_VisualLod_RenderBand, deliberately free of assets/UObjects so
    // the threshold and hysteresis contract has cheap deterministic automation coverage.
    struct CKVISUALLOD_API FVisualLod_RenderBandRange
    {
    public:
        CK_GENERATED_BODY(FVisualLod_RenderBandRange);

    public:
        float _DistanceThreshold = 0.0f;
        float _ReturnHysteresis = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    namespace visual_lod
    {
        CKVISUALLOD_API auto
        Get_IsInView(
            const FVector& InEntityLocation,
            const FVisualLod_LocalView& InView,
            float InAlwaysInViewDistance,
            float InDistance) -> bool;

        // Empty is the legacy single-profile path. Authored bands start at zero, strictly increase,
        // and must not let a band's inward return point reach the preceding threshold.
        CKVISUALLOD_API auto
        Get_AreRenderBandsValid(
            const TArray<FVisualLod_RenderBandRange>& InBands) -> bool;

        // Initial selection (INDEX_NONE) chooses the greatest threshold <= distance. Existing
        // members step outward at the next threshold and inward only below the current band's
        // return point; the loops intentionally support teleports across multiple bands.
        CKVISUALLOD_API auto
        Get_RenderBandIndex(
            const TArray<FVisualLod_RenderBandRange>& InBands,
            float InDistance,
            int32 InCurrentBandIndex = INDEX_NONE) -> int32;

        // In-view-first then nearest-first partial ranking. Promotes into free budget unthrottled;
        // preempts the worst incumbent only when strictly better by the margin, at most
        // InMaxPreempts per call
        CKVISUALLOD_API auto
        Select_Flips(
            const TArray<FVisualLod_RankEntry>& InCandidates,
            const TArray<FVisualLod_RankEntry>& InIncumbents,
            int32 InFreeBudget,
            int32 InMaxPreempts,
            float InPreemptDistanceMargin) -> FVisualLod_Selection;

        CKVISUALLOD_API auto
        Get_IsBetterCandidate(
            const FVisualLod_RankEntry& InLhs,
            const FVisualLod_RankEntry& InRhs) -> bool;

        CKVISUALLOD_API auto
        Get_IsWorseIncumbent(
            const FVisualLod_RankEntry& InLhs,
            const FVisualLod_RankEntry& InRhs) -> bool;

        // Partial selection order: only the first InCount positions are ordered — a full sort of
        // every in-range entity would be O(n^2) for a result nobody reads past the budget
        CKVISUALLOD_API auto
        Get_RankOrder(
            const TArray<FVisualLod_RankEntry>& InEntries,
            int32 InCount) -> TArray<int32>;

        CKVISUALLOD_API auto
        Find_WorstIncumbent(
            const TArray<FVisualLod_RankEntry>& InIncumbents,
            const TArray<int32>& InAlreadyChosen) -> int32;
    }
}

// --------------------------------------------------------------------------------------------------------------------
