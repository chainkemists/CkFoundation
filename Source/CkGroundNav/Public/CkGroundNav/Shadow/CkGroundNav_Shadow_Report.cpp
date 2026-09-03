#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Report.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_shadow_report
{
    using namespace ck::groundnav;

    constexpr auto LinePrefix = TEXT("[SHADOW-REPORT] ");
    constexpr auto EmptyCell = TEXT("-");

    // Not LINE_TERMINATOR: a report is compared byte for byte against one captured on another
    // machine, so the separator cannot be the one the host platform happens to use.
    constexpr auto LineSeparator = TEXT("\n");

    auto Get_IsSuccess(
        ECk_Nav_PathStatus InStatus) -> bool
    {
        return InStatus == ECk_Nav_PathStatus::Ready || InStatus == ECk_Nav_PathStatus::Partial;
    }

    auto Get_IsSuccess(
        ECk_GroundNav_PathStatus InStatus) -> bool
    {
        return InStatus == ECk_GroundNav_PathStatus::Ready || InStatus == ECk_GroundNav_PathStatus::Partial;
    }

    auto Get_SortedKeys(
        const TMap<FName, FCk_GroundNav_ShadowFixtureCounters>& InPerFixture) -> TArray<FName>
    {
        auto Keys = TArray<FName>{};
        InPerFixture.GetKeys(Keys);

        ck::algo::Sort(Keys, [](const FName& InLhs, const FName& InRhs) -> bool
        {
            return InLhs.Compare(InRhs) < 0;
        });

        return Keys;
    }

    auto Format_Uu(
        double InValue) -> FString
    {
        return FString::Printf(TEXT("%.3f"), InValue);
    }

    auto Format_Ratio(
        double InValue) -> FString
    {
        return FString::Printf(TEXT("%.4f"), InValue);
    }

    auto Format_Ms(
        double InValue) -> FString
    {
        return FString::Printf(TEXT("%.4f"), InValue);
    }

    auto Format_StatusPairs(
        const FCk_GroundNav_ShadowFixtureCounters& InCounters) -> FString
    {
        auto Entries = TArray<FString>{};

        for (auto RecastIndex = 0; RecastIndex < ShadowRecastStatusCount; ++RecastIndex)
        {
            for (auto GroundNavIndex = 0; GroundNavIndex < ShadowGroundNavStatusCount; ++GroundNavIndex)
            {
                const auto PairCount = InCounters._StatusPairs[RecastIndex][GroundNavIndex];

                if (PairCount == 0)
                { continue; }

                Entries.Add(FString::Printf(TEXT("r%dg%d=%d"), RecastIndex, GroundNavIndex, PairCount));
            }
        }

        return Entries.IsEmpty() ? FString{EmptyCell} : FString::Join(Entries, TEXT(";"));
    }

    auto Format_Distribution_Uu(
        const FCk_GroundNav_ShadowStats& InStats) -> TArray<FString>
    {
        return TArray<FString>{
            Format_Uu(InStats.Get_Mean()),
            Format_Uu(InStats.Get_P95Approx()),
            Format_Uu(InStats._Max)};
    }

    auto Format_Distribution_Ratio(
        const FCk_GroundNav_ShadowStats& InStats) -> TArray<FString>
    {
        return TArray<FString>{
            Format_Ratio(InStats.Get_Mean()),
            Format_Ratio(InStats.Get_P95Approx()),
            Format_Ratio(InStats._Max)};
    }

    auto Format_Distribution_Ms(
        const FCk_GroundNav_ShadowStats& InStats) -> TArray<FString>
    {
        return TArray<FString>{
            Format_Ms(InStats.Get_Mean()),
            Format_Ms(InStats.Get_P95Approx()),
            Format_Ms(InStats._Max)};
    }

    auto Format_Row(
        FName                                      InFixture,
        const FCk_GroundNav_ShadowFixtureCounters& InCounters) -> FString
    {
        auto Cells = TArray<FString>{};

        Cells.Add(InFixture.IsNone() ? FString{EmptyCell} : InFixture.ToString());

        Cells.Add(FString::FromInt(InCounters._Comparisons));
        Cells.Add(FString::FromInt(InCounters._BothSucceeded));
        Cells.Add(FString::FromInt(InCounters._RecastOnly));
        Cells.Add(FString::FromInt(InCounters._GroundNavOnly));
        Cells.Add(FString::FromInt(InCounters._BothFailed));
        Cells.Add(FString::FromInt(InCounters._FailReasonAgree));
        Cells.Add(FString::FromInt(InCounters._FailReasonDisagree));
        Cells.Add(FString::FromInt(InCounters._PartialDisagree));
        Cells.Add(FString::FromInt(InCounters._ContainmentEscapes));

        Cells.Append(Format_Distribution_Uu(InCounters._LengthDeltaAbsUu));
        Cells.Append(Format_Distribution_Ratio(InCounters._LengthDeltaRel));
        Cells.Append(Format_Distribution_Uu(InCounters._EndpointDeltaUu));
        Cells.Append(Format_Distribution_Ratio(InCounters._WaypointCountDelta));
        Cells.Append(Format_Distribution_Ms(InCounters._RecastQueryMs));
        Cells.Append(Format_Distribution_Ms(InCounters._GroundNavQueryMs));

        Cells.Add(Format_StatusPairs(InCounters));

        return FString::Join(Cells, TEXT("|"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::shadow::
    Accumulate(
        FFragment_GroundNav_ShadowDiagnostics& InOutDiagnostics,
        const FCk_GroundNav_ShadowComparison&  InComparison,
        FName                                  InFallbackKey)
    -> void
{
    const auto FixtureKey = InOutDiagnostics._ActiveFixture.IsNone()
        ? InFallbackKey
        : InOutDiagnostics._ActiveFixture;

    auto& Counters = InOutDiagnostics._PerFixture.FindOrAdd(FixtureKey);

    ++Counters._Comparisons;

    const auto RecastSucceeded = ck_groundnav_shadow_report::Get_IsSuccess(InComparison._RecastStatus);
    const auto GroundNavSucceeded = ck_groundnav_shadow_report::Get_IsSuccess(InComparison._GroundNavStatus);

    if (RecastSucceeded && GroundNavSucceeded)
    { ++Counters._BothSucceeded; }
    else if (RecastSucceeded)
    { ++Counters._RecastOnly; }
    else if (GroundNavSucceeded)
    { ++Counters._GroundNavOnly; }
    else
    { ++Counters._BothFailed; }

    const auto FailReasonsAgree = InComparison._RecastFailReason == InComparison._GroundNavFailReason;

    if (NOT RecastSucceeded && NOT GroundNavSucceeded)
    {
        if (FailReasonsAgree)
        { ++Counters._FailReasonAgree; }
        else
        { ++Counters._FailReasonDisagree; }
    }

    const auto RecastIsPartial = InComparison._RecastStatus == ECk_Nav_PathStatus::Partial;
    const auto GroundNavIsPartial = InComparison._GroundNavStatus == ECk_GroundNav_PathStatus::Partial;
    const auto PartialDisagrees = RecastIsPartial != GroundNavIsPartial;

    if (PartialDisagrees)
    { ++Counters._PartialDisagree; }

    const auto RecastStatusIndex = static_cast<int32>(InComparison._RecastStatus);
    const auto GroundNavStatusIndex = static_cast<int32>(InComparison._GroundNavStatus);

    const auto StatusIndicesAreInRange =
        RecastStatusIndex >= 0 && RecastStatusIndex < ShadowRecastStatusCount &&
        GroundNavStatusIndex >= 0 && GroundNavStatusIndex < ShadowGroundNavStatusCount;

    CK_ENSURE_IF_NOT(StatusIndicesAreInRange,
        TEXT("Shadow comparison [{}] carried statuses [{}] / [{}] outside the status-pair matrix"),
        InComparison._QueryId, InComparison._RecastStatus, InComparison._GroundNavStatus)
    { return; }

    ++Counters._StatusPairs[RecastStatusIndex][GroundNavStatusIndex];

    Counters._RecastQueryMs.Add(InComparison._RecastQueryMs);
    Counters._GroundNavQueryMs.Add(InComparison._GroundNavSearchMs);

    if (RecastSucceeded && GroundNavSucceeded)
    {
        const auto LengthDeltaAbs = FMath::Abs(InComparison._GroundNavLengthUu - InComparison._RecastLengthUu);
        const auto LengthBasis = FMath::Max(FMath::Abs(InComparison._RecastLengthUu), static_cast<double>(UE_KINDA_SMALL_NUMBER));

        Counters._LengthDeltaAbsUu.Add(LengthDeltaAbs);
        Counters._LengthDeltaRel.Add(LengthDeltaAbs / LengthBasis);
        Counters._EndpointDeltaUu.Add(FVector::Distance(InComparison._RecastEndpoint, InComparison._GroundNavEndpoint));
        Counters._WaypointCountDelta.Add(
            static_cast<double>(InComparison._GroundNavWaypointCount - InComparison._RecastWaypointCount));
    }

    const auto OutcomeDisagrees = RecastSucceeded != GroundNavSucceeded;
    const auto FailReasonDisagrees = NOT RecastSucceeded && NOT GroundNavSucceeded && NOT FailReasonsAgree;
    const auto Diverged = OutcomeDisagrees || FailReasonDisagrees || PartialDisagrees;

    if (Diverged && NOT InComparison._QueryId.IsNone())
    { InOutDiagnostics._DivergingQueryIds.AddUnique(InComparison._QueryId); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::shadow::
    Get_ReportHeader()
    -> FString
{
    return FString{
        TEXT("fixture|comparisons|both_succeeded|recast_only|groundnav_only|both_failed|")
        TEXT("failreason_agree|failreason_disagree|partial_disagree|containment_escapes|")
        TEXT("len_delta_uu_mean|len_delta_uu_p95~|len_delta_uu_max|")
        TEXT("len_delta_rel_mean|len_delta_rel_p95~|len_delta_rel_max|")
        TEXT("endpoint_uu_mean|endpoint_uu_p95~|endpoint_uu_max|")
        TEXT("wp_delta_mean|wp_delta_p95~|wp_delta_max|")
        TEXT("recast_ms_mean|recast_ms_p95~|recast_ms_max|")
        TEXT("groundnav_ms_mean|groundnav_ms_p95~|groundnav_ms_max|status_pairs")};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::shadow::
    Get_Report(
        const FFragment_GroundNav_ShadowDiagnostics& InDiagnostics,
        const FString&                               InArtifactIdentity)
    -> FString
{
    auto Lines = TArray<FString>{};

    Lines.Add(FString{ck_groundnav_shadow_report::LinePrefix} + TEXT("schema=1"));
    Lines.Add(FString{ck_groundnav_shadow_report::LinePrefix} + TEXT("header=") + Get_ReportHeader());

    for (const auto& FixtureKey : ck_groundnav_shadow_report::Get_SortedKeys(InDiagnostics.Get_PerFixture()))
    {
        Lines.Add(FString{ck_groundnav_shadow_report::LinePrefix} + TEXT("row=") +
            ck_groundnav_shadow_report::Format_Row(FixtureKey, InDiagnostics.Get_PerFixture()[FixtureKey]));
    }

    auto DivergingNames = TArray<FString>{};

    auto SortedDiverging = InDiagnostics.Get_DivergingQueryIds();
    ck::algo::Sort(SortedDiverging, [](const FName& InLhs, const FName& InRhs) -> bool
    {
        return InLhs.Compare(InRhs) < 0;
    });

    for (const auto& QueryId : SortedDiverging)
    { DivergingNames.Add(QueryId.ToString()); }

    Lines.Add(FString{ck_groundnav_shadow_report::LinePrefix} + TEXT("diverging=") +
        (DivergingNames.IsEmpty()
            ? FString{ck_groundnav_shadow_report::EmptyCell}
            : FString::Join(DivergingNames, TEXT(","))));

    Lines.Add(FString{ck_groundnav_shadow_report::LinePrefix} + TEXT("artifact=") +
        (InArtifactIdentity.IsEmpty() ? FString{ck_groundnav_shadow_report::EmptyCell} : InArtifactIdentity));

    return FString::Join(Lines, ck_groundnav_shadow_report::LineSeparator);
}

// --------------------------------------------------------------------------------------------------------------------
