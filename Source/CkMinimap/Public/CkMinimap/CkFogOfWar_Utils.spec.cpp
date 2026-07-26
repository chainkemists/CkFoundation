#include "CkMinimap/CkFogOfWar_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_test_fog_of_war_utils
{
    auto Get_IsNearlyEqual(const FVector2D& InA, const FVector2D& InB, double InTolerance = 0.001) -> bool
    {
        return FMath::IsNearlyEqual(InA.X, InB.X, InTolerance) && FMath::IsNearlyEqual(InA.Y, InB.Y, InTolerance);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FogOfWar_CellMath_IndexEdgesAndUV,
    "Ck.CkMinimap.Math.CellMath_IndexEdgesAndUV",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FogOfWar_CellMath_IndexEdgesAndUV::RunTest(const FString&)
{
    using namespace ck_test_fog_of_war_utils;

    const auto Bounds = FCk_Minimap_WorldBounds{FVector2D{0, 0}, FVector2D{1000, 500}};
    constexpr auto CellSize = 300.0f;

    // ceil(2000/300) = 7, ceil(1000/300) = 4 — the grid overshoots the bounds (partial edge cells)
    const auto Counts = UCk_Utils_FogOfWar_UE::Get_CellCounts(Bounds, CellSize);
    TestTrue(TEXT("counts = 7 x 4"), Counts == FIntPoint{7, 4});

    TestTrue(TEXT("degenerate cell size -> 1x1"), UCk_Utils_FogOfWar_UE::Get_CellCounts(Bounds, 0.0f) == FIntPoint{1, 1});

    TestEqual(TEXT("min corner -> index 0"), UCk_Utils_FogOfWar_UE::Get_CellIndex(FVector{-1000, -500, 0}, Bounds, CellSize, Counts), 0);
    TestEqual(TEXT("max corner -> last index (inclusive, clamped floor)"),
        UCk_Utils_FogOfWar_UE::Get_CellIndex(FVector{1000, 500, 0}, Bounds, CellSize, Counts), 3 * 7 + 6);
    TestEqual(TEXT("just outside -> INDEX_NONE"), UCk_Utils_FogOfWar_UE::Get_CellIndex(FVector{1001, 0, 0}, Bounds, CellSize, Counts), INDEX_NONE);
    TestEqual(TEXT("on the min-X boundary -> included"),
        UCk_Utils_FogOfWar_UE::Get_CellIndex(FVector{-1000, 0, 0}, Bounds, CellSize, Counts), FMath::FloorToInt32(500.0 / CellSize) * 7 + 0);

    TestTrue(TEXT("cell (0,0) center = min + half-cell"),
        Get_IsNearlyEqual(UCk_Utils_FogOfWar_UE::Get_CellCenter(0, 0, Bounds, CellSize), FVector2D{-850, -350}));

    const auto UV_FirstCell = UCk_Utils_FogOfWar_UE::Get_CellUVRect(0, Bounds, CellSize);
    TestTrue(TEXT("cell (0,0) UV = [0, 0.3] x [0.85, 1] (bottom-left of the map)"),
        Get_IsNearlyEqual(UV_FirstCell.Min, FVector2D{0.0, 0.85}) && Get_IsNearlyEqual(UV_FirstCell.Max, FVector2D{0.3, 1.0}));

    const auto UV_LastCellX = UCk_Utils_FogOfWar_UE::Get_CellUVRect(6, Bounds, CellSize);  // CellX = 6, CellY = 0 — partial along world X
    TestTrue(TEXT("partial edge cell V clamps to 0 (no seam past the texture)"),
        FMath::IsNearlyEqual(UV_LastCellX.Min.Y, 0.0, 0.001) && FMath::IsNearlyEqual(UV_LastCellX.Max.Y, 0.1, 0.001));

    const auto UV_OutOfRange = UCk_Utils_FogOfWar_UE::Get_CellUVRect(7 * 4, Bounds, CellSize);
    TestTrue(TEXT("out-of-range index -> empty rect"),
        Get_IsNearlyEqual(UV_OutOfRange.Min, FVector2D::ZeroVector) && Get_IsNearlyEqual(UV_OutOfRange.Max, FVector2D::ZeroVector));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
