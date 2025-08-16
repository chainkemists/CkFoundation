#include "CkCueToolbox_EditorUtilityWidget.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "ToolMenus.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueToolbox_EditorUtilityWidget::
    NativeConstruct() -> void
{
    Super::NativeConstruct();

    // Bind button events
    if (ck::IsValid(RefreshButton))
    {
        RefreshButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnRefreshClicked);
    }

    if (ck::IsValid(CreateCueButton))
    {
        CreateCueButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnCreateCueClicked);
    }

    if (ck::IsValid(RefreshAllButton))
    {
        RefreshAllButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnRefreshAllClicked);
    }

    if (ck::IsValid(ExecuteContextButton))
    {
        ExecuteContextButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnExecuteContextClicked);
    }

    if (ck::IsValid(ExecuteGlobalButton))
    {
        ExecuteGlobalButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnExecuteGlobalClicked);
    }

    if (ck::IsValid(ExecuteLocalButton))
    {
        ExecuteLocalButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnExecuteLocalClicked);
    }

    // Bind subsystem combo box
    if (ck::IsValid(SubsystemComboBox))
    {
        SubsystemComboBox->OnSelectionChanged.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnSubsystemChanged);
    }

    // Bind search text box
    if (ck::IsValid(SearchTextBox))
    {
        SearchTextBox->OnTextChanged.AddDynamic(this, &UCk_CueToolbox_EditorUtilityWidget::OnSearchTextChanged);
    }

    // Initial setup
    DoDiscoverSubsystems();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnSubsystemChanged(FString SelectedItem, ESelectInfo::Type SelectionType) -> void
{
    DoRefreshCuesForCurrentSubsystem();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnSearchTextChanged(
        const FText& Text) -> void
{
    CurrentSearchText = Text.ToString();
    DoApplySearchFilter();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnRefreshClicked() -> void
{
    DoRefreshCuesForCurrentSubsystem();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnCreateCueClicked() -> void
{
    // Blueprint implementable - open class creation dialog
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnRefreshAllClicked() -> void
{
    DoDiscoverSubsystems();
    DoRefreshCuesForCurrentSubsystem();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnExecuteContextClicked() -> void
{
    // Blueprint implementable - execute selected cue with context entity
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnExecuteGlobalClicked() -> void
{
    // Blueprint implementable - execute selected cue globally
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    OnExecuteLocalClicked() -> void
{
    // Blueprint implementable - execute selected cue locally
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    DoDiscoverSubsystems() -> void
{
    DiscoveredSubsystems.Empty();

    if (NOT ck::IsValid(SubsystemComboBox))
    { return; }

    SubsystemComboBox->ClearOptions();

    // Find all subsystem classes that inherit from UCk_CueSubsystem_Base_UE
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        const auto Class = *ClassIterator;

        if (NOT ck::IsValid(Class))
        { continue; }

        if (Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (NOT Class->IsChildOf<UCk_CueSubsystem_Base_UE>())
        { continue; }

        if (Class == UCk_CueSubsystem_Base_UE::StaticClass())
        { continue; }

        // Skip if this isn't actually a cue subsystem class
        const auto CueSubsystemClass = Cast<UClass>(Class);
        if (ck::Is_NOT_Valid(CueSubsystemClass))
        { continue; }

        const auto CueSubsystem = GEngine->GetEngineSubsystemBase(CueSubsystemClass);
        if (ck::Is_NOT_Valid(CueSubsystem))
        { continue; }

        const auto CastedCueSubsystem = Cast<UCk_CueSubsystem_Base_UE>(CueSubsystem);

        if (ck::Is_NOT_Valid(CastedCueSubsystem))
        { continue; }

        const auto& SubsystemName = Class->GetName();
        DiscoveredSubsystems.Add(SubsystemName, CastedCueSubsystem);
        SubsystemComboBox->AddOption(SubsystemName);
    }

    // Select first option if available
    if (SubsystemComboBox->GetOptionCount() > 0)
    {
        SubsystemComboBox->SetSelectedIndex(0);
        DoRefreshCuesForCurrentSubsystem();
    }
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    DoRefreshCuesForCurrentSubsystem() -> void
{
    AllCueEntries.Empty();

    const auto CurrentSubsystem = DoGetCurrentSubsystem();

    if (ck::Is_NOT_Valid(CurrentSubsystem))
    { return; }

    // Force refresh of the subsystem's cue discovery
    CurrentSubsystem->Request_PopulateAllCues();

    const auto& DiscoveredCues = CurrentSubsystem->Get_DiscoveredCues();
    const auto& SubsystemName = CurrentSubsystem->GetClass()->GetName();

    for (const auto& [CueName, CueClass] : DiscoveredCues)
    {
        const auto CueInfo = FCk_CueToolbox_CueInfo{CueName, CueClass, SubsystemName};
        const auto CueEntry = NewObject<UCk_CueToolbox_CueListEntry>(this, UCk_CueToolbox_CueListEntry::StaticClass());
        CueEntry->CueInfo = CueInfo;
        AllCueEntries.Add(CueEntry);
    }

    DoValidateCues();
    DoApplySearchFilter();
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    DoValidateCues() -> void
{
    TotalCuesCount = AllCueEntries.Num();
    DuplicateCuesCount = 0;
    InvalidCuesCount = 0;

    // Check for duplicate names
    TMap<FGameplayTag, int32> CueNameCounts;

    for (const auto& CueEntry : AllCueEntries)
    {
        if (ck::Is_NOT_Valid(CueEntry))
        { continue; }

        auto& CueInfo = CueEntry->CueInfo;
        CueInfo.IsValid = true;
        CueInfo.ValidationMessage.Empty();

        // Check if cue class is valid
        if (ck::Is_NOT_Valid(CueInfo.CueClass))
        {
            CueInfo.IsValid = false;
            CueInfo.ValidationMessage = TEXT("Invalid cue class");
            InvalidCuesCount++;
        }

        // Count occurrences of cue names
        CueNameCounts.FindOrAdd(CueInfo.CueName, 0)++;
    }

    // Mark duplicates
    for (const auto& CueEntry : AllCueEntries)
    {
        if (ck::Is_NOT_Valid(CueEntry))
        { continue; }

        auto& CueInfo = CueEntry->CueInfo;
        const auto* NameCount = CueNameCounts.Find(CueInfo.CueName);

        if (NameCount && *NameCount > 1)
        {
            CueInfo.IsValid = false;
            CueInfo.ValidationMessage += (CueInfo.ValidationMessage.IsEmpty() ? TEXT("") : TEXT("; ")) +
                FString::Printf(TEXT("Duplicate name (found %d times)"), *NameCount);

            if (DuplicateCuesCount == 0) // Count unique duplicate names, not instances
            {
                DuplicateCuesCount = CueNameCounts.Num() -
                    ck::algo::CountIf(CueNameCounts, [](const auto& Pair) { return Pair.Value == 1; });
            }
        }
    }

    // Update validation status text
    if (ck::IsValid(ValidationStatusText))
    {
        FString ValidationMessage = FString::Printf(TEXT("✓ %d cues discovered"), TotalCuesCount);

        if (DuplicateCuesCount > 0)
        {
            ValidationMessage += FString::Printf(TEXT("\n⚠ %d cues with duplicate names"), DuplicateCuesCount);
        }

        if (InvalidCuesCount > 0)
        {
            ValidationMessage += FString::Printf(TEXT("\n❌ %d cues with invalid class"), InvalidCuesCount);
        }

        ValidationStatusText->SetText(FText::FromString(ValidationMessage));
    }
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    DoApplySearchFilter() -> void
{
    FilteredCueEntries.Empty();

    if (CurrentSearchText.IsEmpty())
    {
        FilteredCueEntries = AllCueEntries;
    }
    else
    {
        const auto SearchTextLower = CurrentSearchText.ToLower();

        FilteredCueEntries = ck::algo::Filter(AllCueEntries,
        [&](const TObjectPtr<UCk_CueToolbox_CueListEntry>& CueEntry) -> bool
        {
            if (ck::Is_NOT_Valid(CueEntry))
            { return false; }

            const auto& CueInfo = CueEntry->CueInfo;
            const auto CueNameString = CueInfo.CueName.ToString().ToLower();

            return CueNameString.Contains(SearchTextLower);
        });
    }

    // Update list view
    if (ck::IsValid(CueListView))
    {
        CueListView->ClearListItems();

        for (const auto& CueEntry : FilteredCueEntries)
        {
            CueListView->AddItem(CueEntry);
        }

        CueListView->RegenerateAllEntries();
    }
}

auto
    UCk_CueToolbox_EditorUtilityWidget::
    DoGetCurrentSubsystem() -> UCk_CueSubsystem_Base_UE*
{
    if (ck::Is_NOT_Valid(SubsystemComboBox))
    { return nullptr; }

    const auto SelectedOption = SubsystemComboBox->GetSelectedOption();

    if (SelectedOption.IsEmpty())
    { return nullptr; }

    const auto* FoundSubsystem = DiscoveredSubsystems.Find(SelectedOption);

    return FoundSubsystem && FoundSubsystem->IsValid() ? FoundSubsystem->Get() : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
