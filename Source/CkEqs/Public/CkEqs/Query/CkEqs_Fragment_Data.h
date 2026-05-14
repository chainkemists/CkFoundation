#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapes/CkShapes_Common.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkEqs_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// CkEqs — Environmental Query System (v1.1)
//
// All reflected USTRUCTs for the EQS module live here. Tags, signals, and non-reflected
// fragments live in CkEqs_Fragment.h. Algorithm helpers in CkEqs_Algorithm.h.
//
// v1.1 ships: OnCircle generator, VolumeCheck + Random tests, _ProjectOntoNav generator flag.
// PathCost and Reachability (async fan-out) remain deferred — see the plan's Pass 5 banner.
// --------------------------------------------------------------------------------------------------------------------

struct FCk_Eqs_Algorithm;
class  UCk_Utils_Eqs_UE;
namespace ck
{
    class FProcessor_Eqs_HandleRequests;
    class FProcessor_Eqs_Generate;
    class FProcessor_Eqs_Test;
    class FProcessor_Eqs_Finalize;
    class FProcessor_Eqs_Cleanup;
}

// --------------------------------------------------------------------------------------------------------------------
// Handle
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKEQS_API FCk_Handle_EqsQuery : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EqsQuery);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EqsQuery);

// --------------------------------------------------------------------------------------------------------------------
// Enums
// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Eqs_GeneratorType : uint8
{
    SimpleGrid,           // Flat 2D grid around querier
    Grid,                 // 3D grid with ground projection via line trace
    Donut,                // Ring/annulus
    Cone,                 // Forward cone projection
    EntitiesWithTag,      // All entities carrying a specified FGameplayTag via CkEntityTag
    OnCircle              // Points evenly distributed on a circle arc around the querier
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_GeneratorType);

UENUM(BlueprintType)
enum class ECk_Eqs_TestType : uint8
{
    Distance,
    Dot,
    Trace,          // Line-of-sight via CkSpatialQuery
    GameplayTag,    // Entity must have/not-have tag (EntitiesWithTag generator only)
    Overlap,        // Point-overlap count at candidate position via CkSpatialQuery shape cast
    VolumeCheck,    // AABB containment filter — pass/fail based on world-space box
    Random          // Assigns FMath::FRand() score; useful for random tie-breaking
    // v1.2: PathCost / Reachability — needs CkEqs<->CkNav async fan-out design (N candidates ->
    // N async Request_FindPath -> aggregate completions -> continue scoring). Do NOT add as
    // direct FindPathSync calls; CkNavigation/Claude.md:133 flags that as an anti-pattern.
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_TestType);

UENUM(BlueprintType)
enum class ECk_Eqs_TestPurpose : uint8
{
    Filter,
    Score,
    FilterAndScore
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_TestPurpose);

UENUM(BlueprintType)
enum class ECk_Eqs_ScoringEquation : uint8
{
    // F4 verified against UE EnvQueryTest.cpp:119-151. UE's "Sine" does NOT exist (review-level
    // invention). Final set matches UE parity:
    Linear,         // y = x
    Square,         // y = x * x  (emphasises high values)
    SquareRoot,     // y = sqrt(x)  (diminishing-returns scoring)
    InverseLinear,  // y = 1 - x
    Constant        // Binary all-or-nothing: y = (NormalizedScore > 0) ? Weight : 0
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_ScoringEquation);

// F5: ECk_Eqs_ClampType drives WHERE the clamp threshold comes from.
UENUM(BlueprintType)
enum class ECk_Eqs_ClampType : uint8
{
    None,             // No clamp - use raw min/max from the candidate set
    FilterThreshold,  // Use _FilterMin / _FilterMax from the test's FilterConfig as the clamp
    SpecifiedValue    // Use _ClampMin / _ClampMax explicit values on the ScoringConfig
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_ClampType);

// F5: ECk_Eqs_NormalizationType drives the normalization baseline.
UENUM(BlueprintType)
enum class ECk_Eqs_NormalizationType : uint8
{
    Absolute,        // Baseline is 0; range = [0, max]
    RelativeToScores // Baseline is the data's actual min; range = [min, max]
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_NormalizationType);

UENUM(BlueprintType)
enum class ECk_Eqs_RunMode : uint8
{
    SingleBest,
    AllMatching,
    AllMatchingSorted,
    RandomBest5Pct,
    RandomBest25Pct
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_RunMode);

UENUM(BlueprintType)
enum class ECk_Eqs_DistanceTo : uint8
{
    Querier,
    Context  // Uses Params._Context if valid, otherwise falls back to Querier
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_DistanceTo);

UENUM(BlueprintType)
enum class ECk_Eqs_DistanceMode : uint8
{
    Distance3D,
    Distance2D,
    DistanceZ
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_DistanceMode);

UENUM(BlueprintType)
enum class ECk_Eqs_DotFrom : uint8
{
    Querier,
    Context
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_DotFrom);

UENUM(BlueprintType)
enum class ECk_Eqs_DotMode : uint8
{
    AbsoluteDot,
    NormalizedDot
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_DotMode);

UENUM(BlueprintType)
enum class ECk_Eqs_TraceMode : uint8
{
    LineTrace,
    ShapeTrace
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_TraceMode);

UENUM(BlueprintType)
enum class ECk_Eqs_TagMatchMode : uint8
{
    HasTag,
    DoesNotHaveTag
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_TagMatchMode);

UENUM(BlueprintType)
enum class ECk_Eqs_FilterType : uint8
{
    Minimum,
    Maximum,
    Range
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Eqs_FilterType);

// --------------------------------------------------------------------------------------------------------------------
// Scoring Config
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_ScoringConfig
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_ScoringConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_ScoringEquation _ScoringEquation = ECk_Eqs_ScoringEquation::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ScoringFactor = 1.0f;

    // F5: split from prior monolithic _NormalizationMode/_ClampMode.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_NormalizationType _NormalizationType = ECk_Eqs_NormalizationType::Absolute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_ClampType _ClampMinType = ECk_Eqs_ClampType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_ClampType _ClampMaxType = ECk_Eqs_ClampType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              EditCondition = "_ClampMinType == ECk_Eqs_ClampType::SpecifiedValue"))
    float _ClampMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              EditCondition = "_ClampMaxType == ECk_Eqs_ClampType::SpecifiedValue"))
    float _ClampMax = 1.0f;

    // F6 RESERVED for v1.1: reference-value scoring. v1 IGNORES this field.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              ToolTip = "Reserved for v1.1 reference-value scoring. Ignored in v1."))
    float _ReferenceValue = 0.0f;

public:
    CK_PROPERTY(_ScoringEquation);
    CK_PROPERTY(_ScoringFactor);
    CK_PROPERTY(_NormalizationType);
    CK_PROPERTY(_ClampMinType);
    CK_PROPERTY(_ClampMaxType);
    CK_PROPERTY(_ClampMin);
    CK_PROPERTY(_ClampMax);
    CK_PROPERTY(_ReferenceValue);
};

// --------------------------------------------------------------------------------------------------------------------
// Filter Config
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_FilterConfig
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_FilterConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_FilterType _FilterType = ECk_Eqs_FilterType::Minimum;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FilterMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FilterMax = 1.0f;

public:
    CK_PROPERTY(_FilterType);
    CK_PROPERTY(_FilterMin);
    CK_PROPERTY(_FilterMax);
};

// --------------------------------------------------------------------------------------------------------------------
// Generator Params
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_GeneratorParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_GeneratorParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_GeneratorType _GeneratorType = ECk_Eqs_GeneratorType::SimpleGrid;

    // SimpleGrid / Grid
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _SpaceBetween = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _GridHalfSize = 500.0f;

    // Grid ground projection
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ProjectDown = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ProjectUp = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _ProjectionFilter;

    // Donut
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _InnerRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _OuterRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _NumRings = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _PointsPerRing = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _ArcDirection = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ArcAngle = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _UseSpiralPattern = false;

    // Cone (F3 — UE-parity parameterization).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              ClampMin = "0.0", ClampMax = "359.0"))
    float _ConeDegrees = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _ConeRange = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              ClampMin = "1.0", ClampMax = "359.0"))
    float _ConeAngleStep = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _ConePointSpacing = 100.0f;

    // EntitiesWithTag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "EntityTag"))
    FGameplayTag _RequiredEntityTag;

    // OnCircle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _CircleRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "2"))
    int32 _NumPointsOnCircle = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              ClampMin = "1.0", ClampMax = "360.0"))
    float _CircleArcAngle = 360.0f;

    // NavProjection — applies after any generator emits candidates; EntitiesWithTag is exempt.
    // When true, each candidate is snapped to the navmesh within _NavProjectionSearchHalfExtentUu.
    // Candidates that fail projection are dropped before tests run.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ProjectOntoNav = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _NavProjectionSearchHalfExtentUu = 500.0f;

public:
    CK_PROPERTY(_GeneratorType);
    CK_PROPERTY(_SpaceBetween);
    CK_PROPERTY(_GridHalfSize);
    CK_PROPERTY(_ProjectDown);
    CK_PROPERTY(_ProjectUp);
    CK_PROPERTY(_ProjectionFilter);
    CK_PROPERTY(_InnerRadius);
    CK_PROPERTY(_OuterRadius);
    CK_PROPERTY(_NumRings);
    CK_PROPERTY(_PointsPerRing);
    CK_PROPERTY(_ArcDirection);
    CK_PROPERTY(_ArcAngle);
    CK_PROPERTY(_UseSpiralPattern);
    CK_PROPERTY(_ConeDegrees);
    CK_PROPERTY(_ConeRange);
    CK_PROPERTY(_ConeAngleStep);
    CK_PROPERTY(_ConePointSpacing);
    CK_PROPERTY(_RequiredEntityTag);
    CK_PROPERTY(_CircleRadius);
    CK_PROPERTY(_NumPointsOnCircle);
    CK_PROPERTY(_CircleArcAngle);
    CK_PROPERTY(_ProjectOntoNav);
    CK_PROPERTY(_NavProjectionSearchHalfExtentUu);
};

// --------------------------------------------------------------------------------------------------------------------
// Test Params
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_TestParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_TestParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_TestType _TestType = ECk_Eqs_TestType::Distance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_TestPurpose _Purpose = ECk_Eqs_TestPurpose::FilterAndScore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Eqs_ScoringConfig _ScoringConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Eqs_FilterConfig _FilterConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Weight = 1.0f;

    // Distance test
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_DistanceTo _DistanceTo = ECk_Eqs_DistanceTo::Querier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_DistanceMode _DistanceMode = ECk_Eqs_DistanceMode::Distance3D;

    // Dot test
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_DotFrom _DotFrom = ECk_Eqs_DotFrom::Querier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_DotMode _DotMode = ECk_Eqs_DotMode::NormalizedDot;

    // Trace test
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _TraceFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_TraceMode _TraceMode = ECk_Eqs_TraceMode::LineTrace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnyShape _TraceShape;

    // GameplayTag test
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "EntityTag"))
    FGameplayTag _RequiredTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_TagMatchMode _TagMatchMode = ECk_Eqs_TagMatchMode::HasTag;

    // Overlap test
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _OverlapRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _OverlapFilter;

    // VolumeCheck test — AABB containment in world space.
    // When Inverted=false, candidates INSIDE the box pass; when true, candidates OUTSIDE pass.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _VolumeCheck_WorldCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _VolumeCheck_HalfExtent = FVector(100.0f, 100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _VolumeCheck_Inverted = false;

    // v1.2 reserved: _PathCostAllowPartial (async fan-out design required).

public:
    CK_PROPERTY(_TestType);
    CK_PROPERTY(_Purpose);
    CK_PROPERTY(_ScoringConfig);
    CK_PROPERTY(_FilterConfig);
    CK_PROPERTY(_Weight);
    CK_PROPERTY(_DistanceTo);
    CK_PROPERTY(_DistanceMode);
    CK_PROPERTY(_DotFrom);
    CK_PROPERTY(_DotMode);
    CK_PROPERTY(_TraceFilter);
    CK_PROPERTY(_TraceMode);
    CK_PROPERTY(_TraceShape);
    CK_PROPERTY(_RequiredTag);
    CK_PROPERTY(_TagMatchMode);
    CK_PROPERTY(_OverlapRadius);
    CK_PROPERTY(_OverlapFilter);
    CK_PROPERTY(_VolumeCheck_WorldCenter);
    CK_PROPERTY(_VolumeCheck_HalfExtent);
    CK_PROPERTY(_VolumeCheck_Inverted);
};

// --------------------------------------------------------------------------------------------------------------------
// Query Params
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_QueryParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_QueryParams);

private:
    // Declaration order matches CK_DEFINE_CONSTRUCTORS arg order below so initializer order
    // and the public constructor signature agree (otherwise: C5038).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Querier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Eqs_GeneratorParams _GeneratorParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_Eqs_TestParams> _Tests;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Eqs_RunMode _RunMode = ECk_Eqs_RunMode::SingleBest;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Context;  // Optional - may be invalid

public:
    CK_PROPERTY(_Querier);
    CK_PROPERTY(_GeneratorParams);
    CK_PROPERTY(_Tests);
    CK_PROPERTY(_RunMode);
    CK_PROPERTY(_Context);

public:
    // Pass-4 (CTO sign-off #4): _GeneratorParams and _Tests are essential. Single-arg
    // construction silently produced a default-SimpleGrid, zero-test query that returned
    // a candidate at index 0 with score 1.0 - silently-wrong consumer behavior. Tests is
    // mandatory at construction; pass {} only if you know you want a generator-only
    // candidate dump (rare; almost certainly a bug - HandleRequests logs a Warning if
    // _Tests.IsEmpty() at runtime).
    CK_DEFINE_CONSTRUCTORS(FCk_Eqs_QueryParams, _Querier, _GeneratorParams, _Tests);
};

// --------------------------------------------------------------------------------------------------------------------
// Candidate
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_Candidate
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_Candidate);

    friend struct FCk_Eqs_Algorithm;
    friend class  UCk_Utils_Eqs_UE;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _EntityHandle;  // Only valid for EntitiesWithTag generator

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Score = 1.0f;       // Multiplicative accumulator; starts at 1.0f

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _Passed = true;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_EntityHandle);
    CK_PROPERTY_GET(_Score);
    CK_PROPERTY_GET(_Passed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Eqs_Candidate, _Location);
};

// --------------------------------------------------------------------------------------------------------------------
// Query Results
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_QueryResults
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_QueryResults);

    friend struct FCk_Eqs_Algorithm;
    friend class  UCk_Utils_Eqs_UE;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_Eqs_Candidate> _Candidates;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _BestLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _BestEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasResults = false;

public:
    CK_PROPERTY_GET(_Candidates);
    CK_PROPERTY_GET(_BestLocation);
    CK_PROPERTY_GET(_BestEntity);
    CK_PROPERTY_GET(_HasResults);
};

// --------------------------------------------------------------------------------------------------------------------
// Per-Test Debug Info (Pass-5.1) — captured per (test, candidate) during DoRunTests so the
// debugger can show why a candidate scored where it did. Always-on (no project-setting gate)
// because EQS is server-only with bounded query counts; ~12 bytes/test/candidate is negligible.
// Mirrors CkCrowd's FCk_Fragment_CrowdAgent_DiagRecorderData — additional-fragment-for-debug
// is the canonical CkFoundation pattern.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_PerTestDebugInfo
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_PerTestDebugInfo);

    friend struct FCk_Eqs_Algorithm;
    friend class  UCk_Utils_Eqs_UE;

private:
    // Raw value the test produced for this candidate (distance in cm, dot in [-1,1] / [0,1],
    // 0 or 1 for trace LOS, hit-count for overlap, etc.). Test-type-specific units; debugger
    // displays as float without interpretation.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _RawValue = 0.0f;

    // NormalizeAndScore output (post-equation, post-clamp, post-_ScoringFactor). Range
    // depends on _ScoringFactor; nominally [0, _ScoringFactor]. Zero for non-Score-purpose
    // tests and for tests skipped via the F7 degenerate-min-equals-max path.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _NormalizedScore = 0.0f;

    // What this test actually multiplied into _Score (= _NormalizedScore * _Weight). 1.0f
    // for tests that didn't contribute to score (Filter-only purpose, or skipped under F7).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _WeightedContribution = 1.0f;

    // false if this test's filter rejected this candidate. Tests that are Score-only never
    // set this to false. Filter failure does NOT short-circuit later tests — the candidate
    // continues to be tested and scored, and Finalize drops it. (Pass-3 contract.)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _PassedThisTest = true;

public:
    CK_PROPERTY_GET(_RawValue);
    CK_PROPERTY_GET(_NormalizedScore);
    CK_PROPERTY_GET(_WeightedContribution);
    CK_PROPERTY_GET(_PassedThisTest);
};

// Per-candidate row of test results. `_PerTest` is parallel-indexed to InParams._Tests:
// `_PerTest[TestIndex]` is the breakdown for test N on this candidate. Sized once at the
// start of DoRunTests; entries are written in place as tests run.
USTRUCT(BlueprintType)
struct CKEQS_API FCk_Eqs_DebugInfo_PerCandidate
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Eqs_DebugInfo_PerCandidate);

    friend struct FCk_Eqs_Algorithm;
    friend class  UCk_Utils_Eqs_UE;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_Eqs_PerTestDebugInfo> _PerTest;

public:
    CK_PROPERTY_GET(_PerTest);
};

// Top-level debug fragment data. Lives on the query entity alongside Params/State/Results.
// Pre-Finalize: indexed in original generation order matching FFragment_EqsQuery_State._Candidates.
// Post-Finalize: parallel-indexed to FFragment_EqsQuery_Results._Candidates (DoFinalize reorders
// _PerCandidate in lockstep with the candidate drop/sort/truncate so debugger lookup by results
// index Just Works).
USTRUCT(BlueprintType)
struct CKEQS_API FCk_Fragment_EqsQuery_DebugInfoData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_EqsQuery_DebugInfoData);

    friend struct FCk_Eqs_Algorithm;
    friend class  UCk_Utils_Eqs_UE;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_Eqs_DebugInfo_PerCandidate> _PerCandidate;

public:
    CK_PROPERTY_GET(_PerCandidate);
};

// --------------------------------------------------------------------------------------------------------------------
// Delegate
// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EqsQuery_OnComplete,
    FCk_Handle_EqsQuery, InHandle,
    FCk_Eqs_QueryResults, InResults);

// --------------------------------------------------------------------------------------------------------------------
// Request Struct
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKEQS_API FCk_Request_Eqs_RunQuery : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Eqs_RunQuery);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Eqs_RunQuery);

    friend class ck::FProcessor_Eqs_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Eqs_QueryParams _QueryParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _AutoDestroy = true;

    // Optional completion delegate; bound on entity creation via CK_SIGNAL_BIND_REQUEST_FULFILLED
    // (auto-unbind after first fire). Invalid/unbound delegate is fine - no binding happens.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Delegate_EqsQuery_OnComplete _OnComplete;

public:
    CK_PROPERTY_GET(_QueryParams);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_AutoDestroy);
    CK_PROPERTY(_OnComplete);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Eqs_RunQuery, _QueryParams);
};

// --------------------------------------------------------------------------------------------------------------------
