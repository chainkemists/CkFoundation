#include "CkStructTypeSelector_Widget.h"

#include <UObject/UObjectIterator.h>

#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Styling/AppStyle.h>

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::Construct(
    const FArguments& InArgs) -> void
{
    _OnStructSelected = InArgs._OnStructSelected;
    _CurrentStruct = InArgs._CurrentStruct;
    _BaseStructFilter = InArgs._BaseStructFilter;

    GatherStructs();

    ChildSlot
    [
        SNew(SComboButton)
        .ButtonContent()
        [
            SNew(STextBlock)
            .Text(this, &SStructTypeSelector_Widget::Get_DisplayText)
            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
        ]
        .OnGetMenuContent(this, &SStructTypeSelector_Widget::Get_MenuContent)
    ];
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::Get_DisplayText() const -> FText
{
    auto* Current = _CurrentStruct.Get();

    if (Current == nullptr)
    { return FText::FromString(TEXT("Select Struct...")); }

    return Current->GetDisplayNameText();
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::OnStructSelected(UScriptStruct* InStruct) -> void
{
    _OnStructSelected.ExecuteIfBound(InStruct);

    FSlateApplication::Get().DismissAllMenus();
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::GatherStructs() -> void
{
    _AllItems.Empty();

    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        auto* Struct = *It;

        if (_BaseStructFilter != nullptr)
        {
            if (Struct == _BaseStructFilter)
            { continue; }

            if (NOT Struct->IsChildOf(_BaseStructFilter))
            { continue; }
        }

        _AllItems.Add(MakeShared<UScriptStruct*>(Struct));
    }

    _AllItems.Sort([](const FStructItem& A, const FStructItem& B)
    {
        return (*A)->GetDisplayNameText().CompareTo((*B)->GetDisplayNameText()) < 0;
    });

    _FilteredItems = _AllItems;
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::UpdateFilteredItems() -> void
{
    if (_SearchText.IsEmpty())
    {
        _FilteredItems = _AllItems;
    }
    else
    {
        _FilteredItems.Empty();

        for (const auto& Item : _AllItems)
        {
            if ((*Item)->GetDisplayNameText().ToString().Contains(_SearchText))
            {
                _FilteredItems.Add(Item);
            }
        }
    }

    if (_ListView.IsValid())
    {
        _ListView->RequestListRefresh();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::OnSearchTextChanged(const FText& InText) -> void
{
    _SearchText = InText.ToString();
    UpdateFilteredItems();
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::OnGenerateRow(
    FStructItem InItem,
    const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>
{
    auto* Struct = *InItem;

    return SNew(STableRow<FStructItem>, InOwnerTable)
        .Padding(FMargin(4.0f, 2.0f))
        [
            SNew(STextBlock)
            .Text(Struct->GetDisplayNameText())
            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::OnItemClicked(FStructItem InItem) -> void
{
    OnStructSelected(*InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto SStructTypeSelector_Widget::Get_MenuContent() -> TSharedRef<SWidget>
{
    _SearchText.Empty();
    _FilteredItems = _AllItems;

    _ListView = SNew(SListView<FStructItem>)
        .ListItemsSource(&_FilteredItems)
        .OnGenerateRow(this, &SStructTypeSelector_Widget::OnGenerateRow)
        .OnMouseButtonClick(this, &SStructTypeSelector_Widget::OnItemClicked)
        .SelectionMode(ESelectionMode::Single);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("Menu.Background")))
        .Padding(2.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(250.0f)
            .MaxDesiredHeight(300.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.0f)
                [
                    SNew(SSearchBox)
                    .OnTextChanged(this, &SStructTypeSelector_Widget::OnSearchTextChanged)
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    _ListView.ToSharedRef()
                ]
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------
