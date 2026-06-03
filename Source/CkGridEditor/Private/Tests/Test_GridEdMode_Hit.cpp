// Unit test for ck::grid_editor::Resolve_CellFromRay — verifies the pure ray->cell hit math used by
// the Grid Paint editor mode. Corner-origin convention: cell (x,y) spans local
// [x*CellSize, (x+1)*CellSize); a world ray is intersected with the grid's local z=0 plane and the
// hit is floored into a cell coordinate, or rejected if outside [0, Dimensions).

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode_Hit.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GridEdMode_RayToCell,
    "CkGrid.UnitTests.EdMode.RayToCell",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GridEdMode_RayToCell::RunTest(const FString&)
{
    const auto GridTransform = FTransform::Identity;
    const auto CellSize      = FVector2D(100.0f, 100.0f);
    const auto Dimensions    = FIntPoint(10, 10);
    const auto RayDown       = FVector(0.0, 0.0, -1.0);

    // A ray straight down through world (550,250) lands inside cell (5,2):
    // 550/100 -> 5, 250/100 -> 2.
    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(550.0, 250.0, 1000.0), RayDown);

        if (! TestTrue(TEXT("In-bounds ray resolves to a cell"), Hit.IsSet()))
        { return false; }

        TestEqual(TEXT("In-bounds ray resolves to cell (5,2)"), Hit.GetValue(), FIntPoint(5, 2));
    }

    // A ray straight down at world (-50,-50) is outside the grid (negative coordinates) -> unset.
    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(-50.0, -50.0, 1000.0), RayDown);

        TestFalse(TEXT("Out-of-bounds ray resolves to no cell"), Hit.IsSet());
    }

    // A ray parallel to the grid plane never intersects -> unset.
    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(550.0, 250.0, 1000.0), FVector(1.0, 0.0, 0.0));

        TestFalse(TEXT("Parallel ray resolves to no cell"), Hit.IsSet());
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
