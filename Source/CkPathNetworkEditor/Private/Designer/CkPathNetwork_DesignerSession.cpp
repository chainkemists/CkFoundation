#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerSession.h"

#include "CkPathNetworkEditor/CkPathNetworkEditor_Log.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerPreset.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"
#include "CkPathNetwork/Network/CkPathNetwork_Build.h"
#include "CkPathNetwork/Network/CkPathNetwork_CorridorCompile.h"
#include "CkPathNetwork/Network/CkPathNetwork_RoutePlan.h"
#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Editor.h>
#include <Engine/Level.h>
#include <Engine/Selection.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <GameFramework/Actor.h>
#include <ScopedTransaction.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_designer_session
{
    constexpr auto RouteWaypointMergeDistance = 1.0f;

    auto
    Append_RouteWaypoint(
        TArray<FVector>& InOutWaypoints,
        const FVector& InPoint) -> void
    {
        if (NOT InOutWaypoints.IsEmpty()
            && FVector::Dist(InOutWaypoints.Last(), InPoint)
                < RouteWaypointMergeDistance)
        { return; }

        InOutWaypoints.Add(InPoint);
    }

    auto
    Append_JoinPoint(
        TArray<FVector>& InOutJoinPoints,
        const FVector& InPoint) -> void
    {
        if (InOutJoinPoints.ContainsByPredicate(
                [&](const FVector& InExisting)
                {
                    return FVector::Dist(InExisting, InPoint)
                        < RouteWaypointMergeDistance;
                }))
        { return; }

        InOutJoinPoints.Add(InPoint);
    }

    auto
    Get_NearestNetworkDistance(
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const FVector& InLocation) -> float
    {
        auto NearestDistance = TNumericLimits<float>::Max();
        for (auto EdgeId = 0; EdgeId < InNetwork._Edges.Num(); ++EdgeId)
        {
            NearestDistance = FMath::Min(
                NearestDistance,
                InNetwork.Project_OntoEdge(EdgeId, InLocation)._Distance);
        }

        return FMath::IsFinite(NearestDistance)
            && NearestDistance < TNumericLimits<float>::Max()
            ? NearestDistance
            : -1.0f;
    }

    auto
    Get_ComponentTransferLinkCount(
        const ck::pathnetwork::FRouteGraphSharedData& InShared) -> int32
    {
        auto DirectedLinkCount = 0;
        for (const auto& Pair :
             InShared._ComponentTransfersByRouteNode)
        { DirectedLinkCount += Pair.Value.Num(); }
        return DirectedLinkCount / 2;
    }

    auto
    Get_RouteNodeLocation(
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FRouteGraphSharedData& InShared,
        const ck::pathnetwork::FRouteNodeId& InNodeId) -> FVector
    {
        if (InNodeId._Kind
                == ck::pathnetwork::ERouteNodeKind::NetNode
            && InNetwork._Nodes.IsValidIndex(InNodeId._Index))
        {
            return InNetwork._Nodes[InNodeId._Index]._Location;
        }
        if (InNodeId._Kind
                == ck::pathnetwork::ERouteNodeKind::OverlayPoint
            && InShared._OverlayPoints.IsValidIndex(InNodeId._Index))
        {
            return InShared._OverlayPoints[InNodeId._Index]._Location;
        }
        return FVector::ZeroVector;
    }

    auto
    Make_TransferSegment(
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FRouteGraphSharedData& InShared,
        const ck::pathnetwork::FRouteNodeId& InNodeA,
        const ck::pathnetwork::FRouteNodeId& InNodeB)
        -> ck::pathnetwork_editor::designer::FRoutePreviewSegment
    {
        const auto LocationA =
            Get_RouteNodeLocation(InNetwork, InShared, InNodeA);
        const auto LocationB =
            Get_RouteNodeLocation(InNetwork, InShared, InNodeB);
        return ck::pathnetwork_editor::designer::FRoutePreviewSegment{
            LocationA,
            LocationB,
            InNodeA._Index,
            InNodeB._Index,
            static_cast<float>(FVector::Dist(
                LocationA,
                LocationB)),
            InNodeA._Kind,
            InNodeB._Kind};
    }

    auto
    Populate_AdmittedComponentTransfers(
        ck::pathnetwork_editor::designer::FRoutePreviewResult& InOutPreview,
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FRouteGraphSharedData& InShared) -> void
    {
        InOutPreview._ComponentTransferSegments.Reset();
        InOutPreview._ComponentTransferCandidateCount =
            InShared._ComponentTransferCandidateCount;
        InOutPreview._ComponentTransferEdgeInteriorCandidateCount =
            InShared._ComponentTransferEdgeInteriorCandidateCount;
        InOutPreview._ComponentTransferRejectedByCellCapCount =
            InShared._ComponentTransferRejectedByCellCapCount;
        InOutPreview._ComponentTransferLinkCount =
            Get_ComponentTransferLinkCount(InShared);

        for (const auto& TransferPair :
             InShared._ComponentTransfersByRouteNode)
        {
            const auto NodeA = TransferPair.Key;

            for (const auto NodeB : TransferPair.Value)
            {
                const bool IsCanonicalDirection =
                    NodeA._Kind < NodeB._Kind
                    || (NodeA._Kind == NodeB._Kind
                        && NodeA._Index < NodeB._Index);
                if (NOT IsCanonicalDirection)
                { continue; }

                InOutPreview._ComponentTransferSegments.Add(
                    Make_TransferSegment(
                        InNetwork,
                        InShared,
                        NodeA,
                        NodeB));
            }
        }

        InOutPreview._ComponentTransferSegments.Sort(
            [](const ck::pathnetwork_editor::designer::FRoutePreviewSegment& InA,
               const ck::pathnetwork_editor::designer::FRoutePreviewSegment& InB)
            {
                if (InA._KindA != InB._KindA)
                { return InA._KindA < InB._KindA; }
                if (InA._NodeA != InB._NodeA)
                { return InA._NodeA < InB._NodeA; }
                if (InA._KindB != InB._KindB)
                { return InA._KindB < InB._KindB; }
                return InA._NodeB < InB._NodeB;
            });
    }

    auto
    Get_EdgeComponent(
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FNetworkTopologyAnalysis& InTopology,
        const int32 InEdgeId) -> int32
    {
        if (NOT InNetwork._Edges.IsValidIndex(InEdgeId))
        { return INDEX_NONE; }

        const auto& Edge = InNetwork._Edges[InEdgeId];
        if (NOT InNetwork._Nodes.IsValidIndex(Edge._NodeA)
            || NOT InNetwork._Nodes.IsValidIndex(Edge._NodeB)
            || NOT InTopology._ComponentByNode.IsValidIndex(Edge._NodeA)
            || NOT InTopology._ComponentByNode.IsValidIndex(Edge._NodeB))
        { return INDEX_NONE; }

        const auto ComponentA =
            InTopology._ComponentByNode[Edge._NodeA];
        const auto ComponentB =
            InTopology._ComponentByNode[Edge._NodeB];
        return ComponentA >= 0 && ComponentA == ComponentB
            ? ComponentA
            : INDEX_NONE;
    }

    auto
    Get_RouteNodeComponent(
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FNetworkTopologyAnalysis& InTopology,
        const ck::pathnetwork::FRouteGraphSharedData& InShared,
        const ck::pathnetwork::FRouteNodeId& InNodeId) -> int32
    {
        if (InNodeId._Kind
            == ck::pathnetwork::ERouteNodeKind::NetNode)
        {
            return InTopology._ComponentByNode.IsValidIndex(
                    InNodeId._Index)
                ? InTopology._ComponentByNode[InNodeId._Index]
                : INDEX_NONE;
        }
        if (InNodeId._Kind
                == ck::pathnetwork::ERouteNodeKind::OverlayPoint
            && InShared._OverlayPoints.IsValidIndex(InNodeId._Index))
        {
            return Get_EdgeComponent(
                InNetwork,
                InTopology,
                InShared._OverlayPoints[InNodeId._Index]._EdgeId);
        }
        return INDEX_NONE;
    }

    auto
    Populate_EdgeInteriorTransferWitnesses(
        ck::pathnetwork_editor::designer::FRoutePreviewResult& InOutPreview,
        const ck::pathnetwork::FBuiltNetwork& InNetwork,
        const ck::pathnetwork::FNetworkTopologyAnalysis& InTopology,
        const FVector& InStart,
        const FVector& InGoal,
        const float InMaxDistance) -> void
    {
        constexpr auto MaxStoredWitnesses = 16;
        constexpr auto MaxSegmentPairChecks = 2000000;

        InOutPreview._EdgeInteriorTransferWitnesses.Reset();
        InOutPreview._EdgeInteriorTransferWitnessCount = 0;
        InOutPreview._EdgeInteriorTransferWitnessBudgetExceeded = false;

        const auto DirectDelta = InGoal - InStart;
        const auto DirectDistance = DirectDelta.Size();
        if (InMaxDistance <= 0.0f || DirectDistance <= 1.0)
        { return; }

        const auto DirectDirection = DirectDelta / DirectDistance;
        auto SegmentPairChecks = 0;
        for (auto EdgeAId = 0;
             EdgeAId < InNetwork._Edges.Num()
                && NOT InOutPreview._EdgeInteriorTransferWitnessBudgetExceeded;
             ++EdgeAId)
        {
            const auto ComponentA =
                Get_EdgeComponent(InNetwork, InTopology, EdgeAId);
            if (ComponentA == INDEX_NONE)
            { continue; }

            const auto& EdgeA = InNetwork._Edges[EdgeAId];
            if (EdgeA._Points.Num() < 2)
            { continue; }

            for (auto EdgeBId = EdgeAId + 1;
                 EdgeBId < InNetwork._Edges.Num()
                    && NOT InOutPreview._EdgeInteriorTransferWitnessBudgetExceeded;
                 ++EdgeBId)
            {
                const auto ComponentB =
                    Get_EdgeComponent(InNetwork, InTopology, EdgeBId);
                if (ComponentB == INDEX_NONE || ComponentA == ComponentB)
                { continue; }

                const auto& EdgeB = InNetwork._Edges[EdgeBId];
                if (EdgeB._Points.Num() < 2)
                { continue; }

                auto ClosestA = FVector::ZeroVector;
                auto ClosestB = FVector::ZeroVector;
                auto ClosestDistanceSquared =
                    TNumericLimits<double>::Max();
                for (auto SegmentA = 0;
                     SegmentA < EdgeA._Points.Num() - 1
                        && NOT InOutPreview
                            ._EdgeInteriorTransferWitnessBudgetExceeded;
                     ++SegmentA)
                {
                    for (auto SegmentB = 0;
                         SegmentB < EdgeB._Points.Num() - 1;
                         ++SegmentB)
                    {
                        if (SegmentPairChecks >= MaxSegmentPairChecks)
                        {
                            InOutPreview
                                ._EdgeInteriorTransferWitnessBudgetExceeded =
                                    true;
                            break;
                        }
                        ++SegmentPairChecks;

                        auto CandidateA = FVector::ZeroVector;
                        auto CandidateB = FVector::ZeroVector;
                        FMath::SegmentDistToSegmentSafe(
                            EdgeA._Points[SegmentA],
                            EdgeA._Points[SegmentA + 1],
                            EdgeB._Points[SegmentB],
                            EdgeB._Points[SegmentB + 1],
                            CandidateA,
                            CandidateB);
                        const auto CandidateDistanceSquared =
                            FVector::DistSquared(CandidateA, CandidateB);
                        if (CandidateDistanceSquared
                            < ClosestDistanceSquared)
                        {
                            ClosestDistanceSquared =
                                CandidateDistanceSquared;
                            ClosestA = CandidateA;
                            ClosestB = CandidateB;
                        }
                    }
                }

                if (InOutPreview._EdgeInteriorTransferWitnessBudgetExceeded)
                { break; }

                const auto ClosestDistance = static_cast<float>(
                    FMath::Sqrt(ClosestDistanceSquared));
                if (ClosestDistance > InMaxDistance)
                { continue; }

                const auto WitnessMidpoint =
                    (ClosestA + ClosestB) * 0.5;
                const auto RouteProgress = FVector::DotProduct(
                    WitnessMidpoint - InStart,
                    DirectDirection);
                if (RouteProgress < 0.0
                    || RouteProgress > DirectDistance)
                { continue; }

                const auto& NodeA0 =
                    InNetwork._Nodes[EdgeA._NodeA]._Location;
                const auto& NodeA1 =
                    InNetwork._Nodes[EdgeA._NodeB]._Location;
                const auto& NodeB0 =
                    InNetwork._Nodes[EdgeB._NodeA]._Location;
                const auto& NodeB1 =
                    InNetwork._Nodes[EdgeB._NodeB]._Location;
                const auto NearestEndpointPairDistance =
                    static_cast<float>(FMath::Min(
                        FMath::Min(
                            FVector::Dist(NodeA0, NodeB0),
                            FVector::Dist(NodeA0, NodeB1)),
                        FMath::Min(
                            FVector::Dist(NodeA1, NodeB0),
                            FVector::Dist(NodeA1, NodeB1))));
                if (NearestEndpointPairDistance <= InMaxDistance)
                {
                    // The current node-only sampler can already represent this
                    // edge pair. This experiment is interested only in physical
                    // gaps that no endpoint pair can admit.
                    continue;
                }

                ++InOutPreview._EdgeInteriorTransferWitnessCount;
                InOutPreview._EdgeInteriorTransferWitnesses.Add(
                    ck::pathnetwork_editor::designer::
                        FEdgeInteriorTransferWitness{
                            ClosestA,
                            ClosestB,
                            EdgeAId,
                            EdgeBId,
                            ComponentA,
                            ComponentB,
                            ClosestDistance,
                            NearestEndpointPairDistance});
            }
        }

        InOutPreview._EdgeInteriorTransferWitnesses.Sort(
            [](const ck::pathnetwork_editor::designer::
                    FEdgeInteriorTransferWitness& InA,
               const ck::pathnetwork_editor::designer::
                    FEdgeInteriorTransferWitness& InB)
            {
                if (InA._Distance != InB._Distance)
                { return InA._Distance < InB._Distance; }
                if (InA._EdgeA != InB._EdgeA)
                { return InA._EdgeA < InB._EdgeA; }
                return InA._EdgeB < InB._EdgeB;
            });
        if (InOutPreview._EdgeInteriorTransferWitnesses.Num()
            > MaxStoredWitnesses)
        {
            InOutPreview._EdgeInteriorTransferWitnesses.SetNum(
                MaxStoredWitnesses);
        }
    }

    auto
    Get_LocalNetworkShortcutLinkCount(
        const ck::pathnetwork::FRouteGraphSharedData& InShared) -> int32
    {
        auto DirectedLinkCount = 0;
        for (const auto& Pair : InShared._LocalNetworkShortcutsByNode)
        { DirectedLinkCount += Pair.Value.Num(); }
        return DirectedLinkCount / 2;
    }

    auto
    Get_RouteDecisionText(
        const ck::pathnetwork_editor::designer::ERoutePreviewDecision InDecision)
        -> const TCHAR*
    {
        using ck::pathnetwork_editor::designer::ERoutePreviewDecision;

        switch (InDecision)
        {
            case ERoutePreviewDecision::NetworkSelected:
                return TEXT("SIDEWALK ROUTE");
            case ERoutePreviewDecision::NetworkPreferredByMinimumSavings:
                return TEXT("SIDEWALK ROUTE: direct saving below minimum");
            case ERoutePreviewDecision::DirectCostWon:
                return TEXT("DIRECT FALLBACK: direct route selected");
            case ERoutePreviewDecision::NoStartJoin:
                return TEXT("DIRECT FALLBACK: no eligible start join");
            case ERoutePreviewDecision::NoGoalJoin:
                return TEXT("DIRECT FALLBACK: no eligible goal join");
            case ERoutePreviewDecision::NoConnectedNetworkRoute:
                return TEXT("DIRECT FALLBACK: no traversable sidewalk alternative");
            case ERoutePreviewDecision::NetworkDiagnosticIncomplete:
                return TEXT("DIRECT FALLBACK: sidewalk diagnosis reached its search limit");
            case ERoutePreviewDecision::Unavailable:
            default:
                return TEXT("ROUTE UNAVAILABLE");
        }
    }

    auto
    Get_RouteSearchOutcomeText(
        const ck::pathnetwork::ERouteSearchOutcome InOutcome)
        -> const TCHAR*
    {
        using ck::pathnetwork::ERouteSearchOutcome;

        switch (InOutcome)
        {
            case ERouteSearchOutcome::Complete:
                return TEXT("Complete");
            case ERouteSearchOutcome::Failed:
                return TEXT("Failed");
            case ERouteSearchOutcome::InProgress:
                return TEXT("InProgress");
            case ERouteSearchOutcome::CostThresholdReached:
                return TEXT("CostThresholdReached");
            case ERouteSearchOutcome::InvalidInput:
            default:
                return TEXT("InvalidInput");
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Initialize(
        UWorld* InWorld)
    -> void
{
    Clear_Preview();
    _RoutePreviewStart = FVector::ZeroVector;
    _RoutePreviewGoal = FVector::ZeroVector;
    _ActiveRouteWatchIndex = INDEX_NONE;
    _World = InWorld;
    _TargetLevel = ck::IsValid(InWorld) ? InWorld->GetCurrentLevel() : nullptr;
    _ExplicitTargetActor.Reset();
    _VisualizedActor.Reset();

    const auto Presets = ck::pathnetwork_editor::designer::Get_Presets();
    if (NOT Presets.IsEmpty())
    { Apply_Preset(Presets[0]._Owner, Presets[0]._Id); }
    else
    {
        const auto DetectorClasses =
            ck::pathnetwork_editor::authoring::Get_LoadedUsableDetectorClasses();
        if (NOT DetectorClasses.IsEmpty())
        { Set_DetectorClass(DetectorClasses[0]); }
    }

    if (ck::IsValid(InWorld))
    { Fit_BoundsToLoadedWorld(); }

    if (ck::Is_NOT_Valid(_DetectorTemplate))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("No concrete path-network detector is available in the loaded editor modules."));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    GetWorld() const
    -> UWorld*
{
    return _World.Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Use_CurrentLevel()
    -> bool
{
    auto* World = _World.Get();
    if (ck::Is_NOT_Valid(World) || World->WorldType != EWorldType::Editor)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("No usable editor world is active."));
        return false;
    }

    auto* Level = World->GetCurrentLevel();
    if (ck::Is_NOT_Valid(Level))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("The editor world has no current target level."));
        return false;
    }

    _TargetLevel = Level;
    _ExplicitTargetActor.Reset();
    _VisualizedActor.Reset();
    Clear_Preview();
    _ActiveRouteWatchIndex = INDEX_NONE;
    if (Get_RouteWatchCount() > 0)
    { Load_RouteWatch(0, false); }
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        FString::Printf(TEXT("Target level: %s"), *Level->GetOutermost()->GetName()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Fit_BoundsToLoadedWorld()
    -> bool
{
    auto* World = _World.Get();
    if (ck::Is_NOT_Valid(World))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Cannot fit bounds without an editor world."));
        return false;
    }

    auto Bounds = FBox{ForceInit};
    for (TActorIterator<AActor> It{World}; It; ++It)
    {
        const auto* Actor = *It;
        if (ck::Is_NOT_Valid(Actor)
            || Actor->IsTemplate()
            || NOT Actor->IsLevelBoundsRelevant()
            || Actor->IsA<ACk_PathNetwork_UE>())
        { continue; }

        const auto ActorBounds = Actor->GetComponentsBoundingBox(true);
        if (ActorBounds.IsValid
            && NOT ActorBounds.Min.ContainsNaN()
            && NOT ActorBounds.Max.ContainsNaN())
        { Bounds += ActorBounds; }
    }

    return Fit_Bounds(Bounds, TEXT("loaded editor world"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Fit_BoundsToSelection()
    -> bool
{
    if (ck::Is_NOT_Valid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Editor selection is unavailable."));
        return false;
    }

    auto Bounds = FBox{ForceInit};
    auto* Selection = GEditor->GetSelectedActors();
    if (Selection != nullptr)
    {
        for (FSelectionIterator It{*Selection}; It; ++It)
        {
            const auto* Actor = Cast<AActor>(*It);
            if (ck::Is_NOT_Valid(Actor) || Actor->IsA<ACk_PathNetwork_UE>())
            { continue; }

            const auto ActorBounds = Actor->GetComponentsBoundingBox(true);
            if (ActorBounds.IsValid
                && NOT ActorBounds.Min.ContainsNaN()
                && NOT ActorBounds.Max.ContainsNaN())
            { Bounds += ActorBounds; }
        }
    }

    return Fit_Bounds(Bounds, TEXT("selected actors"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Load_SelectedPathNetwork()
    -> bool
{
    if (ck::Is_NOT_Valid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    auto* SelectedNetwork = static_cast<ACk_PathNetwork_UE*>(nullptr);
    auto* Selection = GEditor->GetSelectedActors();
    if (Selection != nullptr)
    {
        for (FSelectionIterator It{*Selection}; It; ++It)
        {
            auto* Candidate = Cast<ACk_PathNetwork_UE>(*It);
            if (ck::Is_NOT_Valid(Candidate))
            { continue; }
            if (SelectedNetwork != nullptr)
            {
                Set_Status(
                    ECk_PathNetworkDesigner_Status::Error,
                    TEXT("Select only one path-network actor to load."));
                return false;
            }
            SelectedNetwork = Candidate;
        }
    }

    if (ck::Is_NOT_Valid(SelectedNetwork)
        || ck::Is_NOT_Valid(SelectedNetwork->Get_Detector()))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Select one configured Ck Path Network actor."));
        return false;
    }

    _World = SelectedNetwork->GetWorld();
    _TargetLevel = SelectedNetwork->GetLevel();
    _ExplicitTargetActor = SelectedNetwork;
    _VisualizedActor = SelectedNetwork;
    _DetectorClass = SelectedNetwork->Get_Detector()->GetClass();
    _DetectorTemplate = NewObject<UCk_PathNetwork_Detector_UE>(
        this,
        _DetectorClass,
        NAME_None,
        RF_Transient,
        SelectedNetwork->Get_Detector());
    _DetectionCenter = SelectedNetwork->Get_DetectionBounds().GetCenter();
    _DetectionExtents = SelectedNetwork->Get_DetectionExtents();
    _VectorizeParams = SelectedNetwork->Get_VectorizeParams();
    _BuildParams = SelectedNetwork->Get_BuildParams();
    _AutoDetectOnBeginPlay = SelectedNetwork->Get_AutoDetectOnBeginPlay();
    _UseRecommendedFollowerTuning =
        SelectedNetwork->Get_UseRecommendedFollowerTuning();
    _RecommendedFollowerTuning =
        SelectedNetwork->Get_RecommendedFollowerTuning();
    _ActivePresetOwner = NAME_None;
    _ActivePresetId = NAME_None;
    Clear_Preview();
    _ActiveRouteWatchIndex = INDEX_NONE;
    if (Get_RouteWatchCount() > 0)
    { Load_RouteWatch(0, false); }
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        FString::Printf(TEXT("Loaded %s. Preview before applying changes."), *SelectedNetwork->GetActorLabel()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Apply_Preset(
        const FName InOwner,
        const FName InId)
    -> bool
{
    const auto Presets = ck::pathnetwork_editor::designer::Get_Presets();
    const auto* Preset = Presets.FindByPredicate(
        [&](const ck::pathnetwork_editor::designer::FPreset& InPreset)
        {
            return InPreset._Owner == InOwner && InPreset._Id == InId;
        });
    if (Preset == nullptr)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("The selected path-network designer preset is no longer registered."));
        return false;
    }

    if (NOT Set_DetectorClass(Preset->_DetectorClass.Get()))
    { return false; }

    _DetectionExtents = Preset->_DetectionExtents;
    _VectorizeParams = Preset->_VectorizeParams;
    _BuildParams = Preset->_BuildParams;
    _AutoDetectOnBeginPlay = Preset->_AutoDetectOnBeginPlay;
    _UseRecommendedFollowerTuning =
        Preset->_UseRecommendedFollowerTuning;
    _RecommendedFollowerTuning =
        Preset->_RecommendedFollowerTuning;
    _ActivePresetOwner = Preset->_Owner;
    _ActivePresetId = Preset->_Id;
    Clear_Preview();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        Preset->_Description.ToString());
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Run_Preview()
    -> bool
{
    auto* World = _World.Get();
    const bool InputsAreValid =
        ck::IsValid(World)
        && World->WorldType == EWorldType::Editor
        && ck::IsValid(_TargetLevel)
        && _TargetLevel->GetWorld() == World
        && ck::IsValid(_DetectorTemplate);
    if (NOT InputsAreValid)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Choose a valid editor world, target level, and detector before previewing."));
        return false;
    }

    // A new detector result replaces the geometry snapshot that every route
    // preview was evaluated against. Clear both selected and watchlist results
    // before the attempt so a failed preview cannot leave stale paths visible.
    Clear_Preview();
    _Preview = ck::pathnetwork_editor::authoring::Preview(
        ck::pathnetwork_editor::authoring::FPreviewRequest{
            ._World = World,
            ._DetectorTemplate = _DetectorTemplate,
            ._DetectionBounds = Get_DetectionBounds(),
            ._VectorizeParams = _VectorizeParams});
    if (NOT _Preview._Succeeded)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _Preview._FailureReason);
        return false;
    }

    Rebuild_MaskDrawPoints();
    const auto PreviewNetwork = ck::pathnetwork::Build_NetworkFromRibbons(
        Get_ProspectivePreviewRibbons(),
        _BuildParams);
    _TopologyAnalysis = ck::pathnetwork::Analyze_NetworkTopology(PreviewNetwork);
    _HasTopologyAnalysis = true;
    _RoutePreview = {};
    if (_UseRecommendedFollowerTuning == ECk_EnableDisable::Enable
        && UCk_Utils_PathNetworkFollower_UE::Get_IsTuningValid(
            _RecommendedFollowerTuning))
    {
        const auto PreviewPolicy =
            ck::pathnetwork::Resolve_RouteCostPolicy(_RecommendedFollowerTuning);
        const auto PreviewShared = ck::pathnetwork::Build_RouteGraphSharedData(
            PreviewNetwork,
            FVector::ZeroVector,
            FVector::ZeroVector,
            PreviewPolicy);
        _RoutePreview._NetworkComponentCount = _TopologyAnalysis._ComponentCount;
        _RoutePreview._ComponentTransferMaxDistance =
            PreviewPolicy._ComponentTransferMaxDistance;
        _RoutePreview._ConfiguredNetworkGapCostMultiplier =
            _RecommendedFollowerTuning.Get_NetworkGapCostMultiplier();
        _RoutePreview._EffectiveNetworkGapCostMultiplier =
            PreviewPolicy._NetworkGapCostMultiplier;
        ck_pathnetwork_designer_session::Populate_AdmittedComponentTransfers(
            _RoutePreview,
            PreviewNetwork,
            *PreviewShared);
    }
    Set_Status(
        ECk_PathNetworkDesigner_Status::PreviewReady,
        FString::Printf(
            TEXT("Preview ready: %d occupied cells, %d generated ribbons, %d routable components, %d admitted component transfers from %d candidates, including %d edge-interior opportunities (%d rejected by cell cap). Network-gap cost configured %.2f, effective %.2f. The level is unchanged."),
            _Preview._OccupiedCellCount,
            _Preview._GeneratedWorldRibbons.Num(),
            _TopologyAnalysis._ComponentCount,
            _RoutePreview._ComponentTransferLinkCount,
            _RoutePreview._ComponentTransferCandidateCount,
            _RoutePreview
                ._ComponentTransferEdgeInteriorCandidateCount,
            _RoutePreview._ComponentTransferRejectedByCellCapCount,
            _RoutePreview._ConfiguredNetworkGapCostMultiplier,
            _RoutePreview._EffectiveNetworkGapCostMultiplier));

    if (FVector::Dist(_RoutePreviewStart, _RoutePreviewGoal) > 1.0)
    { Run_RoutePreview(); }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Capture_RouteStartFromSelection()
    -> bool
{
    return Capture_RouteEndpointFromSelection(true);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Capture_RouteGoalFromSelection()
    -> bool
{
    return Capture_RouteEndpointFromSelection(false);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Set_RoutePreviewEndpoints(
        const FVector& InStart,
        const FVector& InGoal)
    -> void
{
    _RoutePreviewStart = InStart;
    _RoutePreviewGoal = InGoal;
    Clear_RoutePreview();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Run_RoutePreview()
    -> bool
{
    using namespace ck::pathnetwork;
    using namespace ck_pathnetwork_designer_session;

    Clear_RoutePreview();

    if (_UseRecommendedFollowerTuning != ECk_EnableDisable::Enable)
    {
        _RoutePreview._FailureReason =
            TEXT("Enable Use Map Route Preferences before previewing a representative route. Disabled maps do not publish this profile to runtime followers.");
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _RoutePreview._FailureReason);
        return false;
    }

    const bool InputsAreValid =
        _Preview._Succeeded
        && NOT _RoutePreviewStart.ContainsNaN()
        && NOT _RoutePreviewGoal.ContainsNaN()
        && FVector::Dist(_RoutePreviewStart, _RoutePreviewGoal) > 1.0
        && UCk_Utils_PathNetworkFollower_UE::Get_IsTuningValid(
            _RecommendedFollowerTuning);
    if (NOT InputsAreValid)
    {
        _RoutePreview._FailureReason =
            TEXT("Preview the network, then capture or enter distinct finite route endpoints and valid route preferences.");
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _RoutePreview._FailureReason);
        return false;
    }

    const auto BuiltNetwork =
        Build_NetworkFromRibbons(Get_ProspectivePreviewRibbons(), _BuildParams);
    if (BuiltNetwork._Edges.IsEmpty())
    {
        _RoutePreview._FailureReason =
            TEXT("The current detector preview produced no routable network edges.");
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _RoutePreview._FailureReason);
        return false;
    }

    const auto RouteTopology = Analyze_NetworkTopology(BuiltNetwork);
    const auto CostPolicy =
        Resolve_RouteCostPolicy(_RecommendedFollowerTuning);
    const auto Shared = Build_RouteGraphSharedData(
        BuiltNetwork,
        _RoutePreviewStart,
        _RoutePreviewGoal,
        CostPolicy);
    _RoutePreview._NetworkComponentCount = RouteTopology._ComponentCount;
    _RoutePreview._ComponentTransferMaxDistance =
        CostPolicy._ComponentTransferMaxDistance;
    _RoutePreview._ConfiguredNetworkGapCostMultiplier =
        _RecommendedFollowerTuning.Get_NetworkGapCostMultiplier();
    _RoutePreview._EffectiveNetworkGapCostMultiplier =
        CostPolicy._NetworkGapCostMultiplier;
    Populate_AdmittedComponentTransfers(
        _RoutePreview,
        BuiltNetwork,
        *Shared);
    Populate_EdgeInteriorTransferWitnesses(
        _RoutePreview,
        BuiltNetwork,
        RouteTopology,
        _RoutePreviewStart,
        _RoutePreviewGoal,
        CostPolicy._ComponentTransferMaxDistance);
    for (const auto& Witness :
         _RoutePreview._EdgeInteriorTransferWitnesses)
    {
        ck::pathnetwork_editor::Display(
            TEXT("[RouteTransferEdgeInteriorWitness] EdgeA=[{}] ComponentA=[{}] LocationA=[{}] EdgeB=[{}] ComponentB=[{}] LocationB=[{}] Distance=[{}] NearestEndpointPairDistance=[{}]"),
            Witness._EdgeA,
            Witness._ComponentA,
            Witness._Start,
            Witness._EdgeB,
            Witness._ComponentB,
            Witness._End,
            Witness._Distance,
            Witness._NearestEndpointPairDistance);
    }
    ck::pathnetwork_editor::Display(
        TEXT("[RouteTransferEdgeInteriorWitnessSummary] Count=[{}] Stored=[{}] SegmentPairBudgetExceeded=[{}]"),
        _RoutePreview._EdgeInteriorTransferWitnessCount,
        _RoutePreview._EdgeInteriorTransferWitnesses.Num(),
        _RoutePreview._EdgeInteriorTransferWitnessBudgetExceeded);
    const auto Plan = Search_RouteGraph(
        BuiltNetwork,
        _RoutePreviewStart,
        _RoutePreviewGoal,
        CostPolicy,
        Shared);
    if (NOT Plan._Succeeded || Plan._Spans.IsEmpty())
    {
        _RoutePreview._NetworkDiagnosticOutcome =
            Get_RouteSearchOutcomeText(Plan._SearchOutcome);
        _RoutePreview._FailureReason = FString::Printf(
            TEXT("No representative route could be planned with the current preferences. Search outcome: %s."),
            *_RoutePreview._NetworkDiagnosticOutcome);
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _RoutePreview._FailureReason);
        ck::pathnetwork_editor::Display(
            TEXT("[RouteDecision] Decision=[ROUTE UNAVAILABLE] SearchOutcome=[{}] Start=[{}] Goal=[{}]"),
            _RoutePreview._NetworkDiagnosticOutcome,
            _RoutePreviewStart,
            _RoutePreviewGoal);
        return false;
    }

    const auto StartCandidates = Gather_RouteEndpointCandidates(
        BuiltNetwork,
        _RoutePreviewStart,
        CostPolicy);
    const auto GoalCandidates = Gather_RouteEndpointCandidates(
        BuiltNetwork,
        _RoutePreviewGoal,
        CostPolicy);
    _RoutePreview._StartCandidateCount =
        StartCandidates.Num();
    _RoutePreview._GoalCandidateCount =
        GoalCandidates.Num();
    _RoutePreview._LocalNetworkShortcutLinkCount =
        Get_LocalNetworkShortcutLinkCount(*Shared);
    _RoutePreview._LocalNetworkShortcutCandidateCount =
        Shared->_LocalNetworkShortcutCandidateCount;
    _RoutePreview._LocalNetworkShortcutCandidateSourceCount =
        Shared->_LocalNetworkShortcutCandidateSourceCount;
    _RoutePreview._LocalNetworkShortcutBudgetExceeded =
        Shared->_LocalNetworkShortcutBudgetExceeded;
    _RoutePreview._NearestStartNetworkDistance =
        Get_NearestNetworkDistance(BuiltNetwork, _RoutePreviewStart);
    _RoutePreview._NearestGoalNetworkDistance =
        Get_NearestNetworkDistance(BuiltNetwork, _RoutePreviewGoal);
    _RoutePreview._EndpointJoinMaxDistance =
        CostPolicy._EndpointJoinMaxDistance;
    _RoutePreview._ComponentTransferMaxDistance =
        CostPolicy._ComponentTransferMaxDistance;
    _RoutePreview._LocalNetworkShortcutMaxDistance =
        CostPolicy._LocalNetworkShortcutMaxDistance;
    _RoutePreview._DirectRouteMinimumSavingsFraction =
        CostPolicy._DirectRouteMinimumSavingsFraction;
    _RoutePreview._DirectTripGraceApplied =
        CostPolicy._DirectTripGraceDistance > 0.0f
        && FVector::Dist(_RoutePreviewStart, _RoutePreviewGoal)
            <= CostPolicy._DirectTripGraceDistance;

    for (const auto& Transfer : _RoutePreview._ComponentTransferSegments)
    {
        const auto EndpointA = FRouteNodeId{
            Transfer._KindA,
            Transfer._NodeA};
        const auto EndpointB = FRouteNodeId{
            Transfer._KindB,
            Transfer._NodeB};
        const auto ComponentA = Get_RouteNodeComponent(
            BuiltNetwork,
            RouteTopology,
            *Shared,
            EndpointA);
        const auto ComponentB = Get_RouteNodeComponent(
            BuiltNetwork,
            RouteTopology,
            *Shared,
            EndpointB);
        ck::pathnetwork_editor::Display(
            TEXT("[RouteTransferAdmitted] KindA=[{}] IndexA=[{}] ComponentA=[{}] LocationA=[{}] KindB=[{}] IndexB=[{}] ComponentB=[{}] LocationB=[{}] Distance=[{}]"),
            static_cast<int32>(Transfer._KindA),
            Transfer._NodeA,
            ComponentA,
            Transfer._Start,
            static_cast<int32>(Transfer._KindB),
            Transfer._NodeB,
            ComponentB,
            Transfer._End,
            Transfer._Distance);
    }

    const auto StartId = FRouteNodeId{ERouteNodeKind::Start, 0};
    const auto GoalId = FRouteNodeId{ERouteNodeKind::Goal, 0};
    const auto DiagnosticGraph = FRouteGraph{
        &BuiltNetwork,
        _RoutePreviewStart,
        _RoutePreviewGoal,
        CostPolicy,
        Shared};
    _RoutePreview._DirectEstimatedCost =
        DiagnosticGraph.Cost(StartId, GoalId);

    const auto SelectedUsesNetwork = Uses_Network(Plan);
    if (SelectedUsesNetwork)
    {
        _RoutePreview._HasNetworkAlternative = true;
        _RoutePreview._BestNetworkEstimatedCost =
            Plan._EstimatedCost;
        if (_RoutePreview._BestNetworkEstimatedCost > 0.0f)
        {
            _RoutePreview._DirectRouteSavingsFraction =
                (_RoutePreview._BestNetworkEstimatedCost
                    - _RoutePreview._DirectEstimatedCost)
                / _RoutePreview._BestNetworkEstimatedCost;
        }
        const bool NetworkPreferredByMinimumSavings =
            NOT _RoutePreview._DirectTripGraceApplied
            && _RoutePreview._DirectRouteMinimumSavingsFraction > 0.0f
            && _RoutePreview._DirectEstimatedCost
                < _RoutePreview._BestNetworkEstimatedCost
            && _RoutePreview._DirectEstimatedCost
                > _RoutePreview._BestNetworkEstimatedCost
                    * (1.0f
                        - _RoutePreview
                            ._DirectRouteMinimumSavingsFraction);
        _RoutePreview._Decision =
            NetworkPreferredByMinimumSavings
            ? ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::
                    NetworkPreferredByMinimumSavings
            : ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::NetworkSelected;
        _RoutePreview._NetworkDiagnosticOutcome =
            TEXT("SelectedPlanUsesNetwork");
    }
    else if (_RoutePreview._StartCandidateCount <= 0)
    {
        _RoutePreview._Decision =
            ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::NoStartJoin;
        _RoutePreview._NetworkDiagnosticOutcome =
            TEXT("NotRunNoStartJoin");
    }
    else if (_RoutePreview._GoalCandidateCount <= 0)
    {
        _RoutePreview._Decision =
            ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::NoGoalJoin;
        _RoutePreview._NetworkDiagnosticOutcome =
            TEXT("NotRunNoGoalJoin");
    }
    else
    {
        auto NetworkOnlyShared =
            MakeShared<FRouteGraphSharedData>(*Shared);
        NetworkOnlyShared->_AllowDirectStartToGoal = false;
        const auto NetworkPlan = Search_RouteGraph(
            BuiltNetwork,
            _RoutePreviewStart,
            _RoutePreviewGoal,
            CostPolicy,
            NetworkOnlyShared);
        _RoutePreview._NetworkDiagnosticOutcome =
            Get_RouteSearchOutcomeText(NetworkPlan._SearchOutcome);
        const auto HasNetworkAlternative =
            NetworkPlan._SearchOutcome == ERouteSearchOutcome::Complete
            && Uses_Network(NetworkPlan);
        _RoutePreview._HasNetworkAlternative =
            HasNetworkAlternative;
        if (HasNetworkAlternative)
        {
            _RoutePreview._Decision =
                ck::pathnetwork_editor::designer::
                    ERoutePreviewDecision::DirectCostWon;
            _RoutePreview._BestNetworkEstimatedCost =
                NetworkPlan._EstimatedCost;
            if (_RoutePreview._BestNetworkEstimatedCost > 0.0f)
            {
                _RoutePreview._DirectRouteSavingsFraction =
                    (_RoutePreview._BestNetworkEstimatedCost
                        - _RoutePreview._DirectEstimatedCost)
                    / _RoutePreview._BestNetworkEstimatedCost;
            }
        }
        else if (NetworkPlan._SearchOutcome == ERouteSearchOutcome::Complete
            || NetworkPlan._SearchOutcome == ERouteSearchOutcome::Failed)
        {
            _RoutePreview._Decision =
                ck::pathnetwork_editor::designer::
                    ERoutePreviewDecision::NoConnectedNetworkRoute;
        }
        else
        {
            _RoutePreview._Decision =
                ck::pathnetwork_editor::designer::
                    ERoutePreviewDecision::NetworkDiagnosticIncomplete;
        }
    }

    _RoutePreview._EstimatedCost = Plan._EstimatedCost;
    auto SelectedTransferKeys = TSet<uint64>{};
    for (auto SpanIndex = 0; SpanIndex < Plan._Spans.Num();)
    {
        const auto& Span = Plan._Spans[SpanIndex];
        if (Span._IsOffPath)
        {
            ++_RoutePreview._OffNetworkLegCount;
            if (DiagnosticGraph.Get_IsComponentTransferHop(
                    Span._FromId,
                    Span._ToId))
            {
                const auto TransferKey = FMath::Min(
                    FRouteGraph::PackOffPathKey(
                        Span._FromId,
                        Span._ToId),
                    FRouteGraph::PackOffPathKey(
                        Span._ToId,
                        Span._FromId));
                if (NOT SelectedTransferKeys.Contains(TransferKey))
                {
                    SelectedTransferKeys.Add(TransferKey);
                    const auto Segment = Make_TransferSegment(
                        BuiltNetwork,
                        *Shared,
                        Span._FromId,
                        Span._ToId);
                    ++_RoutePreview._ComponentTransferLegCount;
                    _RoutePreview._LongestComponentTransferDistance = FMath::Max(
                        _RoutePreview._LongestComponentTransferDistance,
                        Segment._Distance);
                    _RoutePreview._SelectedComponentTransferSegments.Add(Segment);
                    ck::pathnetwork_editor::Display(
                        TEXT("[RouteTransferSelected] KindA=[{}] IndexA=[{}] LocationA=[{}] KindB=[{}] IndexB=[{}] LocationB=[{}] Distance=[{}] SelectedSpanCost=[{}]"),
                        static_cast<int32>(Span._FromId._Kind),
                        Span._FromId._Index,
                        Segment._Start,
                        static_cast<int32>(Span._ToId._Kind),
                        Span._ToId._Index,
                        Segment._End,
                        Segment._Distance,
                        DiagnosticGraph.Cost(Span._FromId, Span._ToId));
                }
            }
            else if (DiagnosticGraph.Get_IsLocalNetworkShortcutHop(
                    Span._FromId,
                    Span._ToId))
            {
                const auto ShortcutDistance = static_cast<float>(
                    FVector::Dist(
                        Span._FromLocation,
                        Span._ToLocation));
                ++_RoutePreview._LocalNetworkShortcutLegCount;
                _RoutePreview._LongestLocalNetworkShortcutDistance =
                    FMath::Max(
                        _RoutePreview
                            ._LongestLocalNetworkShortcutDistance,
                        ShortcutDistance);
                _RoutePreview._LocalNetworkShortcutSegments.Add(
                    ck::pathnetwork_editor::designer::
                        FRoutePreviewSegment{
                            Span._FromLocation,
                            Span._ToLocation,
                            FMath::Min(Span._FromId._Index, Span._ToId._Index),
                            FMath::Max(Span._FromId._Index, Span._ToId._Index),
                            ShortcutDistance});
            }
            Append_RouteWaypoint(
                _RoutePreview._CompiledWaypoints,
                Span._FromLocation);
            Append_RouteWaypoint(
                _RoutePreview._CompiledWaypoints,
                Span._ToLocation);

            if (Span._FromId._Kind == ERouteNodeKind::OverlayPoint)
            {
                Append_JoinPoint(
                    _RoutePreview._JoinPoints,
                    Span._FromLocation);
            }
            if (Span._ToId._Kind == ERouteNodeKind::OverlayPoint)
            {
                Append_JoinPoint(
                    _RoutePreview._JoinPoints,
                    Span._ToLocation);
            }
            ++SpanIndex;
            continue;
        }

        const auto RunStartIndex = SpanIndex;
        while (SpanIndex < Plan._Spans.Num()
            && NOT Plan._Spans[SpanIndex]._IsOffPath)
        {
            ++_RoutePreview._OnNetworkLegCount;
            ++SpanIndex;
        }

        const auto RunSpans = TConstArrayView<FRouteLegSpan>{
            Plan._Spans.GetData() + RunStartIndex,
            SpanIndex - RunStartIndex};
        auto CompileParams = FCorridorCompileParams{};
        CompileParams._SideKeepingFraction =
            _RecommendedFollowerTuning.Get_SideKeepingFraction();
        CompileParams._WaypointSpacing =
            _RecommendedFollowerTuning.Get_CorridorWaypointSpacing();
        CompileParams._CornerSmoothingDistance =
            _RecommendedFollowerTuning.Get_CornerSmoothingDistance();
        CompileParams._RampSideOffsetAtStart =
            RunStartIndex > 0
            && Plan._Spans[RunStartIndex - 1]._IsOffPath;
        CompileParams._RampSideOffsetAtEnd =
            SpanIndex < Plan._Spans.Num()
            && Plan._Spans[SpanIndex]._IsOffPath;

        const auto RunWaypoints =
            Compile_OnRibbonRun(BuiltNetwork, RunSpans, CompileParams);
        if (RunWaypoints.Num() < 2)
        {
            _RoutePreview = {};
            _RoutePreview._FailureReason =
                TEXT("The selected route could not be compiled inside its ribbon corridor.");
            Set_Status(
                ECk_PathNetworkDesigner_Status::Error,
                _RoutePreview._FailureReason);
            return false;
        }

        for (const auto& Waypoint : RunWaypoints)
        {
            Append_RouteWaypoint(
                _RoutePreview._CompiledWaypoints,
                Waypoint);
        }
    }

    if (_RoutePreview._CompiledWaypoints.Num() < 2)
    {
        _RoutePreview = {};
        _RoutePreview._FailureReason =
            TEXT("The representative route did not produce a drawable path.");
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            _RoutePreview._FailureReason);
        return false;
    }

    _RoutePreview._UsesNetwork = SelectedUsesNetwork;
    _RoutePreview._Succeeded = true;
    Set_Status(
        ECk_PathNetworkDesigner_Status::PreviewReady,
        FString::Printf(
            TEXT("%s. Selected cost %.0f, direct cost %.0f, direct saving %.2f%%, required saving %.2f%%, short-trip grace %s, network-gap cost configured %.2f and effective %.2f, %d start candidates, %d goal candidates, %d network components, %d component-transfer candidates, %d rejected by cell cap, %d admitted links, %d selected transfers, %d local-shortcut links, %d local shortcuts used, local-shortcut budget %s, %d on-network and %d off-network legs. This is a geometry/graph estimate; off-network spans are not navmesh-validated in the editor preview."),
            Get_RouteDecisionText(_RoutePreview._Decision),
            _RoutePreview._EstimatedCost,
            _RoutePreview._DirectEstimatedCost,
            _RoutePreview._DirectRouteSavingsFraction * 100.0f,
            _RoutePreview._DirectRouteMinimumSavingsFraction * 100.0f,
            _RoutePreview._DirectTripGraceApplied
                ? TEXT("applied")
                : TEXT("not applied"),
            _RoutePreview._ConfiguredNetworkGapCostMultiplier,
            _RoutePreview._EffectiveNetworkGapCostMultiplier,
            _RoutePreview._StartCandidateCount,
            _RoutePreview._GoalCandidateCount,
            _RoutePreview._NetworkComponentCount,
            _RoutePreview._ComponentTransferCandidateCount,
            _RoutePreview._ComponentTransferRejectedByCellCapCount,
            _RoutePreview._ComponentTransferLinkCount,
            _RoutePreview._ComponentTransferLegCount,
            _RoutePreview._LocalNetworkShortcutLinkCount,
            _RoutePreview._LocalNetworkShortcutLegCount,
            _RoutePreview._LocalNetworkShortcutBudgetExceeded
                ? TEXT("exceeded")
                : TEXT("within limit"),
            _RoutePreview._OnNetworkLegCount,
            _RoutePreview._OffNetworkLegCount));
    ck::pathnetwork_editor::Display(
        TEXT("[RouteDecision] Decision=[{}] NetworkDiagnostic=[{}] Start=[{}] Goal=[{}] SelectedCost=[{}] DirectCost=[{}] BestNetworkCost=[{}] DirectSavingsFraction=[{}] MinimumDirectSavingsFraction=[{}] DirectTripGraceApplied=[{}] HasNetworkAlternative=[{}] StartCandidates=[{}] GoalCandidates=[{}] NearestStartNetwork=[{}] NearestGoalNetwork=[{}] EndpointJoinMaxDistance=[{}] NetworkComponents=[{}] ComponentTransferMaxDistance=[{}] NetworkGapCostMultiplierConfigured=[{}] NetworkGapCostMultiplierEffective=[{}] ComponentTransferCandidates=[{}] ComponentTransferEdgeInteriorCandidates=[{}] ComponentTransferRejectedByCellCap=[{}] ComponentTransferLinks=[{}] ComponentTransferLegs=[{}] LongestComponentTransfer=[{}] LocalNetworkShortcutMaxDistance=[{}] LocalNetworkShortcutCandidates=[{}] LocalNetworkShortcutCandidateSources=[{}] LocalNetworkShortcutBudgetExceeded=[{}] LocalNetworkShortcutLinks=[{}] LocalNetworkShortcutLegs=[{}] LongestLocalNetworkShortcut=[{}] NearEndpointCostMultiplier=[{}] FarOrDirectCostMultiplier=[{}] DirectTripGraceDistance=[{}] OnNetworkLegs=[{}] OffNetworkLegs=[{}]"),
        Get_RouteDecisionText(_RoutePreview._Decision),
        _RoutePreview._NetworkDiagnosticOutcome,
        _RoutePreviewStart,
        _RoutePreviewGoal,
        _RoutePreview._EstimatedCost,
        _RoutePreview._DirectEstimatedCost,
        _RoutePreview._BestNetworkEstimatedCost,
        _RoutePreview._DirectRouteSavingsFraction,
        _RoutePreview._DirectRouteMinimumSavingsFraction,
        _RoutePreview._DirectTripGraceApplied,
        _RoutePreview._HasNetworkAlternative,
        _RoutePreview._StartCandidateCount,
        _RoutePreview._GoalCandidateCount,
        _RoutePreview._NearestStartNetworkDistance,
        _RoutePreview._NearestGoalNetworkDistance,
        _RoutePreview._EndpointJoinMaxDistance,
        _RoutePreview._NetworkComponentCount,
        _RoutePreview._ComponentTransferMaxDistance,
        _RoutePreview._ConfiguredNetworkGapCostMultiplier,
        _RoutePreview._EffectiveNetworkGapCostMultiplier,
        _RoutePreview._ComponentTransferCandidateCount,
        _RoutePreview
            ._ComponentTransferEdgeInteriorCandidateCount,
        _RoutePreview._ComponentTransferRejectedByCellCapCount,
        _RoutePreview._ComponentTransferLinkCount,
        _RoutePreview._ComponentTransferLegCount,
        _RoutePreview._LongestComponentTransferDistance,
        _RoutePreview._LocalNetworkShortcutMaxDistance,
        _RoutePreview._LocalNetworkShortcutCandidateCount,
        _RoutePreview._LocalNetworkShortcutCandidateSourceCount,
        _RoutePreview._LocalNetworkShortcutBudgetExceeded,
        _RoutePreview._LocalNetworkShortcutLinkCount,
        _RoutePreview._LocalNetworkShortcutLegCount,
        _RoutePreview._LongestLocalNetworkShortcutDistance,
        CostPolicy._NearEndpointCostMultiplier,
        CostPolicy._FarOrDirectCostMultiplier,
        CostPolicy._DirectTripGraceDistance,
        _RoutePreview._OnNetworkLegCount,
        _RoutePreview._OffNetworkLegCount);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Resolve_RouteWatchActor() const
    -> ACk_PathNetwork_UE*
{
    if (auto* ExplicitActor = _ExplicitTargetActor.Get();
        ck::IsValid(ExplicitActor))
    { return ExplicitActor; }

    auto* TargetLevel = _TargetLevel.Get();
    if (ck::Is_NOT_Valid(TargetLevel))
    { return nullptr; }

    auto* FoundActor = static_cast<ACk_PathNetwork_UE*>(nullptr);
    for (const auto& Actor : TargetLevel->Actors)
    {
        auto* Candidate = Cast<ACk_PathNetwork_UE>(Actor.Get());
        if (ck::Is_NOT_Valid(Candidate))
        { continue; }
        if (FoundActor != nullptr)
        { return nullptr; }
        FoundActor = Candidate;
    }
    return FoundActor;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Get_RouteWatchNames() const
    -> TArray<FName>
{
    auto Names = TArray<FName>{};
    const auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    { return Names; }

    const auto& Routes = Actor->Get_RepresentativeRoutes();
    Names.Reserve(Routes.Num());
    for (const auto& Route : Routes)
    { Names.Add(Route.Get_Name()); }
    return Names;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Get_RouteWatchCount() const
    -> int32
{
    const auto* Actor = Resolve_RouteWatchActor();
    return ck::IsValid(Actor)
        ? Actor->Get_RepresentativeRoutes().Num()
        : 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Get_CanPersistRouteWatches() const
    -> bool
{
    return ck::IsValid(Resolve_RouteWatchActor());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Persist_RouteWatches(
        const TArray<FCk_PathNetwork_RepresentativeRoute>& InWorldRoutes,
        const FText& InTransactionText)
    -> bool
{
    auto* Actor = Resolve_RouteWatchActor();
    auto* Level = ck::IsValid(Actor) ? Actor->GetLevel() : nullptr;
    if (ck::Is_NOT_Valid(Actor) || ck::Is_NOT_Valid(Level))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Apply or load one path-network actor before saving route watches."));
        return false;
    }

    auto RelativeRoutes = TArray<FCk_PathNetwork_RepresentativeRoute>{};
    RelativeRoutes.Reserve(InWorldRoutes.Num());
    for (const auto& Route : InWorldRoutes)
    {
        RelativeRoutes.Add(
            Actor->Convert_WorldRepresentativeRouteToRelative(Route));
    }

    const auto Transaction = FScopedTransaction{InTransactionText};
    Actor->Modify();
    Actor->Set_RepresentativeRoutes(RelativeRoutes);
    Actor->PostEditChange();
    Level->MarkPackageDirty();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Load_RouteWatch(
        const int32 InRouteIndex,
        const bool InSetStatus)
    -> bool
{
    const auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    {
        if (InSetStatus)
        {
            Set_Status(
                ECk_PathNetworkDesigner_Status::Error,
                TEXT("Apply or load one path-network actor before selecting route watches."));
        }
        return false;
    }

    const auto Routes = Actor->Get_WorldRepresentativeRoutes();
    if (NOT Routes.IsValidIndex(InRouteIndex))
    {
        if (InSetStatus)
        {
            Set_Status(
                ECk_PathNetworkDesigner_Status::Error,
                TEXT("The selected route watch no longer exists."));
        }
        return false;
    }

    const auto& Route = Routes[InRouteIndex];
    _ActiveRouteWatchIndex = InRouteIndex;
    _RoutePreviewStart = Route.Get_Start();
    _RoutePreviewGoal = Route.Get_Goal();
    Clear_RoutePreview();
    if (InSetStatus)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Ready,
            FString::Printf(
                TEXT("Loaded route watch '%s'. Refresh the selected route to inspect it."),
                *Route.Get_Name().ToString()));
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Select_RouteWatch(
        const int32 InRouteIndex)
    -> bool
{
    return Load_RouteWatch(InRouteIndex, true);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Add_RouteWatch(
        const FName InName)
    -> bool
{
    auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Apply or load one path-network actor before adding route watches."));
        return false;
    }

    const bool InputIsValid =
        InName != NAME_None
        && NOT _RoutePreviewStart.ContainsNaN()
        && NOT _RoutePreviewGoal.ContainsNaN()
        && FVector::Dist(_RoutePreviewStart, _RoutePreviewGoal) > 1.0;
    if (NOT InputIsValid)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("A route watch requires a name and distinct finite start and goal locations."));
        return false;
    }

    auto Routes = Actor->Get_WorldRepresentativeRoutes();
    const bool NameAlreadyExists = Routes.ContainsByPredicate(
        [&](const FCk_PathNetwork_RepresentativeRoute& InRoute)
        { return InRoute.Get_Name() == InName; });
    if (NameAlreadyExists)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            FString::Printf(
                TEXT("A route watch named '%s' already exists."),
                *InName.ToString()));
        return false;
    }

    Routes.Emplace(InName, _RoutePreviewStart, _RoutePreviewGoal);
    if (NOT Persist_RouteWatches(
            Routes,
            NSLOCTEXT(
                "CkPathNetworkEditor",
                "AddRouteWatchTransaction",
                "Path Network: Add Route Watch")))
    { return false; }

    _ActiveRouteWatchIndex = Routes.Num() - 1;
    Invalidate_RouteWatchPreviews();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Applied,
        FString::Printf(
            TEXT("Saved route watch '%s' on the level path network. Undo is available."),
            *InName.ToString()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Save_ActiveRouteWatch(
        const FName InName)
    -> bool
{
    auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Apply or load one path-network actor before saving route watches."));
        return false;
    }

    auto Routes = Actor->Get_WorldRepresentativeRoutes();
    if (NOT Routes.IsValidIndex(_ActiveRouteWatchIndex))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Select a saved route watch before updating it."));
        return false;
    }

    const bool InputIsValid =
        InName != NAME_None
        && NOT _RoutePreviewStart.ContainsNaN()
        && NOT _RoutePreviewGoal.ContainsNaN()
        && FVector::Dist(_RoutePreviewStart, _RoutePreviewGoal) > 1.0;
    if (NOT InputIsValid)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("A route watch requires a name and distinct finite start and goal locations."));
        return false;
    }

    const auto DuplicateIndex = Routes.IndexOfByPredicate(
        [&](const FCk_PathNetwork_RepresentativeRoute& InRoute)
        { return InRoute.Get_Name() == InName; });
    if (DuplicateIndex != INDEX_NONE
        && DuplicateIndex != _ActiveRouteWatchIndex)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            FString::Printf(
                TEXT("A route watch named '%s' already exists."),
                *InName.ToString()));
        return false;
    }

    Routes[_ActiveRouteWatchIndex] =
        FCk_PathNetwork_RepresentativeRoute{
            InName,
            _RoutePreviewStart,
            _RoutePreviewGoal};
    if (NOT Persist_RouteWatches(
            Routes,
            NSLOCTEXT(
                "CkPathNetworkEditor",
                "UpdateRouteWatchTransaction",
                "Path Network: Update Route Watch")))
    { return false; }

    Invalidate_RouteWatchPreviews();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Applied,
        FString::Printf(
            TEXT("Updated route watch '%s'. Undo is available."),
            *InName.ToString()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Remove_ActiveRouteWatch()
    -> bool
{
    auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Apply or load one path-network actor before removing route watches."));
        return false;
    }

    auto Routes = Actor->Get_WorldRepresentativeRoutes();
    if (NOT Routes.IsValidIndex(_ActiveRouteWatchIndex))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Select a saved route watch before removing it."));
        return false;
    }

    const auto RemovedName = Routes[_ActiveRouteWatchIndex].Get_Name();
    const auto RemovedIndex = _ActiveRouteWatchIndex;
    Routes.RemoveAt(RemovedIndex);
    if (NOT Persist_RouteWatches(
            Routes,
            NSLOCTEXT(
                "CkPathNetworkEditor",
                "RemoveRouteWatchTransaction",
                "Path Network: Remove Route Watch")))
    { return false; }

    _ActiveRouteWatchIndex = Routes.IsEmpty()
        ? INDEX_NONE
        : FMath::Min(RemovedIndex, Routes.Num() - 1);
    if (Routes.IsValidIndex(_ActiveRouteWatchIndex))
    {
        _RoutePreviewStart =
            Routes[_ActiveRouteWatchIndex].Get_Start();
        _RoutePreviewGoal =
            Routes[_ActiveRouteWatchIndex].Get_Goal();
    }
    else
    {
        _RoutePreviewStart = FVector::ZeroVector;
        _RoutePreviewGoal = FVector::ZeroVector;
    }
    Clear_RoutePreview();
    Invalidate_RouteWatchPreviews();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Applied,
        FString::Printf(
            TEXT("Removed route watch '%s'. Undo is available."),
            *RemovedName.ToString()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Refresh_AllRouteWatches()
    -> bool
{
    const auto* Actor = Resolve_RouteWatchActor();
    if (ck::Is_NOT_Valid(Actor))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Apply or load one path-network actor before refreshing route watches."));
        return false;
    }
    if (NOT _Preview._Succeeded)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Preview the current network before refreshing route watches."));
        return false;
    }

    const auto Routes = Actor->Get_WorldRepresentativeRoutes();
    if (Routes.IsEmpty())
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Add at least one saved route watch before refreshing all."));
        return false;
    }

    const auto WorkingStart = _RoutePreviewStart;
    const auto WorkingGoal = _RoutePreviewGoal;
    const auto WorkingPreview = _RoutePreview;

    auto Refreshed = TArray<
        ck::pathnetwork_editor::designer::FRouteWatchPreview>{};
    Refreshed.Reserve(Routes.Num());
    auto UnavailableCount = 0;
    auto NetworkSelectedCount = 0;
    for (auto RouteIndex = 0; RouteIndex < Routes.Num(); ++RouteIndex)
    {
        const auto& Route = Routes[RouteIndex];
        _RoutePreviewStart = Route.Get_Start();
        _RoutePreviewGoal = Route.Get_Goal();
        if (NOT Run_RoutePreview())
        { ++UnavailableCount; }
        if (_RoutePreview._UsesNetwork)
        { ++NetworkSelectedCount; }

        Refreshed.Add(
            ck::pathnetwork_editor::designer::FRouteWatchPreview{
                ._RouteIndex = RouteIndex,
                ._Name = Route.Get_Name(),
                ._Start = Route.Get_Start(),
                ._Goal = Route.Get_Goal(),
                ._Preview = _RoutePreview});
    }

    if (Refreshed.IsValidIndex(_ActiveRouteWatchIndex))
    {
        const auto& ActiveRoute = Refreshed[_ActiveRouteWatchIndex];
        _RoutePreviewStart = ActiveRoute._Start;
        _RoutePreviewGoal = ActiveRoute._Goal;
        _RoutePreview = ActiveRoute._Preview;
    }
    else
    {
        _RoutePreviewStart = WorkingStart;
        _RoutePreviewGoal = WorkingGoal;
        _RoutePreview = WorkingPreview;
    }
    _RouteWatchPreviews = MoveTemp(Refreshed);
    Set_Status(
        UnavailableCount == 0
            ? ECk_PathNetworkDesigner_Status::PreviewReady
            : ECk_PathNetworkDesigner_Status::Error,
        FString::Printf(
            TEXT("Refreshed %d route watches: %d sidewalk routes, %d direct fallbacks, %d unavailable."),
            Routes.Num(),
            NetworkSelectedCount,
            Routes.Num() - NetworkSelectedCount - UnavailableCount,
            UnavailableCount));
    return UnavailableCount == 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Synchronize_RouteWatchesFromActor()
    -> void
{
    Invalidate_RouteWatchPreviews();

    const auto RouteCount = Get_RouteWatchCount();
    if (RouteCount <= 0)
    {
        _ActiveRouteWatchIndex = INDEX_NONE;
        _RoutePreviewStart = FVector::ZeroVector;
        _RoutePreviewGoal = FVector::ZeroVector;
        Clear_RoutePreview();
        Set_Status(
            ECk_PathNetworkDesigner_Status::Ready,
            TEXT("Route-watch changes synchronized. No saved routes remain."));
        return;
    }

    const auto SynchronizedIndex = FMath::Clamp(
        _ActiveRouteWatchIndex == INDEX_NONE
            ? 0
            : _ActiveRouteWatchIndex,
        0,
        RouteCount - 1);
    if (NOT Load_RouteWatch(SynchronizedIndex, false))
    {
        _ActiveRouteWatchIndex = INDEX_NONE;
        _RoutePreviewStart = FVector::ZeroVector;
        _RoutePreviewGoal = FVector::ZeroVector;
        Clear_RoutePreview();
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Route-watch changes could not be synchronized from the level actor."));
        return;
    }

    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        TEXT("Route-watch changes synchronized from the level actor."));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Apply_ToLevel()
    -> bool
{
    auto* TargetLevel = _TargetLevel.Get();
    if (ck::Is_NOT_Valid(TargetLevel))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Choose a valid target level before applying."));
        return false;
    }

    const auto Applied = ck::pathnetwork_editor::authoring::ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::FApplyToLevelRequest{
            ._TargetLevel = TargetLevel,
            ._ExplicitTargetActor = _ExplicitTargetActor.Get(),
            ._DetectorTemplate = _DetectorTemplate,
            ._DetectionBounds = Get_DetectionBounds(),
            ._VectorizeParams = _VectorizeParams,
            ._BuildParams = _BuildParams,
            ._AutoDetectOnBeginPlay = _AutoDetectOnBeginPlay,
            ._UseRecommendedFollowerTuning =
                _UseRecommendedFollowerTuning,
            ._RecommendedFollowerTuning =
                _RecommendedFollowerTuning,
            ._Preview = &_Preview});
    if (NOT Applied._Succeeded)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            Applied._FailureReason);
        return false;
    }

    _ExplicitTargetActor = Applied._Actor;
    _VisualizedActor = Applied._Actor;
    if (_ActiveRouteWatchIndex == INDEX_NONE
        && Get_RouteWatchCount() > 0)
    { Load_RouteWatch(0, false); }
    if (auto* Actor = Applied._Actor.Get();
        ck::IsValid(Actor)
        && ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
    {
        GEditor->SelectNone(false, true, false);
        GEditor->SelectActor(Actor, true, true, true);
    }

    Set_Status(
        ECk_PathNetworkDesigner_Status::Applied,
        FString::Printf(
            TEXT("%s %d generated ribbons with %d authored ribbons preserved. Undo is available."),
            Applied._CreatedActor ? TEXT("Created path network and applied") : TEXT("Updated path network with"),
            Applied._GeneratedRibbonCount,
            Applied._AuthoredRibbonCount));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Clear_Preview()
    -> void
{
    _Preview = {};
    _TopologyAnalysis = {};
    _HasTopologyAnalysis = false;
    _MaskDrawPoints.Reset();
    Clear_RoutePreview();
    Invalidate_RouteWatchPreviews();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Clear_RoutePreview()
    -> void
{
    _RoutePreview = {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Invalidate_RouteWatchPreviews()
    -> void
{
    _RouteWatchPreviews.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Notify_ConfigurationEdited()
    -> void
{
    Clear_Preview();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        TEXT("Configuration changed. Run Preview to inspect it before Apply."));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Get_DetectionBounds() const
    -> FBox
{
    return FBox{
        _DetectionCenter - _DetectionExtents,
        _DetectionCenter + _DetectionExtents};
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_PathNetworkDesigner_Session_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    const auto PropertyName = InPropertyChangedEvent.GetPropertyName();
    const auto MemberPropertyName =
        InPropertyChangedEvent.GetMemberPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE, _DetectorClass))
    {
        Set_DetectorClass(_DetectorClass.Get());
        return;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE, _MaxMaskCellsToDraw))
    {
        Rebuild_MaskDrawPoints();
        return;
    }

    const bool IsRoutePreviewConfiguration =
        MemberPropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE,
            _RecommendedFollowerTuning)
        || PropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE,
            _UseRecommendedFollowerTuning)
        || PropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE,
            _RoutePreviewStart)
        || PropertyName == GET_MEMBER_NAME_CHECKED(
            UCk_PathNetworkDesigner_Session_UE,
            _RoutePreviewGoal);
    if (IsRoutePreviewConfiguration)
    {
        Clear_RoutePreview();
        Invalidate_RouteWatchPreviews();
        Set_Status(
            ECk_PathNetworkDesigner_Status::Ready,
            _Preview._Succeeded
                ? TEXT("Route preferences changed. Refresh Preview to update available component transfers, then refresh the representative route.")
                : TEXT("Route preferences changed. Preview the network before previewing a route."));
        return;
    }

    const bool IsOverlayOnly =
        PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawBounds)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawMask)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawPreviewRibbons)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawAuthoredRibbons)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawGeneratedRibbons)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _DrawRoutePreview)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UCk_PathNetworkDesigner_Session_UE, _MaxMaskCellsToDraw);
    if (NOT IsOverlayOnly)
    { Notify_ConfigurationEdited(); }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Capture_RouteEndpointFromSelection(
        const bool InCaptureStart)
    -> bool
{
    auto* World = _World.Get();
    if (ck::Is_NOT_Valid(World)
        || ck::Is_NOT_Valid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("An active editor world and actor selection are required."));
        return false;
    }

    auto* SelectedActor = static_cast<AActor*>(nullptr);
    auto* Selection = GEditor->GetSelectedActors();
    if (Selection != nullptr)
    {
        for (FSelectionIterator It{*Selection}; It; ++It)
        {
            auto* Candidate = Cast<AActor>(*It);
            if (ck::Is_NOT_Valid(Candidate)
                || Candidate->GetWorld() != World)
            { continue; }

            if (SelectedActor != nullptr)
            {
                Set_Status(
                    ECk_PathNetworkDesigner_Status::Error,
                    TEXT("Select exactly one actor to capture a route endpoint."));
                return false;
            }
            SelectedActor = Candidate;
        }
    }

    if (ck::Is_NOT_Valid(SelectedActor))
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Select exactly one actor to capture a route endpoint."));
        return false;
    }

    if (InCaptureStart)
    { _RoutePreviewStart = SelectedActor->GetActorLocation(); }
    else
    { _RoutePreviewGoal = SelectedActor->GetActorLocation(); }

    Clear_RoutePreview();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        FString::Printf(
            TEXT("Captured route %s from %s. Capture the other endpoint, then preview the route."),
            InCaptureStart ? TEXT("start") : TEXT("goal"),
            *SelectedActor->GetActorLabel()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Set_DetectorClass(
        UClass* InDetectorClass)
    -> bool
{
    if (NOT ck::pathnetwork_editor::authoring::Is_UsableDetectorClass(InDetectorClass))
    {
        _DetectorTemplate = nullptr;
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Choose a concrete, non-deprecated path-network detector class."));
        return false;
    }

    _DetectorClass = InDetectorClass;
    _DetectorTemplate = NewObject<UCk_PathNetwork_Detector_UE>(
        this, InDetectorClass, NAME_None, RF_Transient);
    const bool TemplateIsValid = ck::IsValid(_DetectorTemplate);
    CK_ENSURE_IF_NOT(TemplateIsValid,
        TEXT("Could not create path-network designer detector template [{}]"),
        InDetectorClass)
    {}
    if (NOT TemplateIsValid)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            TEXT("Could not create the selected detector template."));
        return false;
    }

    _ActivePresetOwner = NAME_None;
    _ActivePresetId = NAME_None;
    Clear_Preview();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Rebuild_MaskDrawPoints()
    -> void
{
    _MaskDrawPoints.Reset();
    if (NOT _Preview._Succeeded
        || NOT _Preview._Mask.Get_IsValidMask()
        || _Preview._OccupiedCellCount <= 0
        || _MaxMaskCellsToDraw <= 0)
    { return; }

    const auto Stride = FMath::Max(
        1,
        FMath::CeilToInt(
            static_cast<double>(_Preview._OccupiedCellCount)
            / static_cast<double>(_MaxMaskCellsToDraw)));
    _MaskDrawPoints.Reserve(FMath::Min(
        _Preview._OccupiedCellCount,
        _MaxMaskCellsToDraw));

    auto OccupiedIndex = 0;
    for (auto Y = 0; Y < _Preview._Mask.Get_SizeY(); ++Y)
    {
        for (auto X = 0; X < _Preview._Mask.Get_SizeX(); ++X)
        {
            if (NOT _Preview._Mask.Get_IsOccupied(X, Y))
            { continue; }

            if (OccupiedIndex % Stride == 0)
            {
                _MaskDrawPoints.Add(
                    _Preview._Mask.Get_CellWorldLocation(X, Y)
                    + FVector{0.0, 0.0, 4.0});
            }
            ++OccupiedIndex;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Get_ProspectivePreviewRibbons() const
    -> TArray<FCk_PathNetwork_Ribbon>
{
    auto Ribbons = TArray<FCk_PathNetwork_Ribbon>{};
    auto* SourceActor = _ExplicitTargetActor.Get();
    if (ck::Is_NOT_Valid(SourceActor))
    {
        auto CandidateActors = TArray<ACk_PathNetwork_UE*>{};
        if (const auto* TargetLevel = _TargetLevel.Get(); ck::IsValid(TargetLevel))
        {
            for (const auto& Actor : TargetLevel->Actors)
            {
                if (auto* Candidate = Cast<ACk_PathNetwork_UE>(Actor.Get());
                    ck::IsValid(Candidate))
                { CandidateActors.Add(Candidate); }
            }
        }
        if (CandidateActors.Num() == 1)
        { SourceActor = CandidateActors[0]; }
    }
    if (ck::IsValid(SourceActor) && SourceActor->GetWorld() == _World.Get())
    {
        for (const auto& Ribbon : SourceActor->Get_WorldRibbons())
        {
            if (Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored)
            { Ribbons.Add(Ribbon); }
        }
    }
    Ribbons.Append(_Preview._GeneratedWorldRibbons);
    return Ribbons;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Set_Status(
        const ECk_PathNetworkDesigner_Status InStatus,
        FString InMessage)
    -> void
{
    _Status = InStatus;
    _StatusMessage = MoveTemp(InMessage);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_Session_UE::
    Fit_Bounds(
        const FBox& InBounds,
        const FString& InSourceLabel)
    -> bool
{
    const bool BoundsAreValid =
        InBounds.IsValid
        && NOT InBounds.Min.ContainsNaN()
        && NOT InBounds.Max.ContainsNaN();
    if (NOT BoundsAreValid)
    {
        Set_Status(
            ECk_PathNetworkDesigner_Status::Error,
            FString::Printf(TEXT("Could not derive finite bounds from %s."), *InSourceLabel));
        return false;
    }

    _DetectionCenter = InBounds.GetCenter();
    const auto Extents = InBounds.GetExtent();
    _DetectionExtents = FVector{
        FMath::Max(Extents.X, 100.0),
        FMath::Max(Extents.Y, 100.0),
        FMath::Max(Extents.Z, 100.0)};
    Clear_Preview();
    Set_Status(
        ECk_PathNetworkDesigner_Status::Ready,
        FString::Printf(
            TEXT("Detection bounds fitted to %s. Preview to verify source coverage."),
            *InSourceLabel));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
