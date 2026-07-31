#pragma once

#include "CkPathNetworkEditor/Authoring/CkPathNetwork_AuthoringService.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_DesignerSession.generated.h"

class ACk_PathNetwork_UE;
class ULevel;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer
{
    enum class ERoutePreviewDecision : uint8
    {
        Unavailable,
        NetworkSelected,
        NetworkPreferredByMinimumSavings,
        DirectCostWon,
        NoStartJoin,
        NoGoalJoin,
        NoConnectedNetworkRoute,
        NetworkDiagnosticIncomplete
    };

    struct CKPATHNETWORKEDITOR_API FRoutePreviewSegment
    {
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        int32 _NodeA = INDEX_NONE;
        int32 _NodeB = INDEX_NONE;
        float _Distance = 0.0f;
        ck::pathnetwork::ERouteNodeKind _KindA =
            ck::pathnetwork::ERouteNodeKind::NetNode;
        ck::pathnetwork::ERouteNodeKind _KindB =
            ck::pathnetwork::ERouteNodeKind::NetNode;
    };

    struct CKPATHNETWORKEDITOR_API FEdgeInteriorTransferWitness
    {
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        int32 _EdgeA = INDEX_NONE;
        int32 _EdgeB = INDEX_NONE;
        int32 _ComponentA = INDEX_NONE;
        int32 _ComponentB = INDEX_NONE;
        float _Distance = 0.0f;
        float _NearestEndpointPairDistance = 0.0f;
    };

    struct CKPATHNETWORKEDITOR_API FRoutePreviewResult
    {
        bool _Succeeded = false;
        FString _FailureReason;
        TArray<FVector> _CompiledWaypoints;
        TArray<FVector> _JoinPoints;
        TArray<FRoutePreviewSegment> _LocalNetworkShortcutSegments;
        TArray<FRoutePreviewSegment> _ComponentTransferSegments;
        TArray<FRoutePreviewSegment> _SelectedComponentTransferSegments;
        TArray<FEdgeInteriorTransferWitness> _EdgeInteriorTransferWitnesses;
        float _EstimatedCost = 0.0f;
        bool _UsesNetwork = false;
        int32 _OnNetworkLegCount = 0;
        int32 _OffNetworkLegCount = 0;
        ERoutePreviewDecision _Decision = ERoutePreviewDecision::Unavailable;
        int32 _StartCandidateCount = 0;
        int32 _GoalCandidateCount = 0;
        int32 _NetworkComponentCount = 0;
        int32 _ComponentTransferLinkCount = 0;
        int32 _ComponentTransferCandidateCount = 0;
        int32 _ComponentTransferEdgeInteriorCandidateCount = 0;
        int32 _ComponentTransferRejectedByCellCapCount = 0;
        int32 _ComponentTransferLegCount = 0;
        int32 _EdgeInteriorTransferWitnessCount = 0;
        bool _EdgeInteriorTransferWitnessBudgetExceeded = false;
        int32 _LocalNetworkShortcutLinkCount = 0;
        int32 _LocalNetworkShortcutLegCount = 0;
        int32 _LocalNetworkShortcutCandidateCount = 0;
        int32 _LocalNetworkShortcutCandidateSourceCount = 0;
        bool _LocalNetworkShortcutBudgetExceeded = false;
        float _NearestStartNetworkDistance = -1.0f;
        float _NearestGoalNetworkDistance = -1.0f;
        float _EndpointJoinMaxDistance = 0.0f;
        float _ComponentTransferMaxDistance = 0.0f;
        float _ConfiguredNetworkGapCostMultiplier = 0.0f;
        float _EffectiveNetworkGapCostMultiplier = 0.0f;
        float _LongestComponentTransferDistance = 0.0f;
        float _LocalNetworkShortcutMaxDistance = 0.0f;
        float _LongestLocalNetworkShortcutDistance = 0.0f;
        float _DirectEstimatedCost = 0.0f;
        float _BestNetworkEstimatedCost = 0.0f;
        float _DirectRouteMinimumSavingsFraction = 0.0f;
        float _DirectRouteSavingsFraction = 0.0f;
        bool _DirectTripGraceApplied = false;
        bool _HasNetworkAlternative = false;
        FString _NetworkDiagnosticOutcome = TEXT("NotRun");
    };

    struct CKPATHNETWORKEDITOR_API FRouteWatchPreview
    {
        int32 _RouteIndex = INDEX_NONE;
        FName _Name;
        FVector _Start = FVector::ZeroVector;
        FVector _Goal = FVector::ZeroVector;
        FRoutePreviewResult _Preview;
    };
}

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_PathNetworkDesigner_Status : uint8
{
    Ready,
    PreviewReady,
    Applied,
    Error
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Transient)
class CKPATHNETWORKEDITOR_API UCk_PathNetworkDesigner_Session_UE : public UObject
{
    GENERATED_BODY()

public:
    auto
    Initialize(
        UWorld* InWorld) -> void;

    auto
    GetWorld() const -> UWorld* override;

    auto
    Use_CurrentLevel() -> bool;

    auto
    Fit_BoundsToLoadedWorld() -> bool;

    auto
    Fit_BoundsToSelection() -> bool;

    auto
    Load_SelectedPathNetwork() -> bool;

    auto
    Apply_Preset(
        FName InOwner,
        FName InId) -> bool;

    auto
    Run_Preview() -> bool;

    auto
    Capture_RouteStartFromSelection() -> bool;

    auto
    Capture_RouteGoalFromSelection() -> bool;

    auto
    Set_RoutePreviewEndpoints(
        const FVector& InStart,
        const FVector& InGoal) -> void;

    auto
    Run_RoutePreview() -> bool;

    auto
    Select_RouteWatch(
        int32 InRouteIndex) -> bool;

    auto
    Add_RouteWatch(
        FName InName) -> bool;

    auto
    Save_ActiveRouteWatch(
        FName InName) -> bool;

    auto
    Remove_ActiveRouteWatch() -> bool;

    auto
    Refresh_AllRouteWatches() -> bool;

    auto
    Synchronize_RouteWatchesFromActor() -> void;

    auto
    Apply_ToLevel() -> bool;

    auto
    Clear_Preview() -> void;

    auto
    Clear_RoutePreview() -> void;

    auto
    Notify_ConfigurationEdited() -> void;

    auto Get_TargetLevel() const -> ULevel* { return _TargetLevel.Get(); }
    auto Get_DetectorTemplate() const -> UCk_PathNetwork_Detector_UE* { return _DetectorTemplate; }
    auto Get_DetectionBounds() const -> FBox;
    auto Get_Preview() const -> const ck::pathnetwork_editor::authoring::FPreviewResult& { return _Preview; }
    auto Get_RoutePreview() const -> const ck::pathnetwork_editor::designer::FRoutePreviewResult& { return _RoutePreview; }
    auto Get_HasTopologyAnalysis() const -> bool { return _HasTopologyAnalysis; }
    auto Get_TopologyAnalysis() const -> const ck::pathnetwork::FNetworkTopologyAnalysis& { return _TopologyAnalysis; }
    auto Get_MaskDrawPoints() const -> const TArray<FVector>& { return _MaskDrawPoints; }
    auto Get_VisualizedActor() const -> ACk_PathNetwork_UE* { return _VisualizedActor.Get(); }
    auto Get_Status() const -> ECk_PathNetworkDesigner_Status { return _Status; }
    auto Get_StatusMessage() const -> const FString& { return _StatusMessage; }
    auto Get_ActivePresetOwner() const -> FName { return _ActivePresetOwner; }
    auto Get_ActivePresetId() const -> FName { return _ActivePresetId; }
    auto Get_UseRecommendedFollowerTuning() const -> ECk_EnableDisable { return _UseRecommendedFollowerTuning; }
    auto Get_RecommendedFollowerTuning() const -> const FCk_PathNetworkFollower_Tuning& { return _RecommendedFollowerTuning; }
    auto Get_NodeSnapRadius() const -> float { return _BuildParams.Get_NodeSnapRadius(); }
    auto Get_ComponentTransferMaxDistance() const -> float { return _RecommendedFollowerTuning.Get_ComponentTransferMaxDistance(); }
    auto Get_LocalNetworkShortcutMaxDistance() const -> float { return _RecommendedFollowerTuning.Get_LocalNetworkShortcutMaxDistance(); }
    // Zero inherits Far/Direct Cost Multiplier. At runtime it prices only navmesh-validated network gaps.
    auto Get_NetworkGapCostMultiplier() const -> float { return _RecommendedFollowerTuning.Get_NetworkGapCostMultiplier(); }
    auto Get_RoutePreviewStart() const -> const FVector& { return _RoutePreviewStart; }
    auto Get_RoutePreviewGoal() const -> const FVector& { return _RoutePreviewGoal; }
    auto Get_RouteWatchNames() const -> TArray<FName>;
    auto Get_RouteWatchCount() const -> int32;
    auto Get_ActiveRouteWatchIndex() const -> int32 { return _ActiveRouteWatchIndex; }
    auto Get_CanPersistRouteWatches() const -> bool;
    auto Get_RouteWatchPreviews() const
        -> const TArray<ck::pathnetwork_editor::designer::FRouteWatchPreview>&
    { return _RouteWatchPreviews; }

    auto Get_DrawBounds() const -> bool { return _DrawBounds; }
    auto Get_DrawMask() const -> bool { return _DrawMask; }
    auto Get_DrawPreviewRibbons() const -> bool { return _DrawPreviewRibbons; }
    auto Get_DrawAuthoredRibbons() const -> bool { return _DrawAuthoredRibbons; }
    auto Get_DrawGeneratedRibbons() const -> bool { return _DrawGeneratedRibbons; }
    auto Get_DrawComponentTransfers() const -> bool { return _DrawComponentTransfers; }
    auto Get_DrawRoutePreview() const -> bool { return _DrawRoutePreview; }
    auto Get_MaxMaskCellsToDraw() const -> int32 { return _MaxMaskCellsToDraw; }

#if WITH_EDITOR
    auto
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

private:
    auto
    Set_DetectorClass(
        UClass* InDetectorClass) -> bool;

    auto
    Set_Status(
        ECk_PathNetworkDesigner_Status InStatus,
        FString InMessage) -> void;

    auto
    Fit_Bounds(
        const FBox& InBounds,
        const FString& InSourceLabel) -> bool;

    auto
    Rebuild_MaskDrawPoints() -> void;

    auto
    Get_ProspectivePreviewRibbons() const -> TArray<FCk_PathNetwork_Ribbon>;

    auto
    Capture_RouteEndpointFromSelection(
        bool InCaptureStart) -> bool;

    auto
    Resolve_RouteWatchActor() const -> ACk_PathNetwork_UE*;

    auto
    Persist_RouteWatches(
        const TArray<FCk_PathNetwork_RepresentativeRoute>& InWorldRoutes,
        const FText& InTransactionText) -> bool;

    auto
    Load_RouteWatch(
        int32 InRouteIndex,
        bool InSetStatus) -> bool;

    auto
    Invalidate_RouteWatchPreviews() -> void;

private:
    UPROPERTY(EditAnywhere,
        Category = "1. Detector",
        meta = (AllowAbstract = false))
    TSubclassOf<UCk_PathNetwork_Detector_UE> _DetectorClass;

    UPROPERTY(EditAnywhere, Instanced,
        Category = "1. Detector",
        meta = (ShowOnlyInnerProperties))
    TObjectPtr<UCk_PathNetwork_Detector_UE> _DetectorTemplate;

    UPROPERTY(EditAnywhere,
        Category = "2. Detection Area")
    FVector _DetectionCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere,
        Category = "2. Detection Area",
        meta = (ClampMin = "1.0"))
    FVector _DetectionExtents = FVector{50000.0, 50000.0, 10000.0};

    UPROPERTY(EditAnywhere,
        Category = "3. Network Generation")
    FCk_PathNetwork_VectorizeParams _VectorizeParams;

    UPROPERTY(EditAnywhere,
        Category = "3. Network Generation")
    FCk_PathNetwork_BuildParams _BuildParams;

    UPROPERTY(EditAnywhere,
        Category = "3. Network Generation")
    ECk_EnableDisable _AutoDetectOnBeginPlay = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere,
        Category = "4. Route Preferences",
        meta = (
            DisplayName = "Use Map Route Preferences",
            ToolTip = "Save this profile on the path-network actor so game followers can use map-specific routing defaults."))
    ECk_EnableDisable _UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere,
        Category = "4. Route Preferences",
        meta = (
            ShowOnlyInnerProperties,
            DisplayName = "Recommended Follower Tuning"))
    FCk_PathNetworkFollower_Tuning _RecommendedFollowerTuning;

    UPROPERTY(EditAnywhere,
        Category = "5. Representative Route",
        meta = (
            DisplayName = "Route Start",
            ToolTip = "Representative trip start used only by the transient editor preview."))
    FVector _RoutePreviewStart = FVector::ZeroVector;

    UPROPERTY(EditAnywhere,
        Category = "5. Representative Route",
        meta = (
            DisplayName = "Route Goal",
            ToolTip = "Representative trip goal used only by the transient editor preview."))
    FVector _RoutePreviewGoal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawBounds = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawMask = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawPreviewRibbons = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawAuthoredRibbons = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawGeneratedRibbons = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay",
        meta = (DisplayName = "Draw Available Component Transfers",
            ToolTip = "Draws all admitted cross-island transfer links in thin amber. Selected route transfers draw thicker and dashed."))
    bool _DrawComponentTransfers = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay")
    bool _DrawRoutePreview = true;

    UPROPERTY(EditAnywhere,
        Category = "6. Viewport Overlay",
        meta = (ClampMin = "100", ClampMax = "100000"))
    int32 _MaxMaskCellsToDraw = 12000;

private:
    TWeakObjectPtr<UWorld> _World;
    TWeakObjectPtr<ULevel> _TargetLevel;
    TWeakObjectPtr<ACk_PathNetwork_UE> _ExplicitTargetActor;
    TWeakObjectPtr<ACk_PathNetwork_UE> _VisualizedActor;

    ck::pathnetwork_editor::authoring::FPreviewResult _Preview;
    ck::pathnetwork_editor::designer::FRoutePreviewResult _RoutePreview;
    TArray<ck::pathnetwork_editor::designer::FRouteWatchPreview>
        _RouteWatchPreviews;
    int32 _ActiveRouteWatchIndex = INDEX_NONE;
    ck::pathnetwork::FNetworkTopologyAnalysis _TopologyAnalysis;
    bool _HasTopologyAnalysis = false;
    TArray<FVector> _MaskDrawPoints;
    ECk_PathNetworkDesigner_Status _Status = ECk_PathNetworkDesigner_Status::Ready;
    FString _StatusMessage = TEXT("Choose a detector and preview the loaded editor world.");
    FName _ActivePresetOwner;
    FName _ActivePresetId;
};

// --------------------------------------------------------------------------------------------------------------------
