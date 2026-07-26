#include "CkMinimap/CkMinimap_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_test_minimap_utils
{
    auto Get_IsNearlyEqual(const FVector2D& InA, const FVector2D& InB, double InTolerance = 0.001) -> bool
    {
        return FMath::IsNearlyEqual(InA.X, InB.X, InTolerance) && FMath::IsNearlyEqual(InA.Y, InB.Y, InTolerance);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Minimap_WorldToFrame_AxisConvention,
    "Ck.CkMinimap.Math.WorldToFrame_AxisConvention",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Minimap_WorldToFrame_AxisConvention::RunTest(const FString&)
{
    using namespace ck_test_minimap_utils;

    const auto Origin = FVector::ZeroVector;
    constexpr auto Extent = 1000.0f;
    constexpr auto NorthLocked = ECk_Minimap_RotationMode::NorthLocked;

    TestTrue(TEXT("POI due North (+X) -> (0, -1) — screen up"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{1000, 0, 0}, Origin, 0.0f, Extent, NorthLocked), FVector2D{0, -1}));
    TestTrue(TEXT("POI due East (+Y) -> (1, 0) — screen right"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{0, 1000, 0}, Origin, 0.0f, Extent, NorthLocked), FVector2D{1, 0}));
    TestTrue(TEXT("POI due South (-X) -> (0, 1) — screen down"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{-1000, 0, 0}, Origin, 0.0f, Extent, NorthLocked), FVector2D{0, 1}));
    TestTrue(TEXT("POI due West (-Y) -> (-1, 0) — screen left"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{0, -1000, 0}, Origin, 0.0f, Extent, NorthLocked), FVector2D{-1, 0}));

    TestTrue(TEXT("half distance -> half offset"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{500, 0, 0}, Origin, 0.0f, Extent, NorthLocked), FVector2D{0, -0.5}));

    TestTrue(TEXT("Z never affects the projection"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{1000, 0, 12345}, Origin, 0.0f, Extent, NorthLocked), FVector2D{0, -1}));

    TestTrue(TEXT("non-positive extent degrades to (0, 0), no div-by-zero"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{1000, 0, 0}, Origin, 0.0f, 0.0f, NorthLocked), FVector2D::ZeroVector));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Minimap_WorldToFrame_RotationModes,
    "Ck.CkMinimap.Math.WorldToFrame_RotationModes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Minimap_WorldToFrame_RotationModes::RunTest(const FString&)
{
    using namespace ck_test_minimap_utils;

    const auto Origin = FVector::ZeroVector;
    constexpr auto Extent = 1000.0f;
    constexpr auto Rotate = ECk_Minimap_RotationMode::RotateWithObserver;
    constexpr auto NorthLocked = ECk_Minimap_RotationMode::NorthLocked;

    TestTrue(TEXT("RotateWithObserver: yaw 90, POI East (ahead) -> (0, -1)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{0, 1000, 0}, Origin, 90.0f, Extent, Rotate), FVector2D{0, -1}));

    TestTrue(TEXT("RotateWithObserver: yaw 90, POI North (left) -> (-1, 0)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{1000, 0, 0}, Origin, 90.0f, Extent, Rotate), FVector2D{-1, 0}));

    TestTrue(TEXT("RotateWithObserver: yaw 180, POI North (behind) -> (0, 1)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{1000, 0, 0}, Origin, 180.0f, Extent, Rotate), FVector2D{0, 1}));

    TestTrue(TEXT("NorthLocked: yaw 90 is ignored, POI East stays (1, 0)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_WorldToFrame(FVector{0, 1000, 0}, Origin, 90.0f, Extent, NorthLocked), FVector2D{1, 0}));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Minimap_FrameToWorld_RoundTrip,
    "Ck.CkMinimap.Math.FrameToWorld_RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Minimap_FrameToWorld_RoundTrip::RunTest(const FString&)
{
    using namespace ck_test_minimap_utils;

    const auto Origin = FVector{300, -700, 50};
    constexpr auto Extent = 2500.0f;

    const auto WorldPoints = TArray<FVector>{
        FVector{1200, 400, 0}, FVector{-900, -1300, 0}, FVector{300, -700, 0}, FVector{0, 2000, 0}};
    const auto Yaws = TArray<float>{0.0f, 37.0f, 90.0f, 180.0f, 271.5f};

    for (const auto& WorldPoint : WorldPoints)
    {
        for (const auto Yaw : Yaws)
        {
            for (const auto RotationMode : {ECk_Minimap_RotationMode::NorthLocked, ECk_Minimap_RotationMode::RotateWithObserver})
            {
                const auto Frame = UCk_Utils_Minimap_UE::Get_WorldToFrame(WorldPoint, Origin, Yaw, Extent, RotationMode);
                const auto RoundTripped = UCk_Utils_Minimap_UE::Get_FrameToWorld(Frame, Origin, Yaw, Extent, RotationMode);

                TestTrue(TEXT("world -> frame -> world round-trips to the same XY"),
                    Get_IsNearlyEqual(RoundTripped, FVector2D{WorldPoint.X, WorldPoint.Y}, 0.01));
            }
        }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Minimap_ClampToFrame_TotalAndShapes,
    "Ck.CkMinimap.Math.ClampToFrame_TotalAndShapes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Minimap_ClampToFrame_TotalAndShapes::RunTest(const FString&)
{
    using namespace ck_test_minimap_utils;

    constexpr auto Rect = ECk_Minimap_FrameShape::Rectangle;
    constexpr auto Circle = ECk_Minimap_FrameShape::Circle;

    TestTrue(TEXT("rect: (2, 0.5) -> (1, 0.25)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D{2, 0.5}, Rect), FVector2D{1, 0.25}));
    TestTrue(TEXT("circle: (3, 4) -> (0.6, 0.8)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D{3, 4}, Circle), FVector2D{0.6, 0.8}));

    TestTrue(TEXT("rect: inside point unchanged"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D{0.5, -0.25}, Rect), FVector2D{0.5, -0.25}));
    TestTrue(TEXT("circle: inside point unchanged"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D{0.3, 0.4}, Circle), FVector2D{0.3, 0.4}));
    TestTrue(TEXT("zero vector -> zero (both shapes)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D::ZeroVector, Rect), FVector2D::ZeroVector) &&
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_ClampToFrame(FVector2D::ZeroVector, Circle), FVector2D::ZeroVector));

    TestTrue(TEXT("rect: corner (1, 1) is INSIDE (edge-inclusive)"), UCk_Utils_Minimap_UE::Get_IsInsideFrame(FVector2D{1, 1}, Rect));
    TestTrue(TEXT("circle: rim (1, 0) is INSIDE (edge-inclusive)"), UCk_Utils_Minimap_UE::Get_IsInsideFrame(FVector2D{1, 0}, Circle));
    TestFalse(TEXT("circle: rect-corner-ish (0.8, 0.8) is OUTSIDE the circle"), UCk_Utils_Minimap_UE::Get_IsInsideFrame(FVector2D{0.8, 0.8}, Circle));
    TestFalse(TEXT("rect: (1.001, 0) is OUTSIDE"), UCk_Utils_Minimap_UE::Get_IsInsideFrame(FVector2D{1.001, 0}, Rect));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Minimap_Bounds_CornersAndInverse,
    "Ck.CkMinimap.Math.Bounds_CornersAndInverse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Minimap_Bounds_CornersAndInverse::RunTest(const FString&)
{
    using namespace ck_test_minimap_utils;

    const auto Bounds = FCk_Minimap_WorldBounds{FVector2D{1000, 2000}, FVector2D{500, 1000}};

    TestTrue(TEXT("center -> (0, 0)"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_BoundsToFrame(FVector{1000, 2000, 0}, Bounds), FVector2D::ZeroVector));
    TestTrue(TEXT("max-X/max-Y corner (north-east) -> (1, -1) — top-right"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_BoundsToFrame(FVector{1500, 3000, 0}, Bounds), FVector2D{1, -1}));
    TestTrue(TEXT("min-X/min-Y corner (south-west) -> (-1, 1) — bottom-left"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_BoundsToFrame(FVector{500, 1000, 0}, Bounds), FVector2D{-1, 1}));

    const auto FramePoints = TArray<FVector2D>{
        FVector2D{0, 0}, FVector2D{1, -1}, FVector2D{-0.5, 0.25}, FVector2D{0.75, 0.75}};

    for (const auto& FramePoint : FramePoints)
    {
        const auto World = UCk_Utils_Minimap_UE::Get_FrameToWorld_Bounds(FramePoint, Bounds);
        const auto RoundTripped = UCk_Utils_Minimap_UE::Get_BoundsToFrame(FVector{World.X, World.Y, 0}, Bounds);

        TestTrue(TEXT("frame -> world -> frame round-trips"),
            Get_IsNearlyEqual(RoundTripped, FramePoint, 0.001));
    }

    TestTrue(TEXT("invalid bounds -> (0, 0), no div-by-zero"),
        Get_IsNearlyEqual(UCk_Utils_Minimap_UE::Get_BoundsToFrame(FVector{123, 456, 0}, FCk_Minimap_WorldBounds{}), FVector2D::ZeroVector));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
