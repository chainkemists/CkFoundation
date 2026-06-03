// Unit test for UCk_2dGridSystem_Spec::Resolve_GridParams — verifies the authoring Spec folds its
// grid-level fields into the runtime params fragment: DisabledCells -> _ExceptionCoordinates (against
// DefaultCellState = Enable) and DefaultCellTags -> _DefaultCellTags. PerCellTags / Blockers are NOT
// part of the params fragment (the EntityScript applies those post-Add), so they are out of scope here.

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "CkCore/Enums/CkEnums.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_2dGridSystem_SpecResolve,
    "CkGrid.UnitTests.Authoring.SpecResolve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_2dGridSystem_SpecResolve::RunTest(const FString&)
{
    auto* Spec = NewObject<UCk_2dGridSystem_Spec>();
    if (! TestNotNull(TEXT("Spec allocated"), Spec))
    { return false; }

    Spec->Dimensions = FIntPoint(5, 5);
    Spec->CellSize = FVector2D(100.0f, 100.0f);
    Spec->DisabledCells = {FIntPoint(2, 2)};

    // Use an existing tag with ErrorIfNotFound=false so a missing tag does not trip an ensure — the
    // assertion on the tag is then guarded by whether it actually resolved.
    const auto Tag = FGameplayTag::RequestGameplayTag(FName{TEXT("2dGridCell")}, /*ErrorIfNotFound*/ false);
    if (Tag.IsValid())
    { Spec->DefaultCellTags.AddTag(Tag); }

    const auto Params = Spec->Resolve_GridParams();

    // Core invariant — DisabledCells become the exception coordinates against a default-Enable grid.
    TestEqual(TEXT("DefaultCellState resolves to Enable"),
        Params.Get_DefaultCellState(), ECk_EnableDisable::Enable);

    TestTrue(TEXT("ExceptionCoordinates contains the disabled cell (2,2)"),
        Params.Get_ExceptionCoordinates().Contains(FIntPoint(2, 2)));

    TestEqual(TEXT("Dimensions propagated"), Params.Get_Dimensions(), FIntPoint(5, 5));

    // Tag mapping — only asserted when the tag exists in the project's tag table.
    if (Tag.IsValid())
    {
        TestTrue(TEXT("DefaultCellTags contains the authored tag"),
            Params.Get_DefaultCellTags().HasTagExact(Tag));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
