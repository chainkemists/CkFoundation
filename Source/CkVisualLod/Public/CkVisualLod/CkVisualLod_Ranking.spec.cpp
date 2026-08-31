#include "CkVisualLod/CkVisualLod_Ranking.h"
#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"

#include "Misc/AutomationTest.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_visuallod_ranking_spec
{
    auto MakeEntry(int32 InIndex, float InDistance, bool InInView) -> ck::FVisualLod_RankEntry
    {
        auto Entry = ck::FVisualLod_RankEntry{};
        Entry._Index    = InIndex;
        Entry._Distance = InDistance;
        Entry._InView   = InInView;
        return Entry;
    }

    auto
    MakeRenderBand(
        float InThreshold,
        float InReturnHysteresis)
        -> ck::FVisualLod_RenderBandRange
    {
        auto Band = ck::FVisualLod_RenderBandRange{};
        Band._DistanceThreshold = InThreshold;
        Band._ReturnHysteresis  = InReturnHysteresis;
        return Band;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_Ranking_InViewBeatsDistance,
    "Ck.CkVisualLod.Ranking.InViewBeatsDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_Ranking_InViewBeatsDistance::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    // A far in-view candidate outranks a near out-of-view one; distance breaks ties within a group
    const auto Candidates = TArray<ck::FVisualLod_RankEntry>{
        MakeEntry(0, 500.0f,  false),
        MakeEntry(1, 2000.0f, true),
        MakeEntry(2, 1000.0f, true),
        MakeEntry(3, 100.0f,  false),
    };

    const auto Selection = ck::visual_lod::Select_Flips(Candidates, {}, 3, 0, 400.0f);

    TestEqual(TEXT("3 promotes for budget 3"), Selection._PromoteIndices.Num(), 3);
    TestEqual(TEXT("nearest in-view first"), Selection._PromoteIndices[0], 2);
    TestEqual(TEXT("farther in-view second"), Selection._PromoteIndices[1], 1);
    TestEqual(TEXT("nearest out-of-view third"), Selection._PromoteIndices[2], 3);
    TestEqual(TEXT("no preempts requested"), Selection._PreemptDemoteIndices.Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_Ranking_BudgetSpend,
    "Ck.CkVisualLod.Ranking.BudgetSpend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_Ranking_BudgetSpend::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    const auto Candidates = TArray<ck::FVisualLod_RankEntry>{
        MakeEntry(0, 300.0f, true),
        MakeEntry(1, 200.0f, true),
        MakeEntry(2, 100.0f, true),
    };

    {
        const auto Selection = ck::visual_lod::Select_Flips(Candidates, {}, 0, 0, 400.0f);
        TestEqual(TEXT("zero budget, zero preempts -> nothing"), Selection._PromoteIndices.Num(), 0);
    }

    {
        const auto Selection = ck::visual_lod::Select_Flips(Candidates, {}, 2, 0, 400.0f);
        TestEqual(TEXT("budget 2 -> best 2"), Selection._PromoteIndices.Num(), 2);
        TestEqual(TEXT("best first"), Selection._PromoteIndices[0], 2);
        TestEqual(TEXT("second best"), Selection._PromoteIndices[1], 1);
    }

    {
        const auto Selection = ck::visual_lod::Select_Flips(Candidates, {}, 10, 0, 400.0f);
        TestEqual(TEXT("budget above pool -> everything"), Selection._PromoteIndices.Num(), 3);
    }

    {
        const auto Selection = ck::visual_lod::Select_Flips(Candidates, {}, -5, 0, 400.0f);
        TestEqual(TEXT("negative budget clamps to zero"), Selection._PromoteIndices.Num(), 0);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_Ranking_PreemptMarginAndRateLimit,
    "Ck.CkVisualLod.Ranking.PreemptMarginAndRateLimit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_Ranking_PreemptMarginAndRateLimit::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    constexpr auto Margin = 400.0f;

    // Budget exhausted (0 free): challengers may only preempt
    const auto Incumbents = TArray<ck::FVisualLod_RankEntry>{
        MakeEntry(10, 2500.0f, false),   // the worst incumbent (out-of-view, farthest)
        MakeEntry(11, 900.0f,  true),
        MakeEntry(12, 1800.0f, false),
    };

    {
        // In-view challenger beats any out-of-view incumbent regardless of distance margin
        const auto Candidates = TArray<ck::FVisualLod_RankEntry>{MakeEntry(0, 2400.0f, true)};
        const auto Selection = ck::visual_lod::Select_Flips(Candidates, Incumbents, 0, 2, Margin);
        TestEqual(TEXT("no free-budget promotes"), Selection._PromoteIndices.Num(), 0);
        TestEqual(TEXT("one preempt"), Selection._PreemptDemoteIndices.Num(), 1);
        TestEqual(TEXT("worst incumbent demoted"), Selection._PreemptDemoteIndices[0], 10);
    }

    {
        // Same view-group challenger must win by MORE than the margin
        const auto Equalish = TArray<ck::FVisualLod_RankEntry>{MakeEntry(0, 2200.0f, false)};
        const auto Selection = ck::visual_lod::Select_Flips(Equalish, Incumbents, 0, 2, Margin);
        TestEqual(TEXT("300uu better < 400uu margin -> incumbent keeps the slot"),
            Selection._PreemptDemoteIndices.Num(), 0);
    }

    {
        const auto ClearlyBetter = TArray<ck::FVisualLod_RankEntry>{MakeEntry(0, 2000.0f, false)};
        const auto Selection = ck::visual_lod::Select_Flips(ClearlyBetter, Incumbents, 0, 2, Margin);
        TestEqual(TEXT("500uu better > margin -> preempts"), Selection._PreemptDemoteIndices.Num(), 1);
        TestEqual(TEXT("worst incumbent chosen"), Selection._PreemptDemoteIndices[0], 10);
    }

    {
        // Rate limit bounds per-tick churn even when many challengers qualify
        const auto Many = TArray<ck::FVisualLod_RankEntry>{
            MakeEntry(0, 100.0f, true),
            MakeEntry(1, 200.0f, true),
            MakeEntry(2, 300.0f, true),
        };
        const auto Selection = ck::visual_lod::Select_Flips(Many, Incumbents, 0, 2, Margin);
        TestEqual(TEXT("preempts capped at 2"), Selection._PreemptDemoteIndices.Num(), 2);
        TestEqual(TEXT("worst incumbent first"), Selection._PreemptDemoteIndices[0], 10);
        TestEqual(TEXT("next-worst incumbent second"), Selection._PreemptDemoteIndices[1], 12);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_Ranking_BudgetThenPreempt,
    "Ck.CkVisualLod.Ranking.BudgetThenPreempt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_Ranking_BudgetThenPreempt::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    // Free budget is spent on the best candidates; only the OVERFLOW competes for preemption.
    // The best-first cursor means a candidate that failed to preempt blocks all worse ones
    const auto Candidates = TArray<ck::FVisualLod_RankEntry>{
        MakeEntry(0, 100.0f, true),
        MakeEntry(1, 5000.0f, false),
        MakeEntry(2, 200.0f, true),
    };
    const auto Incumbents = TArray<ck::FVisualLod_RankEntry>{
        MakeEntry(10, 1000.0f, true),
    };

    const auto Selection = ck::visual_lod::Select_Flips(Candidates, Incumbents, 2, 2, 400.0f);

    TestEqual(TEXT("budget takes the best two"), Selection._PromoteIndices.Num(), 2);
    TestEqual(TEXT("best candidate"), Selection._PromoteIndices[0], 0);
    TestEqual(TEXT("second candidate"), Selection._PromoteIndices[1], 2);
    TestEqual(TEXT("overflow candidate (out-of-view, far) cannot preempt an in-view incumbent"),
        Selection._PreemptDemoteIndices.Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_Ranking_InViewCone,
    "Ck.CkVisualLod.Ranking.InViewCone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_Ranking_InViewCone::RunTest(const FString&)
{
    auto View = ck::FVisualLod_LocalView{};
    View._IsValid     = true;
    View._Location    = FVector::ZeroVector;
    View._Forward     = FVector::ForwardVector;
    View._CosHalfCone = FMath::Cos(FMath::DegreesToRadians(45.0f));

    constexpr auto AlwaysIn = 600.0f;

    const auto Ahead  = FVector{1000.0f, 0.0f, 0.0f};
    const auto Behind = FVector{-1000.0f, 0.0f, 0.0f};
    const auto Edge   = FVector{1000.0f, 990.0f, 0.0f};       // ~44.7 degrees off-axis
    const auto Outside = FVector{1000.0f, 1100.0f, 0.0f};     // ~47.7 degrees off-axis
    const auto BehindButClose = FVector{-500.0f, 0.0f, 0.0f};

    TestTrue(TEXT("dead ahead is in view"),
        ck::visual_lod::Get_IsInView(Ahead, View, AlwaysIn, static_cast<float>(Ahead.Size())));
    TestFalse(TEXT("behind is out of view"),
        ck::visual_lod::Get_IsInView(Behind, View, AlwaysIn, static_cast<float>(Behind.Size())));
    TestTrue(TEXT("just inside the cone edge counts"),
        ck::visual_lod::Get_IsInView(Edge, View, AlwaysIn, static_cast<float>(Edge.Size())));
    TestFalse(TEXT("just outside the cone edge does not"),
        ck::visual_lod::Get_IsInView(Outside, View, AlwaysIn, static_cast<float>(Outside.Size())));
    TestTrue(TEXT("behind but inside the always-in radius counts"),
        ck::visual_lod::Get_IsInView(BehindButClose, View, AlwaysIn, static_cast<float>(BehindButClose.Size())));

    auto InvalidView = ck::FVisualLod_LocalView{};
    TestFalse(TEXT("invalid view sees nothing"),
        ck::visual_lod::Get_IsInView(Ahead, InvalidView, AlwaysIn, static_cast<float>(Ahead.Size())));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_RenderBands_BoundariesAndStability,
    "Ck.CkVisualLod.RenderBands.BoundariesAndStability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_RenderBands_BoundariesAndStability::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    const auto Bands = TArray<ck::FVisualLod_RenderBandRange>{
        MakeRenderBand(0.0f, 0.0f),
        MakeRenderBand(100.0f, 20.0f),
        MakeRenderBand(200.0f, 30.0f),
    };

    TestTrue(TEXT("valid ordered bands"), ck::visual_lod::Get_AreRenderBandsValid(Bands));
    TestEqual(TEXT("initial chooses zero band below first outward threshold"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 99.0f), 0);
    TestEqual(TEXT("initial boundary belongs to next band"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 100.0f), 1);
    TestEqual(TEXT("outward boundary advances"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 200.0f, 1), 2);
    TestEqual(TEXT("inward equality holds current band"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 170.0f, 2), 2);
    TestEqual(TEXT("inward below return boundary demotes one band"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 169.0f, 2), 1);
    TestEqual(TEXT("inside hysteresis is stable"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 90.0f, 1), 1);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_RenderBands_TeleportAndMalformed,
    "Ck.CkVisualLod.RenderBands.TeleportAndMalformed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_RenderBands_TeleportAndMalformed::RunTest(const FString&)
{
    using namespace ck_visuallod_ranking_spec;

    const auto Bands = TArray<ck::FVisualLod_RenderBandRange>{
        MakeRenderBand(0.0f, 0.0f),
        MakeRenderBand(100.0f, 20.0f),
        MakeRenderBand(200.0f, 30.0f),
        MakeRenderBand(300.0f, 40.0f),
    };

    TestEqual(TEXT("outward teleport crosses every eligible band"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 1000.0f, 0), 3);
    TestEqual(TEXT("inward teleport crosses every return boundary"),
        ck::visual_lod::Get_RenderBandIndex(Bands, 0.0f, 3), 0);
    TestEqual(TEXT("empty bands retain legacy profile path"),
        ck::visual_lod::Get_RenderBandIndex({}, 1000.0f), INDEX_NONE);

    TestFalse(TEXT("first threshold must be zero"), ck::visual_lod::Get_AreRenderBandsValid({
        MakeRenderBand(1.0f, 0.0f)}));
    TestFalse(TEXT("thresholds strictly increase"), ck::visual_lod::Get_AreRenderBandsValid({
        MakeRenderBand(0.0f, 0.0f), MakeRenderBand(100.0f, 0.0f), MakeRenderBand(100.0f, 0.0f)}));
    TestFalse(TEXT("return hysteresis cannot reach prior threshold"), ck::visual_lod::Get_AreRenderBandsValid({
        MakeRenderBand(0.0f, 0.0f), MakeRenderBand(100.0f, 100.0f)}));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_RuntimeTuners_Validation,
    "Ck.CkVisualLod.RuntimeTuners.Validation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_RuntimeTuners_Validation::RunTest(const FString&)
{
    auto Tuners = FCk_VisualLodArbiter_RuntimeTuners{};
    TestTrue(TEXT("default runtime tuners are valid"), ck::visual_lod::Get_AreRuntimeTunersValid(Tuners));

    Tuners.Set_PromoteDistance(2601.0f);
    TestFalse(TEXT("promote distance cannot exceed demote distance"),
        ck::visual_lod::Get_AreRuntimeTunersValid(Tuners));

    Tuners.Set_PromoteDistance(2200.0f);
    Tuners.Set_ViewConeMarginDeg(std::numeric_limits<float>::quiet_NaN());
    TestFalse(TEXT("NaN view cone margin is rejected"), ck::visual_lod::Get_AreRuntimeTunersValid(Tuners));

    Tuners.Set_ViewConeMarginDeg(10.0f);
    Tuners.Set_ExhaustionPolicy(static_cast<ECk_VisualLod_PoolExhaustionPolicy>(255));
    TestFalse(TEXT("unknown exhaustion policy is rejected"), ck::visual_lod::Get_AreRuntimeTunersValid(Tuners));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VisualLod_RuntimeTuners_AtomicSet,
    "Ck.CkVisualLod.RuntimeTuners.AtomicSet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VisualLod_RuntimeTuners_AtomicSet::RunTest(const FString&)
{
    auto Existing = FCk_VisualLodArbiter_RuntimeTuners{};
    Existing.Set_NearBudget(7);

    auto Malformed = Existing;
    Malformed.Set_NearBudget(-1);
    TestFalse(TEXT("malformed candidate is rejected"),
        ck::visual_lod::TrySetRuntimeTuners(Existing, Malformed));
    TestEqual(TEXT("rejected candidate retains the prior budget"), Existing.Get_NearBudget(), 7);

    auto Accepted = Existing;
    Accepted.Set_NearBudget(3);
    TestTrue(TEXT("valid candidate is accepted"), ck::visual_lod::TrySetRuntimeTuners(Existing, Accepted));
    TestEqual(TEXT("accepted candidate replaces the full tuner snapshot"), Existing.Get_NearBudget(), 3);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
