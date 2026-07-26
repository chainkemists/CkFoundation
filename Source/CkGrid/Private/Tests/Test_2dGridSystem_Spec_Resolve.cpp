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

    constexpr auto ErrorIfNotFound = false;
    const auto Tag = FGameplayTag::RequestGameplayTag(FName{TEXT("2dGridCell")}, ErrorIfNotFound);
    if (Tag.IsValid())
    { Spec->DefaultCellTags.AddTag(Tag); }

    const auto Params = Spec->Resolve_GridParams();

    TestEqual(TEXT("DefaultCellState resolves to Enable"),
        Params.Get_DefaultCellState(), ECk_EnableDisable::Enable);

    TestTrue(TEXT("ExceptionCoordinates contains the disabled cell (2,2)"),
        Params.Get_ExceptionCoordinates().Contains(FIntPoint(2, 2)));

    TestEqual(TEXT("Dimensions propagated"), Params.Get_Dimensions(), FIntPoint(5, 5));

    if (Tag.IsValid())
    {
        TestTrue(TEXT("DefaultCellTags contains the authored tag"),
            Params.Get_DefaultCellTags().HasTagExact(Tag));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
