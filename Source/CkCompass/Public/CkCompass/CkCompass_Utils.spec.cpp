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

    TestFalse(TEXT("bearing 90 is NOT outside arc 180"), UCk_Utils_Compass_UE::Get_IsOutsideArc(90.0f, Arc180));
    TestTrue(TEXT("bearing 90.1 IS outside arc 180"), UCk_Utils_Compass_UE::Get_IsOutsideArc(90.1f, Arc180));
    TestTrue(TEXT("bearing 60 IS outside arc 90"), UCk_Utils_Compass_UE::Get_IsOutsideArc(60.0f, 90.0f));
    TestFalse(TEXT("bearing -30 is NOT outside arc 90"), UCk_Utils_Compass_UE::Get_IsOutsideArc(-30.0f, 90.0f));

    TestTrue(TEXT("zero arc -> offset 0, no div-by-zero"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_NormalizedArcOffset(45.0f, 0.0f), 0.0f, 0.001f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Compass_RangeFadeAlpha,
    "Ck.CkCompass.Math.RangeFadeAlpha",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Compass_RangeFadeAlpha::RunTest(const FString&)
{
    TestTrue(TEXT("zero band -> 1"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(1000.0f, 900.0f, 1100.0f, 0.0f), 1.0f, 0.001f));

    TestTrue(TEXT("at min boundary -> 0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(1000.0f, 1000.0f, 0.0f, 500.0f), 0.0f, 0.001f));
    TestTrue(TEXT("halfway through min band -> 0.5"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(1250.0f, 1000.0f, 0.0f, 500.0f), 0.5f, 0.001f));
    TestTrue(TEXT("past min band -> 1"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(2000.0f, 1000.0f, 0.0f, 500.0f), 1.0f, 0.001f));

    TestTrue(TEXT("well inside max -> 1"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(4000.0f, 0.0f, 5000.0f, 500.0f), 1.0f, 0.001f));
    TestTrue(TEXT("halfway through max band -> 0.5"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(4750.0f, 0.0f, 5000.0f, 500.0f), 0.5f, 0.001f));
    TestTrue(TEXT("at max boundary -> 0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(5000.0f, 0.0f, 5000.0f, 500.0f), 0.0f, 0.001f));

    TestTrue(TEXT("both active, equidistant narrow band -> min of ramps"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(1500.0f, 1000.0f, 2000.0f, 800.0f), 0.625f, 0.001f));

    TestTrue(TEXT("below min clamps to 0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(500.0f, 1000.0f, 0.0f, 500.0f), 0.0f, 0.001f));
    TestTrue(TEXT("beyond max clamps to 0"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(6000.0f, 0.0f, 5000.0f, 500.0f), 0.0f, 0.001f));

    TestTrue(TEXT("no ranges -> 1 even with a band"),
        FMath::IsNearlyEqual(UCk_Utils_Compass_UE::Get_RangeFadeAlpha(100.0f, 0.0f, 0.0f, 500.0f), 1.0f, 0.001f));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
