// Pure-math coverage for the compass projection helpers — no world, no entities.
//
// Convention under test (documented in CkBearing_Utils.h): heading/bearing are world yaws in degrees,
// 0 = North = +X, 90 = East = +Y; signed deltas are shortest-path in [-180, 180].

#include "CkCore/Math/Bearing/CkBearing_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_test_compass_math
{
    auto Get_LocationAtWorldYaw(float InWorldYawDegrees, float InDistance = 100.0f) -> FVector
    {
        const auto Radians = FMath::DegreesToRadians(InWorldYawDegrees);
        return FVector{FMath::Cos(Radians) * InDistance, FMath::Sin(Radians) * InDistance, 0.0f};
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_Bearing_SignedBearing_CardinalOffsets,
    "Ck.CkCompass.Bearing.SignedBearing_CardinalOffsets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_Bearing_SignedBearing_CardinalOffsets::RunTest(const FString&)
{
    const auto Observer = FVector::ZeroVector;

    // Heading 0 (facing +X / North)
    TestTrue(TEXT("target dead ahead -> 0"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{100, 0, 0}, 0.0f), 0.0f, 0.01f));
    TestTrue(TEXT("target East (+Y) -> +90"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{0, 100, 0}, 0.0f), 90.0f, 0.01f));
    TestTrue(TEXT("target West (-Y) -> -90"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{0, -100, 0}, 0.0f), -90.0f, 0.01f));
    TestTrue(TEXT("target behind -> |180|"),
        FMath::IsNearlyEqual(FMath::Abs(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{-100, 0, 0}, 0.0f)), 180.0f, 0.01f));

    // Heading 90 (facing +Y / East)
    TestTrue(TEXT("heading 90, target +Y -> 0"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{0, 100, 0}, 90.0f), 0.0f, 0.01f));
    TestTrue(TEXT("heading 90, target +X -> -90"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, FVector{100, 0, 0}, 90.0f), -90.0f, 0.01f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_Bearing_SignedBearing_WrapAround,
    "Ck.CkCompass.Bearing.SignedBearing_WrapAround",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_Bearing_SignedBearing_WrapAround::RunTest(const FString&)
{
    using namespace ck_test_compass_math;

    const auto Observer = FVector::ZeroVector;

    // Wrap across 0/360: heading 350, target at world yaw 10 -> shortest path is +20, never -340
    TestTrue(TEXT("heading 350 vs target yaw 10 -> +20"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, Get_LocationAtWorldYaw(10.0f), 350.0f), 20.0f, 0.01f));

    TestTrue(TEXT("heading 10 vs target yaw 350 -> -20"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, Get_LocationAtWorldYaw(350.0f), 10.0f), -20.0f, 0.01f));

    TestTrue(TEXT("heading -10 (denormalized) vs target yaw 10 -> +20"),
        FMath::IsNearlyEqual(ck::bearing::Get_SignedBearingDegrees(Observer, Get_LocationAtWorldYaw(10.0f), -10.0f), 20.0f, 0.01f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_Bearing_NormalizedArcOffset_ClampEdges,
    "Ck.CkCompass.Bearing.NormalizedArcOffset_ClampEdges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_Bearing_NormalizedArcOffset_ClampEdges::RunTest(const FString&)
{
    constexpr auto Arc180 = 180.0f;

    TestTrue(TEXT("bearing 0 -> offset 0"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(0.0f, Arc180), 0.0f, 0.001f));
    TestTrue(TEXT("bearing 45 in arc 180 -> 0.5"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(45.0f, Arc180), 0.5f, 0.001f));
    TestTrue(TEXT("bearing 90 in arc 180 -> exactly the edge (1.0)"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(90.0f, Arc180), 1.0f, 0.001f));
    TestTrue(TEXT("bearing -90 in arc 180 -> -1.0"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(-90.0f, Arc180), -1.0f, 0.001f));
    TestTrue(TEXT("bearing 135 in arc 180 -> clamped to 1.0"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(135.0f, Arc180), 1.0f, 0.001f));

    // Edge-inclusive arc test: exactly the half-arc is INSIDE; beyond it is outside
    TestFalse(TEXT("bearing 90 is NOT outside arc 180"), ck::bearing::Get_IsOutsideArc(90.0f, Arc180));
    TestTrue(TEXT("bearing 90.1 IS outside arc 180"), ck::bearing::Get_IsOutsideArc(90.1f, Arc180));
    TestTrue(TEXT("bearing 60 IS outside arc 90"), ck::bearing::Get_IsOutsideArc(60.0f, 90.0f));
    TestFalse(TEXT("bearing -30 is NOT outside arc 90"), ck::bearing::Get_IsOutsideArc(-30.0f, 90.0f));

    // Degenerate arc never divides by zero
    TestTrue(TEXT("zero arc -> offset 0, no div-by-zero"),
        FMath::IsNearlyEqual(ck::bearing::Get_NormalizedArcOffset(45.0f, 0.0f), 0.0f, 0.001f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_Bearing_CardinalAndOrdinal_Buckets,
    "Ck.CkCompass.Bearing.CardinalAndOrdinal_Buckets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_Bearing_CardinalAndOrdinal_Buckets::RunTest(const FString&)
{
    using ck::bearing::Get_CardinalAndOrdinalDirection;

    TestEqual(TEXT("0 -> North"), Get_CardinalAndOrdinalDirection(0.0f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("22.4 -> North (bucket edge)"), Get_CardinalAndOrdinalDirection(22.4f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("22.6 -> NorthEast"), Get_CardinalAndOrdinalDirection(22.6f), ECk_CardinalAndOrdinalDirection::NorthEast);
    TestEqual(TEXT("90 -> East"), Get_CardinalAndOrdinalDirection(90.0f), ECk_CardinalAndOrdinalDirection::East);
    TestEqual(TEXT("180 -> South"), Get_CardinalAndOrdinalDirection(180.0f), ECk_CardinalAndOrdinalDirection::South);
    TestEqual(TEXT("270 -> West"), Get_CardinalAndOrdinalDirection(270.0f), ECk_CardinalAndOrdinalDirection::West);
    TestEqual(TEXT("337.4 -> NorthWest (bucket edge)"), Get_CardinalAndOrdinalDirection(337.4f), ECk_CardinalAndOrdinalDirection::NorthWest);
    TestEqual(TEXT("337.6 -> North"), Get_CardinalAndOrdinalDirection(337.6f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("-90 (denormalized) -> West"), Get_CardinalAndOrdinalDirection(-90.0f), ECk_CardinalAndOrdinalDirection::West);
    TestEqual(TEXT("359 -> North"), Get_CardinalAndOrdinalDirection(359.0f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("720+45 -> NorthEast (denormalized)"), Get_CardinalAndOrdinalDirection(765.0f), ECk_CardinalAndOrdinalDirection::NorthEast);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS