#include "CkCVar_RefWidget.h"

#include "CkCVar/Settings/CkCVar_Settings.h"

#include "Misc/TextFilter.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Input/SComboButton.h"

#include <HAL/IConsoleManager.h>

#define LOCTEXT_NAMESPACE "SCVarRefWidget"

// --------------------------------------------------------------------------------------------------------------------

// Use FString instead of FName for CVar names — console variable names from
// IConsoleManager can exceed FName length limits or contain edge-case characters.
DECLARE_DELEGATE_OneParam(FOnCVarPicked, const FString&);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

// --------------------------------------------------------------------------------------------------------------------

struct FCVarViewerNode
{
public:
    explicit FCVarViewerNode(const FString& InName)
        : _Name(InName)
    {}

public:
    auto Get_Name() const -> const FString& { return _Name; }

private:
    FString _Name;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class SCVarItem : public SComboRow<TSharedPtr<FCVarViewerNode>>
    {
    public:
        SLATE_BEGIN_ARGS(SCVarItem)
            : _HighlightText()
            , _TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        {}

        SLATE_ARGUMENT(FText, HighlightText)
        SLATE_ARGUMENT(FSlateColor, TextColor)
        SLATE_ARGUMENT(TSharedPtr<FCVarViewerNode>, AssociatedNode)

        SLATE_END_ARGS()

    public:
        auto Construct(
            const FArguments& InArgs,
            const TSharedRef<STableViewBase>& InOwnerTableView)
            -> void
        {
            AssociatedNode = InArgs._AssociatedNode;

            const auto DisplayName = AssociatedNode.IsValid()
                ? FText::FromString(AssociatedNode->Get_Name())
                : FText::GetEmpty();

            this->ChildSlot
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(0.0f, 3.0f, 6.0f, 3.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(DisplayName)
                            .HighlightText(InArgs._HighlightText)
                            .ColorAndOpacity(this, &SCVarItem::GetTextColor)
                            .IsEnabled(true)
                        ]
                ];

            TextColor = InArgs._TextColor;

            ConstructInternal(
                STableRow::FArguments()
                .ShowSelection(true),
                InOwnerTableView
                );
        }

        auto
            GetTextColor() const
            -> FSlateColor
        {
            const auto OwnerWidget = OwnerTablePtr.Pin();
            if (NOT OwnerWidget.IsValid())
            {
                return TextColor;
            }

            const auto* MyItem = OwnerWidget->Private_ItemFromWidget(this);
            if (MyItem != nullptr && OwnerWidget->Private_IsItemSelected(*MyItem))
            {
                return FSlateColor::UseForeground();
            }

            return TextColor;
        }

    private:
        FSlateColor TextColor;
        TSharedPtr<FCVarViewerNode> AssociatedNode;
    };

    // -----------------------------------------------------------------------------------------------------------------

    class SCVarListWidget : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SCVarListWidget) {}

        SLATE_ARGUMENT(FString, FilterMetaData)
        SLATE_ARGUMENT(FOnCVarPicked, OnCVarPickedDelegate)

        SLATE_END_ARGS()

    public:
        virtual ~SCVarListWidget();

    public:
        auto Construct(
            const FArguments& InArgs) -> void;

    private:
        typedef TTextFilter<const FString&> FCVarTextFilter;

        auto OnFilterTextChanged(
            const FText& InFilterText) -> void;

        auto OnGenerateRowForCVarViewer(
            TSharedPtr<FCVarViewerNode> Item,
            const TSharedRef<STableViewBase>& InOwnerTable) const -> TSharedRef<ITableRow>;

        auto OnCVarSelectionChanged(
            TSharedPtr<FCVarViewerNode> Item,
            ESelectInfo::Type SelectInfo) const -> void;

        auto UpdatePropertyOptions() -> void;

    private:
        FOnCVarPicked _OnCVarPicked;
        TSharedPtr<SSearchBox> _SearchBox;
        TSharedPtr<SListView<TSharedPtr<FCVarViewerNode>>> _CVarList;
        TArray<TSharedPtr<FCVarViewerNode>> _PropertyOptions;
        TSharedPtr<FCVarTextFilter> _CVarTextFilter;
        FString _FilterMetaData;
    };

    // -----------------------------------------------------------------------------------------------------------------

    SCVarListWidget::
        ~SCVarListWidget()
    {
        if (_OnCVarPicked.IsBound())
        {
            _OnCVarPicked.Unbind();
        }
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarListWidget::
        Construct(
            const FArguments& InArgs)
        -> void
    {
        struct Local
        {
            static void CVarToStringArray(const FString& InName, OUT TArray<FString>& OutStringArray)
            {
                OutStringArray.Add(InName);
            }
        };

        _FilterMetaData = InArgs._FilterMetaData;
        _OnCVarPicked = InArgs._OnCVarPickedDelegate;

        _CVarTextFilter = MakeShareable(new FCVarTextFilter(
            FCVarTextFilter::FItemToStringArray::CreateStatic(&Local::CVarToStringArray)));

        UpdatePropertyOptions();

        auto ListContent = TSharedPtr<SWidget>{};

        SAssignNew(ListContent, SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew(_SearchBox, SSearchBox)
                .HintText(NSLOCTEXT("CVar", "SearchBoxHint", "Search Console Variables"))
                .OnTextChanged(this, &SCVarListWidget::OnFilterTextChanged)
                .DelayChangeNotificationsWhileTyping(true)
            ]

        + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SSeparator)
                .Visibility(EVisibility::Collapsed)
            ]

        + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SAssignNew(_CVarList, SListView<TSharedPtr<FCVarViewerNode>>)
                .Visibility(EVisibility::Visible)
                .SelectionMode(ESelectionMode::Single)
                .ListItemsSource(&_PropertyOptions)
                .OnGenerateRow(this, &SCVarListWidget::OnGenerateRowForCVarViewer)
                .OnSelectionChanged(this, &SCVarListWidget::OnCVarSelectionChanged)
            ];

        ChildSlot
            [
                ListContent.ToSharedRef()
            ];
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarListWidget::
        OnGenerateRowForCVarViewer(
            TSharedPtr<FCVarViewerNode> Item,
            const TSharedRef<STableViewBase>& InOwnerTable) const
        -> TSharedRef<ITableRow>
    {
        const auto HighlightText = _SearchBox.IsValid()
            ? _SearchBox->GetText()
            : FText::GetEmpty();

        return SNew(SCVarItem, InOwnerTable)
            .HighlightText(HighlightText)
            .TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.f))
            .AssociatedNode(Item);
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarListWidget::
        UpdatePropertyOptions()
        -> void
    {
        _PropertyOptions.Empty();

        // Gather all CVar names as FStrings (not FNames — CVar names can exceed FName limits)
        auto AllNames = TSet<FString>{};

        // 1. From IConsoleManager (engine + all registered CVars)
        IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
            FConsoleObjectVisitor::CreateLambda(
                [&AllNames](const TCHAR* InName, IConsoleObject* InObj)
                {
                    if (InObj == nullptr || InObj->TestFlags(ECVF_Unregistered))
                    {
                        return;
                    }

                    // Include both console variables and console commands
                    if (InObj->AsVariable() == nullptr && InObj->AsCommand() == nullptr)
                    {
                        return;
                    }

                    AllNames.Add(FString{InName});
                }),
            TEXT(""));

        // 2. From our persistent settings
        for (const auto& Name : UCk_CVar_Settings_UE::Get()->GetAllRegisteredNames())
        {
            AllNames.Add(Name.ToString());
        }

        // Sort
        auto SortedNames = AllNames.Array();
        SortedNames.Sort([](const FString& InA, const FString& InB)
        {
            return InA < InB;
        });

        // Add empty entry at the top for "None" selection
        _PropertyOptions.Add(MakeShareable(new FCVarViewerNode(FString{})));

        // Filter and add
        for (const auto& Name : SortedNames)
        {
            if (_CVarTextFilter.IsValid() && NOT _CVarTextFilter->PassesFilter(Name))
            {
                continue;
            }

            _PropertyOptions.Add(MakeShareable(new FCVarViewerNode{Name}));
        }
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarListWidget::
        OnFilterTextChanged(
            const FText& InFilterText)
        -> void
    {
        if (NOT _CVarTextFilter.IsValid())
        {
            return;
        }

        _CVarTextFilter->SetRawFilterText(InFilterText);

        if (_SearchBox.IsValid())
        {
            _SearchBox->SetError(_CVarTextFilter->GetFilterErrorText());
        }

        UpdatePropertyOptions();

        if (_CVarList.IsValid())
        {
            _CVarList->RequestListRefresh();
        }
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarListWidget::
        OnCVarSelectionChanged(
            TSharedPtr<FCVarViewerNode> Item,
            ESelectInfo::Type SelectInfo) const
        -> void
    {
        if (NOT Item.IsValid())
        {
            return;
        }

        std::ignore = _OnCVarPicked.ExecuteIfBound(Item->Get_Name());
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarRef_Widget::
        Construct(
            const FArguments& InArgs)
        -> void
    {
        FilterMetaData = InArgs._FilterMetaData;
        OnCVarRefChanged = InArgs._OnCVarRefChanged;
        SelectedName = InArgs._DefaultName;

        SAssignNew(ComboButton, SComboButton)
            .OnGetMenuContent(this, &SCVarRef_Widget::GenerateCVarPicker)
            .ContentPadding(FMargin(2.0f, 2.0f))
            .ToolTipText(this, &SCVarRef_Widget::GetSelectedValueAsText)
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text(this, &SCVarRef_Widget::GetSelectedValueAsText)
            ];

        ChildSlot
        [
            ComboButton.ToSharedRef()
        ];
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarRef_Widget::
        OnItemPicked(
            const FString& InName)
        -> void
    {
        const auto PickedName = FName{*InName};

        SelectedName = PickedName;

        // Close the dropdown BEFORE firing the delegate — the delegate triggers
        // TrySetDefaultValue which can invalidate Slate widget state.
        if (ComboButton.IsValid())
        {
            ComboButton->SetIsOpen(false);
        }

        std::ignore = OnCVarRefChanged.ExecuteIfBound(PickedName);
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarRef_Widget::
        GenerateCVarPicker()
        -> TSharedRef<SWidget>
    {
        const auto OnPicked = FOnCVarPicked(
            FOnCVarPicked::CreateRaw(this, &SCVarRef_Widget::OnItemPicked));

        return SNew(SBox)
            .WidthOverride(350)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .MaxHeight(500)
                [
                    SNew(SCVarListWidget)
                    .OnCVarPickedDelegate(OnPicked)
                    .FilterMetaData(FilterMetaData)
                ]
            ];
    }

    // -----------------------------------------------------------------------------------------------------------------

    auto
        SCVarRef_Widget::
        GetSelectedValueAsText() const
        -> FText
    {
        return FText::FromName(SelectedName);
    }
}

// --------------------------------------------------------------------------------------------------------------------

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
