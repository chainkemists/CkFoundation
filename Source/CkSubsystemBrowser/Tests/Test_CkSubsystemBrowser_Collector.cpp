// Tests for the subsystem collector seam — verifies that live subsystem
// enumeration finds instances per category without coupling to any specific
// plugin's subsystem. The Slate tab itself is not auto-tested (manual gate).

#include "CkSubsystemBrowser/Model/CkSubsystemBrowser_Collector.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Engine and Editor categories always have live instances in an editor session.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_SubsystemBrowser_Collector_EngineAndEditor,
    "CkSubsystemBrowser.UnitTests.Collector.EngineAndEditor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_SubsystemBrowser_Collector_EngineAndEditor::RunTest(const FString&)
{
    const auto EngineSubsystems = ck::subsystem_browser::Collect_Subsystems(ECk_SubsystemCategory::Engine);
    TestTrue(TEXT("Engine subsystems are discovered"), EngineSubsystems.Num() > 0);

    const auto EditorSubsystems = ck::subsystem_browser::Collect_Subsystems(ECk_SubsystemCategory::Editor);
    TestTrue(TEXT("Editor subsystems are discovered"), EditorSubsystems.Num() > 0);

    // AssetEditorSubsystem is a stock UEditorSubsystem present in every editor
    // session — a coupling-free anchor proving the Editor enumeration works.
    const auto bFoundStockEditorSubsystem = EditorSubsystems.ContainsByPredicate(
        [](const TWeakObjectPtr<UObject>& InWeak)
        {
            const auto* Object = InWeak.Get();
            return Object != nullptr && Object->GetClass()->GetName() == TEXT("AssetEditorSubsystem");
        });

    TestTrue(TEXT("Editor category contains the stock AssetEditorSubsystem"), bFoundStockEditorSubsystem);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Every category is enumerable without crashing, even with no active PIE world
// (World/GameInstance/LocalPlayer simply return empty).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_SubsystemBrowser_Collector_AllCategoriesSafe,
    "CkSubsystemBrowser.UnitTests.Collector.AllCategoriesSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_SubsystemBrowser_Collector_AllCategoriesSafe::RunTest(const FString&)
{
    const auto Categories = ck::subsystem_browser::Get_AllCategories();
    TestEqual(TEXT("All five subsystem categories are present"), Categories.Num(), 5);

    for (const auto Category : Categories)
    {
        // Must not crash; result count is non-negative by construction. We assert
        // the display name is non-empty as a cheap sanity check per category.
        ck::subsystem_browser::Collect_Subsystems(Category);
        TestTrue(TEXT("Category has a display name"),
            ck::subsystem_browser::Get_CategoryDisplayName(Category).IsEmpty() == false);
    }

    return true;
}

#endif
