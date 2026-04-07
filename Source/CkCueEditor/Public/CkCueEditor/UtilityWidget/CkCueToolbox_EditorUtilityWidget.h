#pragma once

#include "CkCueEditor/Toolbox/CkCueToolbox_Common.h"

#include "EditorUtilityWidget.h"

#include "CkCue/CkCueSubsystem_Base.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Engine/DataTable.h"

#include "CkCueToolbox_EditorUtilityWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCUEEDITOR_API UCk_CueToolbox_CueListEntry : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueToolbox_CueListEntry);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCk_CueToolbox_CueInfo CueInfo;

public:
    UCk_CueToolbox_CueListEntry() = default;
    UCk_CueToolbox_CueListEntry(const FCk_CueToolbox_CueInfo& InCueInfo) : CueInfo(InCueInfo) {}
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKCUEEDITOR_API UCk_CueToolbox_EditorUtilityWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueToolbox_EditorUtilityWidget);

protected:
    auto NativeConstruct() -> void override;

public:
    // Widget References - Set via Blueprint
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UComboBoxString> SubsystemComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UEditableTextBox> SearchTextBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UListView> CueListView;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UTextBlock> ValidationStatusText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> RefreshButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> CreateCueButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> RefreshAllButton;

    // Debug Panel
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ContextEntityTextBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> ExecuteContextButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> ExecuteGlobalButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> ExecuteLocalButton;

protected:
    UFUNCTION(BlueprintCallable)
    void OnSubsystemChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION(BlueprintCallable)
    void OnSearchTextChanged(const FText& Text);

    UFUNCTION(BlueprintCallable)
    void OnRefreshClicked();

    UFUNCTION(BlueprintCallable)
    void OnCreateCueClicked();

    UFUNCTION(BlueprintCallable)
    void OnRefreshAllClicked();

    UFUNCTION(BlueprintCallable)
    void OnExecuteContextClicked();

    UFUNCTION(BlueprintCallable)
    void OnExecuteGlobalClicked();

    UFUNCTION(BlueprintCallable)
    void OnExecuteLocalClicked();

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueEdit(const FCk_CueToolbox_CueInfo& CueInfo);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueExecute(const FCk_CueToolbox_CueInfo& CueInfo);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueLocate(const FCk_CueToolbox_CueInfo& CueInfo);

private:
    auto DoDiscoverSubsystems() -> void;
    auto DoRefreshCuesForCurrentSubsystem() -> void;
    auto DoValidateCues() -> void;
    auto DoApplySearchFilter() -> void;
    auto DoGetCurrentSubsystem() -> UCk_CueSubsystem_Base_UE*;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_CueToolbox_CueListEntry>> AllCueEntries;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_CueToolbox_CueListEntry>> FilteredCueEntries;

    UPROPERTY(Transient)
    TMap<FString, TWeakObjectPtr<UCk_CueSubsystem_Base_UE>> DiscoveredSubsystems;

    UPROPERTY(Transient)
    FString CurrentSearchText;

    // Validation counters
    UPROPERTY(Transient)
    int32 TotalCuesCount = 0;

    UPROPERTY(Transient)
    int32 DuplicateCuesCount = 0;

    UPROPERTY(Transient)
    int32 InvalidCuesCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------
