#pragma once

#include "Blueprint/UserWidget.h"
#include "CkCue/CkCueSubsystem_Base.h"
#include "CkUI/Styles/CkCommonButton.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/PanelWidget.h"
#include "CommonTextBlock.h"

#include "CkCue/UtilityWidget/CkCueToolbox_EditorUtilityWidget.h"

#include "CkCueToolbox_EditorUtilityWidget_Programmatic.generated.h"

UCLASS(BlueprintType, Blueprintable)
class CKCUE_API UCkCueToolboxWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkCueToolboxWidget);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

public:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<USizeBox> ContentContainer;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UComboBoxString> SubsystemComboBox;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UEditableTextBox> SearchTextBox;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UListView> CueListView;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCommonTextBlock> ValidationStatusText;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Primary> RefreshButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Secondary> CreateCueButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Secondary> RefreshAllButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Secondary> OpenSettingsButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UEditableTextBox> ContextEntityTextBox;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UEditableTextBox> SpawnParamsTextBox;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Primary> ExecuteContextButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Primary> ExecuteGlobalButton;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UCkCommonButton_Primary> ExecuteLocalButton;

protected:
    UFUNCTION()
    void OnSubsystemChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);

    UFUNCTION()
    void OnRefreshClicked();

    UFUNCTION()
    void OnCreateCueClicked();

    UFUNCTION()
    void OnRefreshAllClicked();

    UFUNCTION()
    void OnOpenSettingsClicked();

    UFUNCTION()
    void OnExecuteContextClicked();

    UFUNCTION()
    void OnExecuteGlobalClicked();

    UFUNCTION()
    void OnExecuteLocalClicked();

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueEdit(const FCk_CueToolbox_CueInfo& CueInfo);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueExecute(const FCk_CueToolbox_CueInfo& CueInfo);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void OnCueLocate(const FCk_CueToolbox_CueInfo& CueInfo);

private:
    void Request_CreateLayout();
    auto Request_CreateSection(const FText& Title, UWidget* Content, const FLinearColor& AccentColor = FLinearColor::White) -> UWidget*;
    auto Request_CreateSubsystemControls() -> UWidget*;
    auto Request_CreateCuesControls() -> UWidget*;
    auto Request_CreateQuickActions() -> UWidget*;
    auto Request_CreateValidationDisplay() -> UWidget*;
    auto Request_CreateDebugPanel() -> UWidget*;

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

    UPROPERTY(Transient)
    int32 TotalCuesCount = 0;

    UPROPERTY(Transient)
    int32 DuplicateCuesCount = 0;

    UPROPERTY(Transient)
    int32 InvalidCuesCount = 0;
};
