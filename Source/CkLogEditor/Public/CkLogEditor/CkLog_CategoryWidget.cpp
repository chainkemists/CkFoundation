#include "CkLog_CategoryWidget.h"

#include "CkLog/CkLog_Utils.h"

#include "Misc/TextFilter.h"
#include "SlateOptMacros.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Input/SComboButton.h"

#define LOCTEXT_NAMESPACE "SLogCategoryWidget"

//--------------------------------------------------------------------------------------------------------------------------
DECLARE_DELEGATE_OneParam(FOnLogCategoryPicked, FName);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

//--------------------------------------------------------------------------------------------------------------------------

struct FLogCategoryViewerNode
{
public:
    explicit FLogCategoryViewerNode(FName InName)
        : _Name(InName)
    {}

public:
    auto Get_Name() const -> FName { return _Name; }

private:
    FName _Name;
};

//--------------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class SLogCategoryItem : public SComboRow<TSharedPtr<FLogCategoryViewerNode>>
    {
    public:

        SLATE_BEGIN_ARGS(SLogCategoryItem)
            : _HighlightText()
            , _TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        {}

        SLATE_ARGUMENT(FText, HighlightText)
        SLATE_ARGUMENT(FSlateColor, TextColor)
        SLATE_ARGUMENT(TSharedPtr<FLogCategoryViewerNode>, AssociatedNode)

        SLATE_END_ARGS()

    public:
        auto Construct(
            const FArguments& InArgs,
            const TSharedRef<STableViewBase>& InOwnerTableView)
            -> void
        {
            AssociatedNode = InArgs._AssociatedNode;

            this->ChildSlot
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(0.0f, 3.0f, 6.0f, 3.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromName(AssociatedNode->Get_Name()))
                            .HighlightText(InArgs._HighlightText)
                            .ColorAndOpacity(this, &SLogCategoryItem::GetTextColor)
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
            const TSharedPtr<ITypedTableView<TSharedPtr<FLogCategoryViewerNode>>> OwnerWidget = OwnerTablePtr.Pin();
            const TSharedPtr<FLogCategoryViewerNode>* MyItem = OwnerWidget->Private_ItemFromWidget(this);

            if (OwnerWidget->Private_IsItemSelected(*MyItem))
            { return FSlateColor::UseForeground(); }

            return TextColor;
        }

    private:
        FSlateColor TextColor;
        TSharedPtr<FLogCategoryViewerNode> AssociatedNode;
    };

    //--------------------------------------------------------------------------------------------------------------------------

    class SLogCategoryListWidget : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SLogCategoryListWidget)
        {
        }

        SLATE_ARGUMENT(FString, FilterMetaData)
        SLATE_ARGUMENT(FOnLogCategoryPicked, OnLogCategoryPickedDelegate)

        SLATE_END_ARGS()

    public:
        virtual ~SLogCategoryListWidget();

    public:
        auto Construct(
            const FArguments& InArgs) -> void;

    private:
        typedef TTextFilter<const FName&> FLogCategoryTextFilter;

        auto OnFilterTextChanged(
            const FText& InFilterText) -> void;

        auto OnGenerateRowForLogCategoryViewer(
            TSharedPtr<FLogCategoryViewerNode> Item,
            const TSharedRef<STableViewBase>&  InOwnerTable) const -> TSharedRef<ITableRow>;

        auto OnLogCategorySelectionChanged(
            TSharedPtr<FLogCategoryViewerNode> Item,
            ESelectInfo::Type SelectInfo) const -> void;

        auto UpdatePropertyOptions() -> TSharedPtr<FLogCategoryViewerNode>;

    private:
        FOnLogCategoryPicked _OnLogCategoryPicked;
        TSharedPtr<SSearchBox> _SearchBox;
        TSharedPtr<SListView<TSharedPtr<FLogCategoryViewerNode>>> _LogCategoryList;
        TArray<TSharedPtr<FLogCategoryViewerNode>> _PropertyOptions;
        TSharedPtr<FLogCategoryTextFilter> _LogCategoryTextFilter;
        FString _FilterMetaData;
    };

    //--------------------------------------------------------------------------------------------------------------------------

    SLogCategoryListWidget::
        ~SLogCategoryListWidget()
    {
        if (_OnLogCategoryPicked.IsBound())
        {
            _OnLogCategoryPicked.Unbind();
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategoryListWidget::
        Construct(
            const FArguments& InArgs)
        -> void
    {
        struct Local
        {
            static void LogCategoryToStringArray(const FName& InName, OUT TArray<FString>& OutStringArray)
            {
                OutStringArray.Add(InName.ToString());
            }
        };

        _FilterMetaData = InArgs._FilterMetaData;
        _OnLogCategoryPicked = InArgs._OnLogCategoryPickedDelegate;

        // Setup text filtering
        _LogCategoryTextFilter = MakeShareable(new FLogCategoryTextFilter(FLogCategoryTextFilter::FItemToStringArray::CreateStatic(&Local::LogCategoryToStringArray)));

        UpdatePropertyOptions();

        TSharedPtr< SWidget > ClassViewerContent;

        SAssignNew(ClassViewerContent, SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew(_SearchBox, SSearchBox)
                .HintText(NSLOCTEXT("Log", "SearchBoxHint", "Search Log Categories"))
                .OnTextChanged(this, &SLogCategoryListWidget::OnFilterTextChanged)
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
                SAssignNew(_LogCategoryList, SListView<TSharedPtr<FLogCategoryViewerNode>>)
                .Visibility(EVisibility::Visible)
                .SelectionMode(ESelectionMode::Single)
                .ListItemsSource(&_PropertyOptions)

                // Generates the actual widget for a tree item
                .OnGenerateRow(this, &SLogCategoryListWidget::OnGenerateRowForLogCategoryViewer)

                // Find out when the user selects something in the tree
                .OnSelectionChanged(this, &SLogCategoryListWidget::OnLogCategorySelectionChanged)
            ];


        ChildSlot
            [
                ClassViewerContent.ToSharedRef()
            ];
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategoryListWidget::
        OnGenerateRowForLogCategoryViewer(
            TSharedPtr<FLogCategoryViewerNode> Item,
            const TSharedRef<STableViewBase>&  InOwnerTable) const
        -> TSharedRef<ITableRow>
    {
        TSharedRef< SLogCategoryItem > ReturnRow = SNew(SLogCategoryItem, InOwnerTable)
            .HighlightText(_SearchBox->GetText())
            .TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.f))
            .AssociatedNode(Item);

        return ReturnRow;
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategoryListWidget::
        UpdatePropertyOptions()
        -> TSharedPtr<FLogCategoryViewerNode>
    {
        _PropertyOptions.Empty();

        auto SortedLogCategories = log::Get_AllRegisteredCategories();
        algo::Sort(SortedLogCategories, [](FName InA, FName InB) -> bool
        {
            return InA.LexicalLess(InB);
        });

        const auto FilteredLogCategories = algo::Filter(SortedLogCategories, [&](FName InLogCategoryName)
        {
            if (NOT _LogCategoryTextFilter.IsValid())
            { return true; }

            return _LogCategoryTextFilter->PassesFilter(InLogCategoryName);
        });

        const TSharedPtr<FLogCategoryViewerNode> InitiallySelected = MakeShareable(new FLogCategoryViewerNode(FName()));
        _PropertyOptions.Add(InitiallySelected);

        _PropertyOptions.Append(algo::Transform<TArray<TSharedPtr<FLogCategoryViewerNode>>>(FilteredLogCategories, [](FName InLogCategoryName)
        {
            return MakeShareable(new FLogCategoryViewerNode{InLogCategoryName});
        }));

        return InitiallySelected;
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategoryListWidget::
        OnFilterTextChanged(
            const FText& InFilterText)
        -> void
    {
        _LogCategoryTextFilter->SetRawFilterText(InFilterText);
        _SearchBox->SetError(_LogCategoryTextFilter->GetFilterErrorText());

        UpdatePropertyOptions();
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategoryListWidget::
        OnLogCategorySelectionChanged(
            TSharedPtr<FLogCategoryViewerNode> Item,
            ESelectInfo::Type SelectInfo) const
        -> void
    {
        std::ignore = _OnLogCategoryPicked.ExecuteIfBound(Item->Get_Name());
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategory_Widget::
        Construct(
            const FArguments& InArgs)
        -> void
    {
        FilterMetaData = InArgs._FilterMetaData;
        OnLogCategoryChanged = InArgs._OnLogCategoryChanged;
        SelectedName = InArgs._DefaultName;

        SAssignNew(ComboButton, SComboButton)
            .OnGetMenuContent(this, &SLogCategory_Widget::GenerateLogCategoryPicker)
            .ContentPadding(FMargin(2.0f, 2.0f))
            .ToolTipText(this, &SLogCategory_Widget::GetSelectedValueAsText)
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text(this, &SLogCategory_Widget::GetSelectedValueAsText)
            ];

        ChildSlot
        [
            ComboButton.ToSharedRef()
        ];
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategory_Widget::
        OnItemPicked(
            FName Name)
        -> void
    {
        std::ignore = OnLogCategoryChanged.ExecuteIfBound(Name);

        SelectedName = Name;
        ComboButton->SetIsOpen(false);
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategory_Widget::
        GenerateLogCategoryPicker()
        -> TSharedRef<SWidget>
    {
        const FOnLogCategoryPicked OnPicked(FOnLogCategoryPicked::CreateRaw(this, &SLogCategory_Widget::OnItemPicked));

        return SNew(SBox)
            .WidthOverride(280)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .MaxHeight(500)
                [
                    SNew(SLogCategoryListWidget)
                    .OnLogCategoryPickedDelegate(OnPicked)
                    .FilterMetaData(FilterMetaData)
                ]
            ];
    }

    //--------------------------------------------------------------------------------------------------------------------------

    auto
        SLogCategory_Widget::
        GetSelectedValueAsText() const
        -> FText
    {
        return FText::FromName(SelectedName);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
