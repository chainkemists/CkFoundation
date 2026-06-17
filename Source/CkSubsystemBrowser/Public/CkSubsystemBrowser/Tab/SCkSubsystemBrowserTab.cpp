#include "CkSubsystemBrowser/Tab/SCkSubsystemBrowserTab.h"

#include "Editor.h"

#include <IDetailsView.h>
#include <Modules/ModuleManager.h>
#include <PropertyEditorModule.h>
#include <Styling/CoreStyle.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Layout/SSplitter.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Views/STableRow.h>

#define LOCTEXT_NAMESPACE "SCkSubsystemBrowserTab"

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkSubsystemBrowserTab::
    Construct(
        const FArguments& InArgs)
    -> void
{
    auto& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    auto DetailsViewArgs = FDetailsViewArgs{};
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.bShowOptions = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    _DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    ChildSlot
    [
        SNew(SSplitter)
        + SSplitter::Slot()
        .Value(0.4f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(4.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(_SearchBox, SSearchBox)
                    .HintText(LOCTEXT("SearchHint", "Filter subsystems by class name"))
                    .OnTextChanged(this, &SCkSubsystemBrowserTab::DoOnSearchTextChanged)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("Refresh", "Refresh"))
                    .ToolTipText(LOCTEXT("RefreshTooltip", "Re-enumerate live subsystems"))
                    .OnClicked(this, &SCkSubsystemBrowserTab::DoOnRefreshClicked)
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(4.0f, 0.0f, 4.0f, 4.0f)
            [
                SAssignNew(_TreeView, STreeView<NodeType>)
                .TreeItemsSource(&_RootNodes)
                .SelectionMode(ESelectionMode::Single)
                .OnGenerateRow(this, &SCkSubsystemBrowserTab::DoGenerateRow)
                .OnGetChildren(this, &SCkSubsystemBrowserTab::DoGetChildren)
                .OnSelectionChanged(this, &SCkSubsystemBrowserTab::DoOnSelectionChanged)
            ]
        ]
        + SSplitter::Slot()
        .Value(0.6f)
        [
            _DetailsView.ToSharedRef()
        ]
    ];

    DoRebuildTree();

    _PostPieStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &SCkSubsystemBrowserTab::DoHandlePieEvent);
    _EndPieHandle         = FEditorDelegates::EndPIE.AddRaw(this, &SCkSubsystemBrowserTab::DoHandlePieEvent);
}

SCkSubsystemBrowserTab::
    ~SCkSubsystemBrowserTab()
{
    FEditorDelegates::PostPIEStarted.Remove(_PostPieStartedHandle);
    FEditorDelegates::EndPIE.Remove(_EndPieHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkSubsystemBrowserTab::
    DoRebuildTree()
    -> void
{
    _RootNodes.Reset();

    for (const auto Category : ck::subsystem_browser::Get_AllCategories())
    {
        auto CategoryNode = MakeShared<FCk_SubsystemBrowser_Node>();
        CategoryNode->IsCategoryNode = true;
        CategoryNode->Category = Category;

        for (const auto& WeakSubsystem : ck::subsystem_browser::Collect_Subsystems(Category))
        {
            if (WeakSubsystem.IsValid() == false)
            { continue; }

            const auto DisplayName = WeakSubsystem->GetClass()->GetName();

            if (DoPassesFilter(DisplayName) == false)
            { continue; }

            auto LeafNode = MakeShared<FCk_SubsystemBrowser_Node>();
            LeafNode->DisplayName = DisplayName;
            LeafNode->Category = Category;
            LeafNode->Subsystem = WeakSubsystem;
            CategoryNode->Children.Add(LeafNode);
        }

        // While filtering, hide categories that matched nothing. With no filter,
        // always show the (possibly empty) category so its absence is meaningful.
        if (_FilterText.IsEmpty() == false && CategoryNode->Children.Num() == 0)
        { continue; }

        CategoryNode->DisplayName = FString::Printf(TEXT("%s (%d)"),
            *ck::subsystem_browser::Get_CategoryDisplayName(Category),
            CategoryNode->Children.Num());

        _RootNodes.Add(CategoryNode);
    }

    if (_TreeView.IsValid())
    {
        _TreeView->RequestTreeRefresh();

        for (const auto& RootNode : _RootNodes)
        { _TreeView->SetItemExpansion(RootNode, true); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkSubsystemBrowserTab::
    DoGenerateRow(
        NodeType InNode,
        const TSharedRef<STableViewBase>& InOwnerTable)
    -> TSharedRef<ITableRow>
{
    const auto Font = InNode->IsCategoryNode
        ? FCoreStyle::GetDefaultFontStyle("Bold", 10)
        : FCoreStyle::GetDefaultFontStyle("Regular", 9);

    return SNew(STableRow<NodeType>, InOwnerTable)
    [
        SNew(STextBlock)
        .Text(FText::FromString(InNode->DisplayName))
        .Font(Font)
    ];
}

auto
    SCkSubsystemBrowserTab::
    DoGetChildren(
        NodeType InNode,
        TArray<NodeType>& OutChildren)
    -> void
{
    if (InNode.IsValid())
    { OutChildren.Append(InNode->Children); }
}

auto
    SCkSubsystemBrowserTab::
    DoOnSelectionChanged(
        NodeType InNode,
        ESelectInfo::Type InSelectInfo)
    -> void
{
    if (_DetailsView.IsValid() == false)
    { return; }

    if (InNode.IsValid() && InNode->IsCategoryNode == false && InNode->Subsystem.IsValid())
    { _DetailsView->SetObject(InNode->Subsystem.Get()); }
    else
    { _DetailsView->SetObject(nullptr); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkSubsystemBrowserTab::
    DoOnSearchTextChanged(
        const FText& InText)
    -> void
{
    _FilterText = InText.ToString();
    DoRebuildTree();
}

auto
    SCkSubsystemBrowserTab::
    DoOnRefreshClicked()
    -> FReply
{
    DoRebuildTree();
    return FReply::Handled();
}

auto
    SCkSubsystemBrowserTab::
    DoHandlePieEvent(
        const bool bInIsSimulating)
    -> void
{
    DoRebuildTree();
}

auto
    SCkSubsystemBrowserTab::
    DoPassesFilter(
        const FString& InDisplayName) const
    -> bool
{
    if (_FilterText.IsEmpty())
    { return true; }

    return InDisplayName.Contains(_FilterText);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
