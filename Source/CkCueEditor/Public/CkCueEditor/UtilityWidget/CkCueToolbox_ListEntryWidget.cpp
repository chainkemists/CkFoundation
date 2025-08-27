#include "CkCueToolbox_ListEntryWidget.h"

#include "EditorAssetLibrary.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ToolMenus.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueToolbox_ListEntryWidget::
    NativeOnListItemObjectSet(
        UObject* ListItemObject) -> void
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    CueListEntry = Cast<UCk_CueToolbox_CueListEntry>(ListItemObject);

    // Bind button events
    if (ck::IsValid(EditButton))
    {
        EditButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_ListEntryWidget::OnEditClicked);
    }

    if (ck::IsValid(ExecuteButton))
    {
        ExecuteButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_ListEntryWidget::OnExecuteClicked);
    }

    if (ck::IsValid(LocateButton))
    {
        LocateButton->OnClicked.AddDynamic(this, &UCk_CueToolbox_ListEntryWidget::OnLocateClicked);
    }

    DoUpdateDisplay();
}

auto
    UCk_CueToolbox_ListEntryWidget::
    OnEditClicked() -> void
{
    if (ck::Is_NOT_Valid(CueListEntry))
    { return; }

    const auto MainWidget = DoGetMainWidget();

    if (ck::Is_NOT_Valid(MainWidget))
    { return; }

    MainWidget->OnCueEdit(CueListEntry->CueInfo);

    // Also try to open the blueprint directly
    const auto CueClass = CueListEntry->CueInfo.CueClass;

    if (ck::IsValid(CueClass))
    {
        TArray<UObject*> Assets = {CueClass->GetDefaultObject()};
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
    }
}

auto
    UCk_CueToolbox_ListEntryWidget::
    OnExecuteClicked() -> void
{
    if (ck::Is_NOT_Valid(CueListEntry))
    { return; }

    const auto MainWidget = DoGetMainWidget();

    if (ck::Is_NOT_Valid(MainWidget))
    { return; }

    MainWidget->OnCueExecute(CueListEntry->CueInfo);
}

auto
    UCk_CueToolbox_ListEntryWidget::
    OnLocateClicked() -> void
{
    if (ck::Is_NOT_Valid(CueListEntry))
    { return; }

    const auto MainWidget = DoGetMainWidget();

    if (ck::Is_NOT_Valid(MainWidget))
    { return; }

    MainWidget->OnCueLocate(CueListEntry->CueInfo);

    // Also sync Content Browser
    const auto CueClass = CueListEntry->CueInfo.CueClass;

    if (ck::IsValid(CueClass))
    {
        const auto Blueprint = UEditorAssetLibrary::FindAssetData(CueClass->GetPathName());

        if (Blueprint.IsValid())
        {
            const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            TArray<FAssetData> AssetsToSync = {Blueprint};
            ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
        }
    }
}

auto
    UCk_CueToolbox_ListEntryWidget::
    DoUpdateDisplay() -> void
{
    if (ck::Is_NOT_Valid(CueListEntry))
    { return; }

    const auto& CueInfo = CueListEntry->CueInfo;

    // Update cue name
    if (ck::IsValid(CueNameText))
    {
        const auto CueName = CueInfo.CueName.ToString();
        CueNameText->SetText(FText::FromString(CueName));
    }

    // Update validation status
    if (ck::IsValid(ValidationStatusText))
    {
        if (CueInfo.IsValid)
        {
            ValidationStatusText->SetText(FText::FromString(TEXT("✓")));
            ValidationStatusText->SetColorAndOpacity(FLinearColor::Green);
        }
        else
        {
            ValidationStatusText->SetText(FText::FromString(TEXT("❌")));
            ValidationStatusText->SetColorAndOpacity(FLinearColor::Red);
            ValidationStatusText->SetToolTipText(FText::FromString(CueInfo.ValidationMessage));
        }
    }

    // Enable/disable buttons based on validity
    const auto ButtonsEnabled = CueInfo.IsValid;

    if (ck::IsValid(EditButton))
    {
        EditButton->SetIsEnabled(ButtonsEnabled);
    }

    if (ck::IsValid(ExecuteButton))
    {
        ExecuteButton->SetIsEnabled(ButtonsEnabled);
    }

    if (ck::IsValid(LocateButton))
    {
        LocateButton->SetIsEnabled(ButtonsEnabled);
    }
}

auto
    UCk_CueToolbox_ListEntryWidget::
    DoGetMainWidget() -> UCk_CueToolbox_EditorUtilityWidget*
{
    // Walk up the widget tree to find the main toolbox widget
    auto CurrentWidget = GetParent();

    while (ck::IsValid(CurrentWidget))
    {
        // Cast through UObject to avoid unrelated type warning
        if (const auto AsObject = Cast<UObject>(CurrentWidget))
        {
            if (const auto MainWidget = Cast<UCk_CueToolbox_EditorUtilityWidget>(AsObject))
            {
                return MainWidget;
            }
        }

        // Walk up to the next parent
        if (const auto AsWidget = Cast<UWidget>(CurrentWidget))
        {
            CurrentWidget = AsWidget->GetParent();
        }
        else
        {
            break;
        }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
