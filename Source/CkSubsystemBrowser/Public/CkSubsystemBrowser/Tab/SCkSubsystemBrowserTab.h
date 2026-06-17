#pragma once

#include "CkSubsystemBrowser/Model/CkSubsystemBrowser_Collector.h"

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/STreeView.h>

// --------------------------------------------------------------------------------------------------------------------

class IDetailsView;
class SSearchBox;

// --------------------------------------------------------------------------------------------------------------------

// View-model node for the browser tree. A node is either a category header
// (IsCategoryNode == true, with Children) or a subsystem leaf (Subsystem set).
struct FCk_SubsystemBrowser_Node
{
    FString DisplayName;
    ECk_SubsystemCategory Category = ECk_SubsystemCategory::Engine;
    bool IsCategoryNode = false;
    TWeakObjectPtr<UObject> Subsystem;
    TArray<TSharedPtr<FCk_SubsystemBrowser_Node>> Children;
};

// --------------------------------------------------------------------------------------------------------------------

class CKSUBSYSTEMBROWSER_API SCkSubsystemBrowserTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkSubsystemBrowserTab) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
    virtual ~SCkSubsystemBrowserTab() override;

private:
    using NodeType = TSharedPtr<FCk_SubsystemBrowser_Node>;

    auto DoRebuildTree() -> void;

    auto DoGenerateRow(NodeType InNode, const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>;
    auto DoGetChildren(NodeType InNode, TArray<NodeType>& OutChildren) -> void;
    auto DoOnSelectionChanged(NodeType InNode, ESelectInfo::Type InSelectInfo) -> void;

    auto DoOnSearchTextChanged(const FText& InText) -> void;
    auto DoOnRefreshClicked() -> FReply;
    auto DoHandlePieEvent(const bool bInIsSimulating) -> void;

    auto DoPassesFilter(const FString& InDisplayName) const -> bool;

private:
    TSharedPtr<STreeView<NodeType>> _TreeView;
    TSharedPtr<IDetailsView> _DetailsView;
    TSharedPtr<SSearchBox> _SearchBox;

    TArray<NodeType> _RootNodes;
    FString _FilterText;

    FDelegateHandle _PostPieStartedHandle;
    FDelegateHandle _EndPieHandle;
};

// --------------------------------------------------------------------------------------------------------------------
