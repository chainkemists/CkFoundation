#include "CkEqs/Query/CkEqs_Algorithm.h"

#include "CkEqs/CkEqs_Log.h"
#include "CkEqs/CkEqs_Stats.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"
#include "CkSpatialQuery/Probe/CkProbeTrace_Utils.h"

#include <Algo/MaxElement.h>
#include <Algo/MinElement.h>
#include <Async/ParallelFor.h>
#include <Math/UnrealMathUtility.h>
#include <NavigationSystem.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Eqs::Generate"), STAT_Eqs_Generate, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::RunTests"), STAT_Eqs_RunTests, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Finalize"), STAT_Eqs_Finalize, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_Distance"), STAT_Eqs_Test_Distance, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_Dot"), STAT_Eqs_Test_Dot, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_Trace"), STAT_Eqs_Test_Trace, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_GameplayTag"), STAT_Eqs_Test_GameplayTag, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_Overlap"), STAT_Eqs_Test_Overlap, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_VolumeCheck"), STAT_Eqs_Test_VolumeCheck, STATGROUP_CkEqs);
DECLARE_CYCLE_STAT(TEXT("Eqs::Test_Random"), STAT_Eqs_Test_Random, STATGROUP_CkEqs);
DECLARE_DWORD_COUNTER_STAT(TEXT("Eqs Candidates Generated"), STAT_Eqs_CandidatesGenerated, STATGROUP_CkEqs);
DECLARE_DWORD_COUNTER_STAT(TEXT("Eqs Physics Casts Issued"), STAT_Eqs_PhysicsCastsIssued, STATGROUP_CkEqs);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_eqs_algorithm
{
    // Coarser than KINDA_SMALL_NUMBER on purpose — matches UE NormalizeItemScores' skip threshold.
    constexpr auto Eqs_DegenerateRangeEpsilon = 1.0e-4f;

    auto
    Eqs_GetReferenceLocation(
        const FCk_Eqs_QueryParams& InParams,
        ECk_Eqs_DistanceTo InMode) -> FVector
    {
        const auto& Context = InParams.Get_Context();
        const auto UseContext = InMode == ECk_Eqs_DistanceTo::Context && ck::IsValid(Context);
        const auto& Reference = UseContext ? Context : InParams.Get_Querier();

        if (NOT Reference.Has<ck::FFragment_Transform>())
        { return FVector::ZeroVector; }

        return Reference.Get<ck::FFragment_Transform>().Get_Transform().GetLocation();
    }

    auto
    Eqs_GetDotFromLocation(
        const FCk_Eqs_QueryParams& InParams,
        ECk_Eqs_DotFrom InMode) -> FVector
    {
        const auto& Context = InParams.Get_Context();
        const auto UseContext = InMode == ECk_Eqs_DotFrom::Context && ck::IsValid(Context);
        const auto& Reference = UseContext ? Context : InParams.Get_Querier();

        if (NOT Reference.Has<ck::FFragment_Transform>())
        { return FVector::ZeroVector; }

        return Reference.Get<ck::FFragment_Transform>().Get_Transform().GetLocation();
    }

    auto
    Eqs_GetDotFromForward(
        const FCk_Eqs_QueryParams& InParams,
        ECk_Eqs_DotFrom InMode) -> FVector
    {
        const auto& Context = InParams.Get_Context();
        const auto UseContext = InMode == ECk_Eqs_DotFrom::Context && ck::IsValid(Context);
        const auto& Reference = UseContext ? Context : InParams.Get_Querier();

        if (NOT Reference.Has<ck::FFragment_Transform>())
        { return FVector::ForwardVector; }

        return Reference.Get<ck::FFragment_Transform>().Get_Transform().GetRotation().GetForwardVector();
    }

    auto
    Eqs_PassesFilter(
        float InRawValue,
        const FCk_Eqs_FilterConfig& InFilter) -> bool
    {
        switch (InFilter.Get_FilterType())
        {
            case ECk_Eqs_FilterType::Minimum:
                return InRawValue >= InFilter.Get_FilterMin();
            case ECk_Eqs_FilterType::Maximum:
                return InRawValue <= InFilter.Get_FilterMax();
            case ECk_Eqs_FilterType::Range:
                return InRawValue >= InFilter.Get_FilterMin() && InRawValue <= InFilter.Get_FilterMax();
        }
        return true;
    }

    auto
    Eqs_ResolveClampMin(
        const FCk_Eqs_ScoringConfig& InConfig,
        const FCk_Eqs_FilterConfig& InFilter,
        float InDataMin) -> float
    {
        switch (InConfig.Get_ClampMinType())
        {
            case ECk_Eqs_ClampType::None:             return InDataMin;
            case ECk_Eqs_ClampType::FilterThreshold:  return InFilter.Get_FilterMin();
            case ECk_Eqs_ClampType::SpecifiedValue:   return InConfig.Get_ClampMin();
        }
        return InDataMin;
    }

    auto
    Eqs_ResolveClampMax(
        const FCk_Eqs_ScoringConfig& InConfig,
        const FCk_Eqs_FilterConfig& InFilter,
        float InDataMax) -> float
    {
        switch (InConfig.Get_ClampMaxType())
        {
            case ECk_Eqs_ClampType::None:             return InDataMax;
            case ECk_Eqs_ClampType::FilterThreshold:  return InFilter.Get_FilterMax();
            case ECk_Eqs_ClampType::SpecifiedValue:   return InConfig.Get_ClampMax();
        }
        return InDataMax;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    NormalizeAndScore(
        float InValue,
        float InMin,
        float InMax,
        const FCk_Eqs_ScoringConfig& InConfig)
    -> float
{
    constexpr auto MultiplicativeIdentity = 1.0f;

    const auto RangeIsDegenerate = FMath::Abs(InMax - InMin) < ck_eqs_algorithm::Eqs_DegenerateRangeEpsilon;
    if (RangeIsDegenerate)
    { return MultiplicativeIdentity * InConfig.Get_ScoringFactor(); }

    // No FilterConfig reaches this function, so ClampType::FilterThreshold resolves against a
    // default filter, not the test's own. For a real threshold use ClampType::None/SpecifiedValue.
    const auto FilterStub = FCk_Eqs_FilterConfig{};
    const auto EffectiveMin = ck_eqs_algorithm::Eqs_ResolveClampMin(InConfig, FilterStub, InMin);
    const auto EffectiveMax = ck_eqs_algorithm::Eqs_ResolveClampMax(InConfig, FilterStub, InMax);

    const auto ClampedValue = FMath::Clamp(InValue, EffectiveMin, EffectiveMax);

    auto Normalized = 0.0f;
    switch (InConfig.Get_NormalizationType())
    {
        case ECk_Eqs_NormalizationType::Absolute:
        {
            const auto AbsMax = FMath::Max(FMath::Abs(EffectiveMax), ck_eqs_algorithm::Eqs_DegenerateRangeEpsilon);
            Normalized = ClampedValue / AbsMax;
            break;
        }
        case ECk_Eqs_NormalizationType::RelativeToScores:
        default:
        {
            const auto Span = FMath::Max(EffectiveMax - EffectiveMin, ck_eqs_algorithm::Eqs_DegenerateRangeEpsilon);
            Normalized = (ClampedValue - EffectiveMin) / Span;
            break;
        }
    }

    Normalized = FMath::Clamp(Normalized, 0.0f, 1.0f);

    auto Transformed = 0.0f;
    switch (InConfig.Get_ScoringEquation())
    {
        case ECk_Eqs_ScoringEquation::Linear:
            Transformed = Normalized;
            break;
        case ECk_Eqs_ScoringEquation::Square:
            Transformed = Normalized * Normalized;
            break;
        case ECk_Eqs_ScoringEquation::SquareRoot:
            Transformed = FMath::Sqrt(Normalized);
            break;
        case ECk_Eqs_ScoringEquation::InverseLinear:
            Transformed = 1.0f - Normalized;
            break;
        case ECk_Eqs_ScoringEquation::Constant:
            Transformed = Normalized > 0.0f ? 1.0f : 0.0f;
            break;
    }

    return Transformed * InConfig.Get_ScoringFactor();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    DoGenerate(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
    -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Generate);

    const auto& Querier = InParams.Get_Querier();

    if (ck::Is_NOT_Valid(Querier))
    { return false; }

    if (NOT Querier.Has<ck::FFragment_Transform>())
    { return false; }

    const auto& QuerierTransform = Querier.Get<ck::FFragment_Transform>().Get_Transform();
    const auto QuerierLocation = QuerierTransform.GetLocation();

    auto& OutCandidates = InState._Candidates;
    OutCandidates.Reset();

    switch (InParams.Get_GeneratorParams().Get_GeneratorType())
    {
        case ECk_Eqs_GeneratorType::SimpleGrid:
            DoGenerate_SimpleGrid(InParams.Get_GeneratorParams(), QuerierLocation, OutCandidates);
            break;
        case ECk_Eqs_GeneratorType::Grid:
            DoGenerate_Grid(InParams.Get_GeneratorParams(), QuerierLocation, Querier, InPhysicsSystem, OutCandidates);
            break;
        case ECk_Eqs_GeneratorType::Donut:
            DoGenerate_Donut(InParams.Get_GeneratorParams(), QuerierLocation, OutCandidates);
            break;
        case ECk_Eqs_GeneratorType::Cone:
            DoGenerate_Cone(InParams.Get_GeneratorParams(), QuerierTransform, OutCandidates);
            break;
        case ECk_Eqs_GeneratorType::EntitiesWithTag:
            DoGenerate_EntitiesWithTag(InParams.Get_GeneratorParams(), Querier, OutCandidates);
            break;
        case ECk_Eqs_GeneratorType::OnCircle:
            DoGenerate_OnCircle(InParams.Get_GeneratorParams(), QuerierLocation, OutCandidates);
            break;
        default:
            ck::eqs::Warning(TEXT("EqsQuery [{}] has UNKNOWN generator type — no candidates produced."), InQueryHandle);
            return false;
    }

    const auto& GenParams = InParams.Get_GeneratorParams();
    if (GenParams.Get_ProjectOntoNav()
        && GenParams.Get_GeneratorType() != ECk_Eqs_GeneratorType::EntitiesWithTag
        && NOT OutCandidates.IsEmpty())
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Querier);
        auto* NavSys = ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{})
            ? UNavigationSystemV1::GetCurrent(World)
            : nullptr;

        if (NavSys != nullptr)
        {
            const auto HalfExt = FMath::Max(GenParams.Get_NavProjectionSearchHalfExtentUu(), 1.0f);
            const auto ProjectionExtent = FVector{HalfExt, HalfExt, HalfExt};

            auto SnappedCandidates = TArray<FCk_Eqs_Candidate>{};
            SnappedCandidates.Reserve(OutCandidates.Num());

            for (auto& Candidate : OutCandidates)
            {
                auto Projected = FNavLocation{};
                if (NavSys->ProjectPointToNavigation(Candidate._Location, Projected, ProjectionExtent))
                {
                    Candidate._Location = Projected.Location;
                    SnappedCandidates.Add(Candidate);
                }
            }

            OutCandidates = MoveTemp(SnappedCandidates);
        }
    }

    INC_DWORD_STAT_BY(STAT_Eqs_CandidatesGenerated, OutCandidates.Num());

    return NOT OutCandidates.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    DoGenerate_SimpleGrid(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    const auto Step = FMath::Max(InGen.Get_SpaceBetween(), 1.0f);
    const auto HalfSize = FMath::Max(InGen.Get_GridHalfSize(), 0.0f);

    const auto NumPerSide = FMath::FloorToInt32((HalfSize * 2.0f) / Step) + 1;
    OutCandidates.Reserve(NumPerSide * NumPerSide);

    for (auto Y = -HalfSize; Y <= HalfSize + KINDA_SMALL_NUMBER; Y += Step)
    {
        for (auto X = -HalfSize; X <= HalfSize + KINDA_SMALL_NUMBER; X += Step)
        {
            const auto Location = InQuerierLocation + FVector{X, Y, 0.0f};
            OutCandidates.Emplace(FCk_Eqs_Candidate{Location});
        }
    }
}

auto
    FCk_Eqs_Algorithm::
    DoGenerate_Grid(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        const FCk_Handle& InAnyHandle,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    DoGenerate_SimpleGrid(InGen, InQuerierLocation, OutCandidates);

    const auto PhysicsSystem = InPhysicsSystem.Pin();

    if (ck::Is_NOT_Valid(PhysicsSystem))
    {
        ck::eqs::Verbose(TEXT("DoGenerate_Grid: physics unavailable; using flat-grid fallback ({} candidates)."),
            OutCandidates.Num());
        return;
    }

    const auto ProjectUp = FMath::Max(InGen.Get_ProjectUp(), 0.0f);
    const auto ProjectDown = FMath::Max(InGen.Get_ProjectDown(), 0.0f);
    const auto& Filter = InGen.Get_ProjectionFilter();

    // EQS scores the world — its casts must not fire overlap events into hit probes.
    constexpr auto FireOverlaps = false;
    constexpr auto TryDrawDebug = false;

    // Worker threads: Jolt narrow-phase is thread-safe, FireOverlaps=false keeps the trace path
    // registry-read-only, and each iteration writes only its own candidate. Multi (not Single)
    // because Single draws debug unconditionally.
    const auto RegistryHandle = InAnyHandle.Get_RegistryView().Get_RegistryHandle();
#if !UE_BUILD_SHIPPING
    ck::registry_table::BeginParallelRegion(RegistryHandle);
#endif

    ParallelFor(OutCandidates.Num(), [&](int32 InIndex)
    {
        auto& Candidate = OutCandidates[InIndex];

        const auto Start = Candidate._Location + FVector::UpVector * ProjectUp;
        const auto End = Candidate._Location - FVector::UpVector * ProjectDown;

        const auto Settings = FCk_Probe_RayCast_Settings{Start, End, Filter};
        INC_DWORD_STAT(STAT_Eqs_PhysicsCastsIssued);
        const auto Hits = UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(
            InAnyHandle, Settings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

        if (Hits.Num() > 0)
        { Candidate._Location = Hits[0].Get_HitLocation(); }
    });

#if !UE_BUILD_SHIPPING
    ck::registry_table::EndParallelRegion(RegistryHandle);
#endif
}

auto
    FCk_Eqs_Algorithm::
    DoGenerate_Donut(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    const auto NumRings = FMath::Max(InGen.Get_NumRings(), 1);
    const auto PointsPerRing = FMath::Max(InGen.Get_PointsPerRing(), 1);
    const auto InnerRadius = FMath::Max(InGen.Get_InnerRadius(), 0.0f);
    const auto OuterRadius = FMath::Max(InGen.Get_OuterRadius(), InnerRadius);
    const auto ArcAngle = FMath::Clamp(InGen.Get_ArcAngle(), 0.0f, 360.0f);

    // UE divides by (NumRings - 1) unguarded; a single ring sits at OuterRadius here.
    const auto RadiusDelta = NumRings > 1
        ? (OuterRadius - InnerRadius) / static_cast<float>(NumRings - 1)
        : 0.0f;
    const auto FirstRingRadius = NumRings > 1 ? InnerRadius : OuterRadius;

    // Point-count denominator (NOT minus-one): UE doesn't close the loop on a full circle.
    const auto AngleDeltaDeg = ArcAngle / static_cast<float>(PointsPerRing);

    const auto ArcYawDeg = InGen.Get_ArcDirection().GetSafeNormal().Rotation().Yaw;
    const auto ArcStartDeg = ArcYawDeg - ArcAngle * 0.5f;

    OutCandidates.Reserve(NumRings * PointsPerRing);

    auto RingRadius = FirstRingRadius;
    for (auto r = 0; r < NumRings; ++r, RingRadius += RadiusDelta)
    {
        const auto SpiralOffsetDeg = InGen.Get_UseSpiralPattern()
            ? (AngleDeltaDeg / static_cast<float>(NumRings)) * static_cast<float>(r)
            : 0.0f;

        for (auto p = 0; p < PointsPerRing; ++p)
        {
            const auto Yaw = ArcStartDeg + SpiralOffsetDeg + AngleDeltaDeg * static_cast<float>(p);
            const auto Direction = FRotator{0.0f, Yaw, 0.0f}.Vector();
            const auto Location = InQuerierLocation + Direction * RingRadius;
            OutCandidates.Emplace(FCk_Eqs_Candidate{Location});
        }
    }
}

auto
    FCk_Eqs_Algorithm::
    DoGenerate_Cone(
        const FCk_Eqs_GeneratorParams& InGen,
        const FTransform& InQuerierTransform,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    const auto ConeDegrees = FMath::Clamp(InGen.Get_ConeDegrees(), 0.0f, 359.0f);
    const auto Range = FMath::Max(InGen.Get_ConeRange(), 1.0f);
    const auto AngleStep = FMath::Clamp(InGen.Get_ConeAngleStep(), 1.0f, 359.0f);
    const auto PointSpacing = FMath::Max(InGen.Get_ConePointSpacing(), 1.0f);

    const auto QuerierLocation = InQuerierTransform.GetLocation();
    const auto Forward = InQuerierTransform.GetRotation().GetForwardVector();
    const auto Up = InQuerierTransform.GetRotation().GetUpVector();

    const auto AngleStart = -ConeDegrees * 0.5f;
    const auto AngleEnd = +ConeDegrees * 0.5f;  // exclusive on the upper end

    const auto NumDistanceSteps = FMath::FloorToInt32(Range / PointSpacing);
    OutCandidates.Reserve(static_cast<int32>(ConeDegrees / AngleStep + 1.0f) * NumDistanceSteps);

    for (auto Angle = AngleStart; Angle < AngleEnd - KINDA_SMALL_NUMBER; Angle += AngleStep)
    {
        const auto Rotated = Forward.RotateAngleAxis(Angle, Up);

        for (auto Distance = PointSpacing; Distance <= Range + KINDA_SMALL_NUMBER; Distance += PointSpacing)
        {
            const auto Location = QuerierLocation + Rotated * Distance;
            OutCandidates.Emplace(FCk_Eqs_Candidate{Location});
        }
    }
}

auto
    FCk_Eqs_Algorithm::
    DoGenerate_EntitiesWithTag(
        const FCk_Eqs_GeneratorParams& InGen,
        const FCk_Handle& InQuerier,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    const auto Tag = InGen.Get_RequiredEntityTag();
    if (NOT Tag.IsValid())
    { return; }

    const auto Entities = UCk_Utils_EntityTag_UE::ForEach_Entity_UsingGameplayTag(InQuerier, Tag);

    auto SkippedCount = 0;
    OutCandidates.Reserve(Entities.Num());

    for (const auto& Entity : Entities)
    {
        if (NOT Entity.Has<ck::FFragment_Transform>())
        {
            ++SkippedCount;
            continue;
        }

        const auto Location = Entity.Get<ck::FFragment_Transform>().Get_Transform().GetLocation();
        auto Candidate = FCk_Eqs_Candidate{Location};
        Candidate._EntityHandle = Entity;
        OutCandidates.Emplace(MoveTemp(Candidate));
    }

    if (SkippedCount > 0)
    {
        ck::eqs::Verbose(TEXT("DoGenerate_EntitiesWithTag: skipped {} entities without FFragment_Transform (tag={})"),
            SkippedCount, Tag.ToString());
    }
}

auto
    FCk_Eqs_Algorithm::
    DoGenerate_OnCircle(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates)
    -> void
{
    const auto NumPoints = FMath::Max(InGen.Get_NumPointsOnCircle(), 2);
    const auto Radius = FMath::Max(InGen.Get_CircleRadius(), 1.0f);
    const auto ArcAngle = FMath::Clamp(InGen.Get_CircleArcAngle(), 1.0f, 360.0f);

    // Donut's convention (denominator NOT minus-one): a full circle gets no duplicate at 0°/360°,
    // and a partial arc ends one step inside so the spread stays symmetric about the arc centre.
    const auto AngleDeltaDeg = ArcAngle / static_cast<float>(NumPoints);
    const auto ArcStartDeg = -(ArcAngle * 0.5f);  // centred on querier's forward (+X = 0°)

    OutCandidates.Reserve(NumPoints);
    for (auto i = 0; i < NumPoints; ++i)
    {
        const auto Yaw = ArcStartDeg + AngleDeltaDeg * static_cast<float>(i);
        const auto Direction = FRotator{0.0f, Yaw, 0.0f}.Vector();
        const auto Location = InQuerierLocation + Direction * Radius;
        OutCandidates.Emplace(FCk_Eqs_Candidate{Location});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    DoRunTests(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        int32& InOutRemainingBudget)
    -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_RunTests);

    const auto& Tests = InParams.Get_Tests();
    const auto NumTests = Tests.Num();

    if (NumTests == 0)
    {
        InState._NextTestIndex = 0;
        return true;
    }

    const auto NumCandidates = InState._Candidates.Num();

    const auto DebugRowsNeedSizing = InOutDebug._PerCandidate.Num() != NumCandidates;
    if (DebugRowsNeedSizing)
    {
        InOutDebug._PerCandidate.Reset();
        InOutDebug._PerCandidate.SetNum(NumCandidates);
        for (auto& Row : InOutDebug._PerCandidate)
        { Row._PerTest.SetNum(NumTests); }
    }

    const auto MadeProgressBeforeEntry = InState._NextTestIndex > 0;
    auto MadeProgressThisCall = false;

    while (InState._NextTestIndex < NumTests)
    {
        const auto BudgetExhausted = InOutRemainingBudget < NumCandidates;
        const auto MadeAnyProgress = MadeProgressThisCall || MadeProgressBeforeEntry;
        if (BudgetExhausted && MadeAnyProgress)
        { return false; }

        const auto TestIdx = InState._NextTestIndex;
        const auto& Test = Tests[TestIdx];

        switch (Test.Get_TestType())
        {
            case ECk_Eqs_TestType::Distance:
                DoRunTest_Distance(Test, InParams, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::Dot:
                DoRunTest_Dot(Test, InParams, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::Trace:
                DoRunTest_Trace(Test, InParams, InParams.Get_Querier(), InPhysicsSystem, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::GameplayTag:
                DoRunTest_GameplayTag(Test, InQueryHandle, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::Overlap:
                DoRunTest_Overlap(Test, InParams.Get_Querier(), InPhysicsSystem, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::VolumeCheck:
                DoRunTest_VolumeCheck(Test, InState._Candidates, InOutDebug, TestIdx);
                break;
            case ECk_Eqs_TestType::Random:
                DoRunTest_Random(Test, InState._Candidates, InOutDebug, TestIdx);
                break;
            default:
                CK_TRIGGER_ENSURE(TEXT("EqsQuery [{}]: unknown TestType encountered (stale Blueprint asset?). Test skipped."), InQueryHandle);
                break;
        }

        InOutRemainingBudget -= NumCandidates;
        ++InState._NextTestIndex;
        MadeProgressThisCall = true;
    }

    InState._NextTestIndex = 0;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    FinishTest(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        const TArray<float>& InRawValues,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    const auto Purpose = InTest.Get_Purpose();
    const auto IncludesScore = Purpose == ECk_Eqs_TestPurpose::Score
                            || Purpose == ECk_Eqs_TestPurpose::FilterAndScore;
    const auto IncludesFilter = Purpose == ECk_Eqs_TestPurpose::Filter
                             || Purpose == ECk_Eqs_TestPurpose::FilterAndScore;

    auto Min = TNumericLimits<float>::Max();
    auto Max = TNumericLimits<float>::Lowest();
    auto SeenAny = false;

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        if (NOT InOutCandidates[i].Get_Passed())
        { continue; }
        const auto Raw = InRawValues[i];
        Min = FMath::Min(Min, Raw);
        Max = FMath::Max(Max, Raw);
        SeenAny = true;
    }

    if (NOT SeenAny)
    { return; }  // every candidate already failed; nothing to do

    const auto& ScoringConfig = InTest.Get_ScoringConfig();
    const auto& FilterConfig = InTest.Get_FilterConfig();
    const auto Weight = InTest.Get_Weight();

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        auto& Candidate = InOutCandidates[i];

        if (NOT Candidate.Get_Passed())
        { continue; }

        const auto Raw = InRawValues[i];

        auto& DebugSlot = InOutDebug._PerCandidate[i]._PerTest[InTestIndex];

        if (IncludesScore)
        {
            const auto Normalized = NormalizeAndScore(Raw, Min, Max, ScoringConfig);
            const auto Contribution = Normalized * Weight;
            Candidate._Score *= Contribution;
            DebugSlot._NormalizedScore = Normalized;
            DebugSlot._WeightedContribution = Contribution;
        }
        // else: leave _NormalizedScore=0, _WeightedContribution=1.0f (multiplicative identity).

        if (IncludesFilter)
        {
            if (NOT ck_eqs_algorithm::Eqs_PassesFilter(Raw, FilterConfig))
            {
                Candidate._Passed = false;
                DebugSlot._PassedThisTest = false;
            }
        }
    }
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_Distance(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_Distance);

    const auto RefLocation = ck_eqs_algorithm::Eqs_GetReferenceLocation(InParams, InTest.Get_DistanceTo());
    const auto Mode = InTest.Get_DistanceMode();

    auto RawValues = TArray<float>{};
    RawValues.Reserve(InOutCandidates.Num());

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        if (NOT InOutCandidates[i].Get_Passed())
        {
            RawValues.Add(0.0f);
            continue;
        }

        const auto& Loc = InOutCandidates[i].Get_Location();
        auto Raw = 0.0f;
        switch (Mode)
        {
            case ECk_Eqs_DistanceMode::Distance3D: Raw = FVector::Dist(Loc, RefLocation); break;
            case ECk_Eqs_DistanceMode::Distance2D: Raw = FVector::Dist2D(Loc, RefLocation); break;
            case ECk_Eqs_DistanceMode::DistanceZ:  Raw = FMath::Abs(Loc.Z - RefLocation.Z); break;
        }
        RawValues.Add(Raw);
        InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._RawValue = Raw;
    }

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_Dot(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_Dot);

    const auto FromLocation = ck_eqs_algorithm::Eqs_GetDotFromLocation(InParams, InTest.Get_DotFrom());
    const auto Forward = ck_eqs_algorithm::Eqs_GetDotFromForward(InParams, InTest.Get_DotFrom()).GetSafeNormal();
    const auto Mode = InTest.Get_DotMode();

    auto RawValues = TArray<float>{};
    RawValues.Reserve(InOutCandidates.Num());

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        if (NOT InOutCandidates[i].Get_Passed())
        {
            RawValues.Add(0.0f);
            continue;
        }

        const auto Direction = (InOutCandidates[i].Get_Location() - FromLocation).GetSafeNormal();
        auto Raw = FVector::DotProduct(Forward, Direction);
        if (Mode == ECk_Eqs_DotMode::NormalizedDot)
        { Raw = (Raw + 1.0f) * 0.5f; }
        RawValues.Add(Raw);
        InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._RawValue = Raw;
    }

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_Trace(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        const FCk_Handle& InAnyHandle,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_Trace);

    const auto PhysicsSystem = InPhysicsSystem.Pin();

    if (ck::Is_NOT_Valid(PhysicsSystem))
    {
        ck::eqs::Warning(TEXT("DoRunTest_Trace: physics unavailable; failing every candidate for this test."));
        for (auto i = 0; i < InOutCandidates.Num(); ++i)
        {
            InOutCandidates[i]._Passed = false;
            InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._PassedThisTest = false;
        }
        return;
    }

    const auto QuerierLocation = InParams.Get_Querier().Has<ck::FFragment_Transform>()
        ? InParams.Get_Querier().Get<ck::FFragment_Transform>().Get_Transform().GetLocation()
        : FVector::ZeroVector;

    const auto& TraceFilter = InTest.Get_TraceFilter();
    const auto Mode = InTest.Get_TraceMode();

    // EQS scores the world — its casts must not fire overlap events into hit probes.
    constexpr auto FireOverlaps = false;
    constexpr auto TryDrawDebug = false;

    auto RawValues = TArray<float>{};
    RawValues.SetNumZeroed(InOutCandidates.Num());

    // Same worker-thread contract as DoGenerate_Grid: read-only Jolt casts, per-index writes.
    const auto RegistryHandle = InAnyHandle.Get_RegistryView().Get_RegistryHandle();
#if !UE_BUILD_SHIPPING
    ck::registry_table::BeginParallelRegion(RegistryHandle);
#endif

    ParallelFor(InOutCandidates.Num(), [&](int32 InIndex)
    {
        const auto& Candidate = InOutCandidates[InIndex];
        if (NOT Candidate.Get_Passed())
        { return; }

        auto LosClear = false;
        switch (Mode)
        {
            case ECk_Eqs_TraceMode::LineTrace:
            {
                const auto Settings = FCk_Probe_RayCast_Settings{QuerierLocation, Candidate.Get_Location(), TraceFilter};
                INC_DWORD_STAT(STAT_Eqs_PhysicsCastsIssued);
                const auto Hits = UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(
                    InAnyHandle, Settings, FireOverlaps, TryDrawDebug, *PhysicsSystem);
                LosClear = Hits.IsEmpty();
                break;
            }
            case ECk_Eqs_TraceMode::ShapeTrace:
            {
                const auto Settings = FCk_ShapeCast_Settings{QuerierLocation, Candidate.Get_Location(), InTest.Get_TraceShape(), TraceFilter};
                INC_DWORD_STAT(STAT_Eqs_PhysicsCastsIssued);
                const auto Hits = UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(
                    InAnyHandle, Settings, FireOverlaps, TryDrawDebug, *PhysicsSystem);
                LosClear = Hits.IsEmpty();
                break;
            }
        }

        const auto Raw = LosClear ? 1.0f : 0.0f;
        RawValues[InIndex] = Raw;
        InOutDebug._PerCandidate[InIndex]._PerTest[InTestIndex]._RawValue = Raw;
    });

#if !UE_BUILD_SHIPPING
    ck::registry_table::EndParallelRegion(RegistryHandle);
#endif

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_GameplayTag(
        const FCk_Eqs_TestParams& InTest,
        FCk_Handle_EqsQuery InQueryHandle,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_GameplayTag);

    const auto Tag = InTest.Get_RequiredTag();
    const auto MatchMode = InTest.Get_TagMatchMode();
    const auto Weight = InTest.Get_Weight();

    if (NOT Tag.IsValid())
    {
        ck::eqs::Warning(TEXT("EqsQuery [{}]: GameplayTag test has invalid RequiredTag; failing all candidates."), InQueryHandle);
        for (auto i = 0; i < InOutCandidates.Num(); ++i)
        {
            InOutCandidates[i]._Passed = false;
            InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._PassedThisTest = false;
        }
        return;
    }

    auto AnyValidEntity = false;

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        auto& Candidate = InOutCandidates[i];
        auto& DebugSlot = InOutDebug._PerCandidate[i]._PerTest[InTestIndex];

        const auto& EntityHandle = Candidate.Get_EntityHandle();

        if (ck::Is_NOT_Valid(EntityHandle))
        {
            Candidate._Passed = false;
            DebugSlot._RawValue = 0.0f;          // no entity to query
            DebugSlot._PassedThisTest = false;
            continue;
        }

        AnyValidEntity = true;

        const auto HasTag = UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(EntityHandle, Tag);
        const auto Passed = MatchMode == ECk_Eqs_TagMatchMode::HasTag ? HasTag : NOT HasTag;

        DebugSlot._RawValue = HasTag ? 1.0f : 0.0f;  // raw = "did the entity have the tag?"

        if (NOT Passed)
        {
            Candidate._Passed = false;
            DebugSlot._PassedThisTest = false;
        }
        else
        {
            const auto Contribution = 1.0f * Weight;
            Candidate._Score *= Contribution;
            DebugSlot._NormalizedScore = 1.0f;
            DebugSlot._WeightedContribution = Contribution;
        }
    }

    if (NOT AnyValidEntity && NOT InOutCandidates.IsEmpty())
    {
        ck::eqs::Warning(TEXT("EqsQuery [{}]: GameplayTag test ran on a generator producing location-only candidates "
                              "(no entity handles). Test failed every candidate. Pair this test with EntitiesWithTag."),
            InQueryHandle);
    }
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_Overlap(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Handle& InAnyHandle,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_Overlap);

    const auto PhysicsSystem = InPhysicsSystem.Pin();

    if (ck::Is_NOT_Valid(PhysicsSystem))
    {
        ck::eqs::Warning(TEXT("DoRunTest_Overlap: physics unavailable; failing every candidate for this test."));
        for (auto i = 0; i < InOutCandidates.Num(); ++i)
        {
            InOutCandidates[i]._Passed = false;
            InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._PassedThisTest = false;
        }
        return;
    }

    const auto Radius = FMath::Max(InTest.Get_OverlapRadius(), 1.0f);
    const auto Sphere = FCk_AnyShape{FCk_ShapeSphere_Dimensions{Radius}};
    const auto& OverlapFilter = InTest.Get_OverlapFilter();

    // EQS scores the world — its casts must not fire overlap events into hit probes.
    constexpr auto FireOverlaps = false;
    constexpr auto TryDrawDebug = false;

    auto RawValues = TArray<float>{};
    RawValues.SetNumZeroed(InOutCandidates.Num());

    // Same worker-thread contract as DoGenerate_Grid: read-only Jolt casts, per-index writes.
    const auto RegistryHandle = InAnyHandle.Get_RegistryView().Get_RegistryHandle();
#if !UE_BUILD_SHIPPING
    ck::registry_table::BeginParallelRegion(RegistryHandle);
#endif

    ParallelFor(InOutCandidates.Num(), [&](int32 InIndex)
    {
        const auto& Candidate = InOutCandidates[InIndex];
        if (NOT Candidate.Get_Passed())
        { return; }

        const auto& Loc = Candidate.Get_Location();
        const auto Settings = FCk_ShapeCast_Settings{Loc, Loc, Sphere, OverlapFilter};
        INC_DWORD_STAT(STAT_Eqs_PhysicsCastsIssued);
        const auto Hits = UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(
            InAnyHandle, Settings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

        const auto Raw = static_cast<float>(Hits.Num());
        RawValues[InIndex] = Raw;
        InOutDebug._PerCandidate[InIndex]._PerTest[InTestIndex]._RawValue = Raw;
    });

#if !UE_BUILD_SHIPPING
    ck::registry_table::EndParallelRegion(RegistryHandle);
#endif

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_VolumeCheck(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_VolumeCheck);

    const auto Center = InTest.Get_VolumeCheck_WorldCenter();
    const auto HalfExtent = InTest.Get_VolumeCheck_HalfExtent();
    const auto Rotation = InTest.Get_VolumeCheck_WorldRotation();
    const auto Inverted = InTest.Get_VolumeCheck_Inverted();

    auto RawValues = TArray<float>{};
    RawValues.Reserve(InOutCandidates.Num());

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        if (NOT InOutCandidates[i].Get_Passed())
        {
            RawValues.Add(0.0f);
            continue;
        }

        const auto& Loc = InOutCandidates[i].Get_Location();
        const auto InsideBox = Get_IsPointInOrientedBox(Loc, Center, HalfExtent, Rotation);

        const auto Passes = Inverted ? NOT InsideBox : InsideBox;
        const auto Raw = Passes ? 1.0f : 0.0f;
        RawValues.Add(Raw);
        InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._RawValue = Raw;
    }

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

auto
    FCk_Eqs_Algorithm::
    Get_IsPointInOrientedBox(
        const FVector& InPoint,
        const FVector& InCenter,
        const FVector& InHalfExtent,
        const FRotator& InRotation)
    -> bool
{
    const auto Local = InRotation.UnrotateVector(InPoint - InCenter);
    return FMath::Abs(Local.X) <= InHalfExtent.X
        && FMath::Abs(Local.Y) <= InHalfExtent.Y
        && FMath::Abs(Local.Z) <= InHalfExtent.Z;
}

auto
    FCk_Eqs_Algorithm::
    DoRunTest_Random(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Test_Random);

    auto RawValues = TArray<float>{};
    RawValues.Reserve(InOutCandidates.Num());

    for (auto i = 0; i < InOutCandidates.Num(); ++i)
    {
        if (NOT InOutCandidates[i].Get_Passed())
        {
            RawValues.Add(0.0f);
            continue;
        }

        const auto Raw = FMath::FRand();
        RawValues.Add(Raw);
        InOutDebug._PerCandidate[i]._PerTest[InTestIndex]._RawValue = Raw;
    }

    FinishTest(InTest, InOutCandidates, RawValues, InOutDebug, InTestIndex);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Eqs_Algorithm::
    DoFinalize(
        FCk_Handle_EqsQuery /*InQueryHandle*/,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug)
    -> FCk_Eqs_QueryResults
{
    SCOPE_CYCLE_COUNTER(STAT_Eqs_Finalize);

    auto Results = FCk_Eqs_QueryResults{};
    auto& Final = Results._Candidates;

    struct FSurvivor
    {
        FCk_Eqs_Candidate Candidate;
        int32             OriginalIndex;
    };
    auto Survivors = TArray<FSurvivor>{};
    Survivors.Reserve(InState._Candidates.Num());
    for (auto i = 0; i < InState._Candidates.Num(); ++i)
    {
        if (InState._Candidates[i].Get_Passed())
        { Survivors.Emplace(FSurvivor{MoveTemp(InState._Candidates[i]), i}); }
    }

    Survivors.Sort([](const FSurvivor& A, const FSurvivor& B)
    {
        return A.Candidate.Get_Score() > B.Candidate.Get_Score();
    });

    // Truncation happens on Survivors so the OriginalIndex pairing stays in lockstep.
    switch (InParams.Get_RunMode())
    {
        case ECk_Eqs_RunMode::SingleBest:
        {
            if (Survivors.Num() > 1)
            { Survivors.SetNum(1); }
            break;
        }
        case ECk_Eqs_RunMode::AllMatching:
        case ECk_Eqs_RunMode::AllMatchingSorted:
            // AllMatching promises no ordering, so the sort above already satisfies both.
            break;
        case ECk_Eqs_RunMode::RandomBest5Pct:
        {
            if (Survivors.IsEmpty())
            { break; }
            const auto SliceCount = FMath::Max(1, Survivors.Num() * 5 / 100);
            const auto PickIndex = FMath::RandRange(0, SliceCount - 1);
            auto Picked = MoveTemp(Survivors[PickIndex]);
            Survivors.Reset();
            Survivors.Emplace(MoveTemp(Picked));
            break;
        }
        case ECk_Eqs_RunMode::RandomBest25Pct:
        {
            if (Survivors.IsEmpty())
            { break; }
            const auto SliceCount = FMath::Max(1, Survivors.Num() * 25 / 100);
            const auto PickIndex = FMath::RandRange(0, SliceCount - 1);
            auto Picked = MoveTemp(Survivors[PickIndex]);
            Survivors.Reset();
            Survivors.Emplace(MoveTemp(Picked));
            break;
        }
    }

    Final.Reserve(Survivors.Num());
    auto NewPerCandidate = TArray<FCk_Eqs_DebugInfo_PerCandidate>{};
    NewPerCandidate.Reserve(Survivors.Num());
    for (auto& S : Survivors)
    {
        // DoRunTests pre-sizes _PerCandidate to the original candidate count, so the index is in
        // range today; the guard only covers a future refactor that decouples the two.
        if (InOutDebug._PerCandidate.IsValidIndex(S.OriginalIndex))
        { NewPerCandidate.Emplace(MoveTemp(InOutDebug._PerCandidate[S.OriginalIndex])); }
        else
        { NewPerCandidate.Emplace(FCk_Eqs_DebugInfo_PerCandidate{}); }
        Final.Emplace(MoveTemp(S.Candidate));
    }
    InOutDebug._PerCandidate = MoveTemp(NewPerCandidate);

    Results._HasResults = NOT Final.IsEmpty();
    Results._BestLocation = Final.IsEmpty() ? FVector::ZeroVector : Final[0].Get_Location();
    Results._BestEntity   = Final.IsEmpty() ? FCk_Handle{}        : Final[0].Get_EntityHandle();

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
