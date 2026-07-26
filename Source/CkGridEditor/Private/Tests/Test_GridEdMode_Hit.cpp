// Corner-origin convention: cell (x,y) spans local [x*CellSize, (x+1)*CellSize); a world ray is
// intersected with the grid's local z=0 plane and floored into a cell, or rejected outside [0, Dimensions).

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode_Hit.h"
#include "CkGridEditor/Draw/Ck2dGridSystem_AuthoredOverlay.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

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

    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(550.0, 250.0, 1000.0), RayDown);

        if (! TestTrue(TEXT("In-bounds ray resolves to a cell"), Hit.IsSet()))
        { return false; }

        TestEqual(TEXT("In-bounds ray resolves to cell (5,2)"), Hit.GetValue(), FIntPoint(5, 2));
    }

    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(-50.0, -50.0, 1000.0), RayDown);

        TestFalse(TEXT("Out-of-bounds ray resolves to no cell"), Hit.IsSet());
    }

    {
        const auto Hit = ck::grid_editor::Resolve_CellFromRay(
            GridTransform, CellSize, Dimensions, FVector(550.0, 250.0, 1000.0), FVector(1.0, 0.0, 0.0));

        TestFalse(TEXT("Parallel ray resolves to no cell"), Hit.IsSet());
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GridEdMode_RectCells,
    "CkGrid.UnitTests.EdMode.RectCells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GridEdMode_RectCells::RunTest(const FString&)
{
    const auto Dimensions = FIntPoint(10, 10);

    // Corners in arbitrary order produce the same inclusive rectangle.
    {
        const auto Cells = ck::grid_editor::Compute_RectCells(FIntPoint(2, 1), FIntPoint(1, 2), Dimensions);
        TestEqual(TEXT("2x2 rect has 4 cells"), Cells.Num(), 4);
        TestTrue(TEXT("contains (1,1)"), Cells.Contains(FIntPoint(1, 1)));
        TestTrue(TEXT("contains (2,1)"), Cells.Contains(FIntPoint(2, 1)));
        TestTrue(TEXT("contains (1,2)"), Cells.Contains(FIntPoint(1, 2)));
        TestTrue(TEXT("contains (2,2)"), Cells.Contains(FIntPoint(2, 2)));
    }

    {
        const auto Cells = ck::grid_editor::Compute_RectCells(FIntPoint(5, 5), FIntPoint(5, 5), Dimensions);
        TestEqual(TEXT("1x1 rect has 1 cell"), Cells.Num(), 1);
        TestEqual(TEXT("1x1 rect is the clicked cell"), Cells[0], FIntPoint(5, 5));
    }

    {
        const auto Cells = ck::grid_editor::Compute_RectCells(FIntPoint(-3, -3), FIntPoint(1, 1), Dimensions);
        TestEqual(TEXT("clamped rect drops negative coords"), Cells.Num(), 4); // (0..1, 0..1)
        TestFalse(TEXT("no negative cell"), Cells.Contains(FIntPoint(-1, -1)));

        const auto FullyOut = ck::grid_editor::Compute_RectCells(FIntPoint(20, 20), FIntPoint(30, 30), Dimensions);
        TestEqual(TEXT("fully-out rect is empty"), FullyOut.Num(), 0);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GridEdMode_TagColor,
    "CkGrid.UnitTests.EdMode.TagColor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GridEdMode_TagColor::RunTest(const FString&)
{
    const auto A1 = ck::grid_editor::Resolve_TagColor_FromName(FName(TEXT("Grid.Cell.Spawn")));
    const auto A2 = ck::grid_editor::Resolve_TagColor_FromName(FName(TEXT("Grid.Cell.Spawn")));
    TestEqual(TEXT("same name -> same color"), A1, A2);

    // These two names are known to differ under the hash.
    const auto B = ck::grid_editor::Resolve_TagColor_FromName(FName(TEXT("Grid.Cell.Cover")));
    TestNotEqual(TEXT("different names -> different colors"), A1, B);

    TestTrue(TEXT("color is bright"), A1.GetLuminance() > 0.05f);

    const auto Fallback = ck::grid_editor::Resolve_TagColor(FGameplayTag{});
    TestEqual(TEXT("invalid tag -> legacy ColorTagged"), Fallback, ck::grid_editor::ColorTagged);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GridEdMode_TagEnumeration,
    "CkGrid.UnitTests.EdMode.TagEnumeration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GridEdMode_TagEnumeration::RunTest(const FString&)
{
    auto* Spec = NewObject<UCk_2dGridSystem_Spec>();
    if (! TestNotNull(TEXT("Spec allocated"), Spec))
    { return false; }
    Spec->Dimensions = FIntPoint(5, 5);

    const auto Tag = FGameplayTag::RequestGameplayTag(FName{TEXT("2dGridCell")}, /*ErrorIfNotFound*/ false);
    if (! Tag.IsValid())
    { return true; } // tag table lacks the category in this project config — nothing to assert

    auto Container = FGameplayTagContainer{};
    Container.AddTag(Tag);
    Spec->PerCellTags.Add(FIntPoint(1, 1), Container);
    Spec->PerCellTags.Add(FIntPoint(2, 3), Container);

    const auto WithCounts = ck::grid_editor::Collect_PerCellTagsWithCounts(Spec);
    const auto* Entry = WithCounts.FindByPredicate(
        [&](const TPair<FGameplayTag, int32>& In){ return In.Key == Tag; });
    if (TestNotNull(TEXT("tag present in counts"), Entry))
    { TestEqual(TEXT("tag covers 2 cells"), Entry->Value, 2); }

    const auto Cells = ck::grid_editor::Get_CellsWithTag(Spec, Tag);
    TestEqual(TEXT("two cells carry the tag"), Cells.Num(), 2);
    TestTrue(TEXT("contains (1,1)"), Cells.Contains(FIntPoint(1, 1)));
    TestTrue(TEXT("contains (2,3)"), Cells.Contains(FIntPoint(2, 3)));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
