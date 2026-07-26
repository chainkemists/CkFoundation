#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Vector_HeadingAngleBetweenLocations_Cardinals,
    "Ck.CkCore.Math.HeadingAngleBetweenLocations_Cardinals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Vector_HeadingAngleBetweenLocations_Cardinals::RunTest(const FString&)
{
    const auto Observer = FVector::ZeroVector;

    TestTrue(TEXT("target +X (North) -> 0"),
        FMath::IsNearlyEqual(UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(Observer, FVector{100, 0, 0}), 0.0f, 0.01f));
    TestTrue(TEXT("target +Y (East) -> +90"),
        FMath::IsNearlyEqual(UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(Observer, FVector{0, 100, 0}), 90.0f, 0.01f));
    TestTrue(TEXT("target -Y (West) -> -90"),
        FMath::IsNearlyEqual(UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(Observer, FVector{0, -100, 0}), -90.0f, 0.01f));
    TestTrue(TEXT("target -X (South) -> |180|"),
        FMath::IsNearlyEqual(FMath::Abs(UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(Observer, FVector{-100, 0, 0})), 180.0f, 0.01f));

    TestTrue(TEXT("target +X with Z offset -> still 0"),
        FMath::IsNearlyEqual(UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(Observer, FVector{100, 0, 500}), 0.0f, 0.01f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Vector_CardinalAndOrdinal_ByHeading_Buckets,
    "Ck.CkCore.Math.CardinalAndOrdinal_ByHeading_Buckets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Vector_CardinalAndOrdinal_ByHeading_Buckets::RunTest(const FString&)
{
    constexpr auto Get_ClosestCardinalAndOrdinalDirection_ByHeading =
        [](float InHeadingDegrees) { return UCk_Utils_Vector2_UE::Get_ClosestCardinalAndOrdinalDirection_ByHeading(InHeadingDegrees); };

    TestEqual(TEXT("0 -> North"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(0.0f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("22.4 -> North (bucket edge)"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(22.4f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("22.6 -> NorthEast"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(22.6f), ECk_CardinalAndOrdinalDirection::NorthEast);
    TestEqual(TEXT("90 -> East"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(90.0f), ECk_CardinalAndOrdinalDirection::East);
    TestEqual(TEXT("180 -> South"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(180.0f), ECk_CardinalAndOrdinalDirection::South);
    TestEqual(TEXT("270 -> West"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(270.0f), ECk_CardinalAndOrdinalDirection::West);
    TestEqual(TEXT("337.4 -> NorthWest (bucket edge)"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(337.4f), ECk_CardinalAndOrdinalDirection::NorthWest);
    TestEqual(TEXT("337.6 -> North"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(337.6f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("-90 (denormalized) -> West"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(-90.0f), ECk_CardinalAndOrdinalDirection::West);
    TestEqual(TEXT("359 -> North"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(359.0f), ECk_CardinalAndOrdinalDirection::North);
    TestEqual(TEXT("720+45 -> NorthEast (denormalized)"), Get_ClosestCardinalAndOrdinalDirection_ByHeading(765.0f), ECk_CardinalAndOrdinalDirection::NorthEast);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
