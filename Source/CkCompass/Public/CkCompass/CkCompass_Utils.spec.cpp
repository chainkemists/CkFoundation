// Pure-math coverage for the compass arc helpers — no world, no entities.
//
// The generic angle math the compass composes with (heading-between-locations, cardinal buckets)
// lives in CkCore and is covered by CkVector_Utils.spec.cpp; the bearing delta is
// FMath::FindDeltaAngleDegrees (engine).

#include "CkCompass/CkCompass_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_NormalizedArcOffset_ClampEdges,
    "Ck.CkCompass.Math.NormalizedArcOffset_ClampEdges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_NormalizedArcOffset_ClampEdges::RunTest(const FString&)
{
    constexpr auto Arc180 = 180.0f;

    TestTrue(TEXT("bearing 0 -> offset 0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(0.0f, Arc180), 0.0f, 0.001f));
    TestTrue(TEXT("bearing 45 in arc 180 -> 0.5"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(45.0f, Arc180), 0.5f, 0.001f));
    TestTrue(TEXT("bearing 90 in arc 180 -> exactly the edge (1.0)"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(90.0f, Arc180), 1.0f, 0.001f));
    TestTrue(TEXT("bearing -90 in arc 180 -> -1.0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(-90.0f, Arc180), -1.0f, 0.001f));
    TestTrue(TEXT("bearing 135 in arc 180 -> clamped to 1.0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(135.0f, Arc180), 1.0f, 0.001f));

    // Edge-inclusive arc test: exactly the half-arc is INSIDE; beyond it is outside
    TestFalse(TEXT("bearing 90 is NOT outside arc 180"), UCk_Utils_Compass_UE::Get_IsOutsideArc(90.0f, Arc180));
    TestTrue(TEXT("bearing 90.1 IS outside arc 180"), UCk_Utils_Compass_UE::Get_IsOutsideArc(90.1f, Arc180));
    TestTrue(TEXT("bearing 60 IS outside arc 90"), UCk_Utils_Compass_UE::Get_IsOutsideArc(60.0f, 90.0f));
    TestFalse(TEXT("bearing -30 is NOT outside arc 90"), UCk_Utils_Compass_UE::Get_IsOutsideArc(-30.0f, 90.0f));

    // Degenerate arc never divides by zero
    TestTrue(TEXT("zero arc -> offset 0, no div-by-zero"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(45.0f, 0.0f), 0.0f, 0.001f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
