#include "CkSubsystemBrowser/Model/CkSubsystemBrowser_Collector.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

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

    // Stock UEditorSubsystem present in every editor session — a plugin-coupling-free anchor.
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
        ck::subsystem_browser::Collect_Subsystems(Category);
        TestTrue(TEXT("Category has a display name"),
            ck::subsystem_browser::Get_CategoryDisplayName(Category).IsEmpty() == false);
    }

    return true;
}

#endif
