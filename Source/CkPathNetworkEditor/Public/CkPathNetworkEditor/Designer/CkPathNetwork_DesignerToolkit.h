#pragma once

#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerPreset.h"

#include <CoreMinimal.h>
#include <EditorUndoClient.h>
#include <Toolkits/BaseToolkit.h>

class IDetailsView;
class SEditableTextBox;
class UCk_PathNetworkDesigner_EdMode;
class UCk_PathNetworkDesigner_Session_UE;
template<typename OptionType>
class SComboBox;

// --------------------------------------------------------------------------------------------------------------------

class CKPATHNETWORKEDITOR_API FCk_PathNetworkDesigner_Toolkit
    : public FModeToolkit
    , public FEditorUndoClient
{
public:
    ~FCk_PathNetworkDesigner_Toolkit() override;

    auto
    Init(
        const TSharedPtr<IToolkitHost>& InToolkitHost,
        TWeakObjectPtr<UEdMode> InOwningMode) -> void override;

    auto
    GetToolPaletteNames(
        TArray<FName>& OutPaletteNames) const -> void override;

    auto GetToolkitFName() const -> FName override;
    auto GetBaseToolkitName() const -> FText override;
    auto GetInlineContent() const -> TSharedPtr<SWidget> override;

    auto PostUndo(bool InSuccess) -> void override;
    auto PostRedo(bool InSuccess) -> void override;

private:
    auto Get_Mode() const -> UCk_PathNetworkDesigner_EdMode*;
    auto Get_Session() const -> UCk_PathNetworkDesigner_Session_UE*;
    auto Refresh_Details() -> void;
    auto Refresh_RouteWatchItems() -> void;

    auto On_UseCurrentLevel() -> FReply;
    auto On_FitLoadedWorld() -> FReply;
    auto On_FitSelection() -> FReply;
    auto On_LoadSelectedNetwork() -> FReply;
    auto On_CaptureRouteStart() -> FReply;
    auto On_CaptureRouteGoal() -> FReply;
    auto On_PreviewRoute() -> FReply;
    auto On_AddRouteWatch() -> FReply;
    auto On_SaveRouteWatch() -> FReply;
    auto On_RemoveRouteWatch() -> FReply;
    auto On_RefreshAllRouteWatches() -> FReply;
    auto On_Preview() -> FReply;
    auto On_Apply() -> FReply;
    auto On_ClearPreview() -> FReply;
    auto On_ConfigurationChanged(const FPropertyChangedEvent& InEvent) -> void;

    auto On_PresetSelected(
        TSharedPtr<ck::pathnetwork_editor::designer::FPreset> InPreset,
        ESelectInfo::Type InSelectInfo) -> void;

    auto Make_PresetRow(
        TSharedPtr<ck::pathnetwork_editor::designer::FPreset> InPreset) const
        -> TSharedRef<SWidget>;

    auto On_RouteWatchSelected(
        TSharedPtr<FName> InRouteWatch,
        ESelectInfo::Type InSelectInfo) -> void;

    auto Make_RouteWatchRow(
        TSharedPtr<FName> InRouteWatch) const
        -> TSharedRef<SWidget>;

    auto Get_TargetLevelText() const -> FText;
    auto Get_DetectorText() const -> FText;
    auto Get_ConnectivityText() const -> FText;
    auto Get_ConnectivityColor() const -> FSlateColor;
    auto Get_RoutePreviewText() const -> FText;
    auto Get_RoutePreviewColor() const -> FSlateColor;
    auto Get_StatusText() const -> FText;
    auto Get_StatusColor() const -> FSlateColor;
    auto Get_StatusBackgroundColor() const -> FSlateColor;
    auto Get_ActivePresetText() const -> FText;
    auto Get_ActiveRouteWatchText() const -> FText;
    auto Get_RouteWatchSummaryText() const -> FText;
    auto Get_CanPreview() const -> bool;
    auto Get_CanPreviewRoute() const -> bool;
    auto Get_CanAddRouteWatch() const -> bool;
    auto Get_CanSaveRouteWatch() const -> bool;
    auto Get_CanRefreshAllRouteWatches() const -> bool;
    auto Get_CanApply() const -> bool;

private:
    TSharedPtr<SWidget> _InlineContent;
    TWeakObjectPtr<UEdMode> _OwningMode;
    TSharedPtr<IDetailsView> _DetailsView;
    TArray<TSharedPtr<ck::pathnetwork_editor::designer::FPreset>> _PresetItems;
    TSharedPtr<ck::pathnetwork_editor::designer::FPreset> _SelectedPreset;
    TArray<TSharedPtr<FName>> _RouteWatchItems;
    TSharedPtr<FName> _SelectedRouteWatchItem;
    TSharedPtr<SComboBox<TSharedPtr<FName>>> _RouteWatchComboBox;
    TSharedPtr<SEditableTextBox> _RouteWatchNameTextBox;
};

// --------------------------------------------------------------------------------------------------------------------
