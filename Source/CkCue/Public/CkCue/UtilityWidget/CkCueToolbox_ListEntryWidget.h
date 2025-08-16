#pragma once

#include "CkCueToolbox_EditorUtilityWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"

#include "CkCueToolbox_ListEntryWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCUE_API UCk_CueToolbox_ListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueToolbox_ListEntryWidget);

protected:
    auto NativeOnListItemObjectSet(UObject* ListItemObject) -> void override;

public:
    // Widget References - Set via Blueprint
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UTextBlock> CueNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UTextBlock> ValidationStatusText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> EditButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> ExecuteButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UButton> LocateButton;

protected:
    UFUNCTION()
    void OnEditClicked();

    UFUNCTION()
    void OnExecuteClicked();

    UFUNCTION()
    void OnLocateClicked();

private:
    auto DoUpdateDisplay() -> void;
    auto DoGetMainWidget() -> UCk_CueToolbox_EditorUtilityWidget*;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_CueToolbox_CueListEntry> CueListEntry;
};

// --------------------------------------------------------------------------------------------------------------------
