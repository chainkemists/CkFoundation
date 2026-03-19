#pragma once

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Input/SComboButton.h>
#include <Widgets/Views/SListView.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_OneParam(FOnStructTypeSelected, UScriptStruct* /* SelectedStruct */);

// --------------------------------------------------------------------------------------------------------------------

class CKEDITORGRAPH_API SStructTypeSelector_Widget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SStructTypeSelector_Widget) {}
        SLATE_EVENT(FOnStructTypeSelected, OnStructSelected)
        SLATE_ATTRIBUTE(UScriptStruct*, CurrentStruct)
        SLATE_ARGUMENT(UScriptStruct*, BaseStructFilter)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:
    using FStructItem = TSharedPtr<UScriptStruct*>;

    auto Get_DisplayText() const -> FText;
    auto OnStructSelected(UScriptStruct* InStruct) -> void;
    auto Get_MenuContent() -> TSharedRef<SWidget>;
    auto OnSearchTextChanged(const FText& InText) -> void;
    auto GatherStructs() -> void;
    auto UpdateFilteredItems() -> void;

    auto OnGenerateRow(FStructItem InItem, const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>;
    auto OnItemClicked(FStructItem InItem) -> void;

private:
    FOnStructTypeSelected _OnStructSelected;
    TAttribute<UScriptStruct*> _CurrentStruct;
    TObjectPtr<UScriptStruct> _BaseStructFilter;

    FString _SearchText;
    TArray<FStructItem> _AllItems;
    TArray<FStructItem> _FilteredItems;
    TSharedPtr<SListView<FStructItem>> _ListView;
};

// --------------------------------------------------------------------------------------------------------------------
