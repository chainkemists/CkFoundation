#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include <CoreMinimal.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------
// What a shadow run measures, held as values only.
//
// Nothing here knows about a world, a registry or an entity: a comparison is fed in as a struct of
// numbers and comes back out as a string. That is what lets the accumulator and the report be
// exercised with nothing standing behind them, and what keeps the diagnostics free of anything whose
// lifetime could outlive the run that produced it.
//
// Distributions are kept as fixed-edge histograms rather than samples. A reservoir would not answer
// the same twice for the same input, and a full sample array grows without a bound anyone declared;
// a histogram is bounded, order-independent and reproducible, at the cost of a p95 that is
// interpolated inside one bucket rather than exact - which is why the report labels that column
// approximate.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_GroundNav_ShadowDiagnostics;
}

namespace ck::groundnav
{
    struct FCk_GroundNav_ShadowComparison;
}

namespace ck::groundnav::shadow
{
    CKGROUNDNAV_API auto Accumulate(
        FFragment_GroundNav_ShadowDiagnostics& InOutDiagnostics,
        const FCk_GroundNav_ShadowComparison&  InComparison,
        FName                                  InFallbackKey) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /** Which edge table a distribution buckets against. A distribution cannot pick its own: the
     *  same number means a different thing as a distance, a fraction and a duration. */
    enum class ECk_GroundNav_ShadowMetricKind : uint8
    {
        DistanceUu,
        Ratio,
        Milliseconds,
        IntegerDelta
    };

    inline constexpr int32 ShadowBucketCount = 12;
    inline constexpr int32 ShadowBucketEdgeCount = ShadowBucketCount - 1;

    struct CKGROUNDNAV_API FCk_GroundNav_ShadowBucketEdges
    {
    public:
        double _Edges[ShadowBucketEdgeCount] = {};
    };

    /** Bucket i holds v where Edges[i - 1] <= v < Edges[i]; the first holds everything below the
     *  first edge and the last everything from the final edge up. */
    inline constexpr auto Get_ShadowBucketEdges(
        ECk_GroundNav_ShadowMetricKind InKind) -> FCk_GroundNav_ShadowBucketEdges
    {
        switch (InKind)
        {
            case ECk_GroundNav_ShadowMetricKind::Ratio:
            { return FCk_GroundNav_ShadowBucketEdges{{0.001, 0.0025, 0.005, 0.01, 0.025, 0.05, 0.10, 0.25, 0.50, 1.0, 2.0}}; }

            case ECk_GroundNav_ShadowMetricKind::Milliseconds:
            { return FCk_GroundNav_ShadowBucketEdges{{0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0}}; }

            case ECk_GroundNav_ShadowMetricKind::IntegerDelta:
            { return FCk_GroundNav_ShadowBucketEdges{{-16.0, -8.0, -4.0, -2.0, -1.0, 0.0, 1.0, 2.0, 4.0, 8.0, 16.0}}; }

            case ECk_GroundNav_ShadowMetricKind::DistanceUu:
            default:
            { return FCk_GroundNav_ShadowBucketEdges{{0.5, 1.0, 2.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 1000.0, 5000.0}}; }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    /** One metric's distribution over a fixture, streamed: every sample updates the summary and one
     *  bucket, and nothing is retained per sample. */
    struct CKGROUNDNAV_API FCk_GroundNav_ShadowStats
    {
    public:
        FCk_GroundNav_ShadowStats() = default;

        explicit FCk_GroundNav_ShadowStats(
            ECk_GroundNav_ShadowMetricKind InKind)
            : _Kind(InKind) {}

    public:
        ECk_GroundNav_ShadowMetricKind _Kind = ECk_GroundNav_ShadowMetricKind::DistanceUu;

        int32 _Count = 0;

        double _Min = 0.0;
        double _Max = 0.0;
        double _Sum = 0.0;
        double _SumSq = 0.0;

        int32 _Buckets[ShadowBucketCount] = {};

    public:
        auto Add(
            double InValue) -> void
        {
            const auto Edges = Get_ShadowBucketEdges(_Kind);

            auto BucketIndex = ShadowBucketCount - 1;

            for (auto EdgeIndex = 0; EdgeIndex < ShadowBucketEdgeCount; ++EdgeIndex)
            {
                if (InValue < Edges._Edges[EdgeIndex])
                {
                    BucketIndex = EdgeIndex;
                    break;
                }
            }

            _Min = _Count == 0 ? InValue : FMath::Min(_Min, InValue);
            _Max = _Count == 0 ? InValue : FMath::Max(_Max, InValue);

            ++_Count;
            _Sum += InValue;
            _SumSq += InValue * InValue;
            ++_Buckets[BucketIndex];
        }

        auto Get_Mean() const -> double
        {
            return _Count > 0 ? _Sum / static_cast<double>(_Count) : 0.0;
        }

        /** The 95th percentile read off the histogram, interpolated inside the bucket the rank falls
         *  in. Approximate by construction: the bucket's own edges bound the error, and the extreme
         *  buckets are bounded by the recorded min and max rather than by an open edge. */
        auto Get_P95Approx() const -> double
        {
            if (_Count <= 0)
            { return 0.0; }

            if (_Count == 1)
            { return _Min; }

            const auto Edges = Get_ShadowBucketEdges(_Kind);
            const auto TargetRank = 0.95 * static_cast<double>(_Count);

            auto Cumulative = 0.0;

            for (auto BucketIndex = 0; BucketIndex < ShadowBucketCount; ++BucketIndex)
            {
                const auto InBucket = static_cast<double>(_Buckets[BucketIndex]);

                if (InBucket <= 0.0)
                { continue; }

                if (Cumulative + InBucket < TargetRank)
                {
                    Cumulative += InBucket;
                    continue;
                }

                const auto LowerBound = BucketIndex == 0
                    ? _Min
                    : FMath::Max(Edges._Edges[BucketIndex - 1], _Min);

                const auto RawUpperBound = BucketIndex == ShadowBucketCount - 1
                    ? _Max
                    : FMath::Min(Edges._Edges[BucketIndex], _Max);

                const auto UpperBound = FMath::Max(RawUpperBound, LowerBound);
                const auto Fraction = FMath::Clamp((TargetRank - Cumulative) / InBucket, 0.0, 1.0);

                return LowerBound + (UpperBound - LowerBound) * Fraction;
            }

            return _Max;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr int32 ShadowRecastStatusCount =
        static_cast<int32>(ECk_Nav_PathStatus::Partial) + 1;

    inline constexpr int32 ShadowGroundNavStatusCount =
        static_cast<int32>(ECk_GroundNav_PathStatus::Blocked) + 1;

    static_assert(ShadowRecastStatusCount == 5,
        "ECk_Nav_PathStatus gained or lost a value - the status-pair matrix and the report row move with it");

    static_assert(ShadowGroundNavStatusCount == 9,
        "ECk_GroundNav_PathStatus gained or lost a value - the status-pair matrix and the report row move with it");

    // ----------------------------------------------------------------------------------------------------------------

    /** Everything one fixture's shadow run recorded. Fixed size: the status matrix and every
     *  histogram are C arrays, so a fixture costs the same whether it saw one comparison or a
     *  million. */
    struct CKGROUNDNAV_API FCk_GroundNav_ShadowFixtureCounters
    {
    public:
        int32 _Comparisons = 0;
        int32 _BothSucceeded = 0;
        int32 _RecastOnly = 0;
        int32 _GroundNavOnly = 0;
        int32 _BothFailed = 0;

        int32 _FailReasonAgree = 0;
        int32 _FailReasonDisagree = 0;
        int32 _PartialDisagree = 0;

        // Produced by the crowd's single Transform writer, FProcessor_CrowdAgent_ConstrainToNavmesh,
        // and banked here through FProcessor_GroundNav_ShadowCompare: once per agent per frame, when
        // the position that pass resolved projects onto walkable ground for one of the two providers
        // and onto none for the other. Not a per-comparison question - it is asked of a POSITION
        // rather than of a query pair - which is why it is raised there and only counted here.
        //
        // The count also folds COVERAGE divergence, and will until a whole-band field is staged: an
        // agent standing outside every GroundNav volume is a split verdict on every frame it stands
        // there, and the number therefore says how much of the run happened off the shadow provider's
        // ground as well as where the two disagreed about ground both cover.
        //
        // Readable only through the shadow REPORT and through Get_ShadowContainmentEscapes - the
        // per-comparison [SHADOW-CMP] line does not carry it, because the line describes a query pair
        // and this describes a body.
        int32 _ContainmentEscapes = 0;

        int32 _StatusPairs[ShadowRecastStatusCount][ShadowGroundNavStatusCount] = {};

        FCk_GroundNav_ShadowStats _LengthDeltaAbsUu{ECk_GroundNav_ShadowMetricKind::DistanceUu};
        FCk_GroundNav_ShadowStats _LengthDeltaRel{ECk_GroundNav_ShadowMetricKind::Ratio};
        FCk_GroundNav_ShadowStats _EndpointDeltaUu{ECk_GroundNav_ShadowMetricKind::DistanceUu};
        FCk_GroundNav_ShadowStats _WaypointCountDelta{ECk_GroundNav_ShadowMetricKind::IntegerDelta};
        FCk_GroundNav_ShadowStats _RecastQueryMs{ECk_GroundNav_ShadowMetricKind::Milliseconds};
        FCk_GroundNav_ShadowStats _GroundNavQueryMs{ECk_GroundNav_ShadowMetricKind::Milliseconds};
    };

    // ----------------------------------------------------------------------------------------------------------------

    static_assert(std::is_trivially_copyable_v<FCk_GroundNav_ShadowStats>,
        "A distribution must stay copyable as bytes - anything with a destructor here is a lifetime the report could outlive");

    static_assert(std::is_trivially_copyable_v<FCk_GroundNav_ShadowFixtureCounters>,
        "A fixture's counters must stay copyable as bytes - anything with a destructor here is a lifetime the report could outlive");

    // ----------------------------------------------------------------------------------------------------------------

    /** One comparison, already reduced to numbers by whoever held both results.
     *
     *  The GroundNav fail reason arrives ALREADY MAPPED onto CkNavigation's vocabulary: whether the
     *  two providers agree about a reason is only a question once both are speaking the same one,
     *  and that mapping belongs to the boundary that holds both results, not here. */
    struct CKGROUNDNAV_API FCk_GroundNav_ShadowComparison
    {
    public:
        FName _QueryId;

        ECk_Nav_PathStatus _RecastStatus = ECk_Nav_PathStatus::None;
        ECk_Nav_PathFailReason _RecastFailReason = ECk_Nav_PathFailReason::None;
        int32 _RecastWaypointCount = 0;
        double _RecastLengthUu = 0.0;
        FVector _RecastEndpoint = FVector::ZeroVector;
        double _RecastQueryMs = 0.0;

        ECk_GroundNav_PathStatus _GroundNavStatus = ECk_GroundNav_PathStatus::InProgress;
        ECk_Nav_PathFailReason _GroundNavFailReason = ECk_Nav_PathFailReason::None;
        int32 _GroundNavWaypointCount = 0;
        double _GroundNavLengthUu = 0.0;
        FVector _GroundNavEndpoint = FVector::ZeroVector;
        double _GroundNavSearchMs = 0.0;
    };
}

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GroundNav_Shadow_UE;

namespace ck
{
    /**
     * Every fixture a shadow run has bucketed, on the world's transient entity.
     *
     * Per-world rather than per-agent because the unit a report row speaks about is a fixture, not
     * an agent: per-agent counters would die with the agent that produced them and would need a
     * second pass to become a row. Two PIE worlds therefore hold their own, and neither can read or
     * clear the other's.
     */
    struct CKGROUNDNAV_API FFragment_GroundNav_ShadowDiagnostics
    {
        CK_GENERATED_BODY(FFragment_GroundNav_ShadowDiagnostics);

        friend class FProcessor_GroundNav_ShadowCompare;
        friend class ::UCk_Utils_GroundNav_Shadow_UE;

        friend auto groundnav::shadow::Accumulate(
            FFragment_GroundNav_ShadowDiagnostics&           InOutDiagnostics,
            const groundnav::FCk_GroundNav_ShadowComparison& InComparison,
            FName                                            InFallbackKey) -> void;

    private:
        FName _ActiveFixture;

        TMap<FName, groundnav::FCk_GroundNav_ShadowFixtureCounters> _PerFixture;

        TArray<FName> _DivergingQueryIds;

    public:
        CK_PROPERTY_GET(_ActiveFixture);
        CK_PROPERTY_GET(_PerFixture);
        CK_PROPERTY_GET(_DivergingQueryIds);
    };
}

// --------------------------------------------------------------------------------------------------------------------
