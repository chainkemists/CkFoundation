#include "CkPathNetwork_Utils.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Vectorize.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_utils
{
    auto
        Is_PathNetworkFollowerTuningValid(
            const FCk_PathNetworkFollower_Tuning& InTuning)
        -> bool
    {
        const auto Multiplier = InTuning.Get_OffPathCostMultiplier();
        const auto NearEndpointMultiplier = InTuning.Get_NearEndpointCostMultiplier();
        const auto NetworkGapMultiplier = InTuning.Get_NetworkGapCostMultiplier();
        const auto JoinMaxDistance = InTuning.Get_EndpointJoinMaxDistance();
        const auto TransferMaxDistance = InTuning.Get_ComponentTransferMaxDistance();
        const auto LocalShortcutMaxDistance =
            InTuning.Get_LocalNetworkShortcutMaxDistance();
        const auto DirectGraceDistance = InTuning.Get_DirectTripGraceDistance();
        const auto DirectMinimumSavings =
            InTuning.Get_DirectRouteMinimumSavingsFraction();
        const auto SideKeeping = InTuning.Get_SideKeepingFraction();
        const auto Spacing = InTuning.Get_CorridorWaypointSpacing();
        const auto Smoothing = InTuning.Get_CornerSmoothingDistance();
        const auto Clearance = InTuning.Get_DesiredNavmeshClearance();
        const auto ResolvedRibbonTolerance =
            InTuning.Get_NavmeshResolvedRibbonTolerance();
        return FMath::IsFinite(Multiplier) && Multiplier >= 1.0f
            && FMath::IsFinite(NearEndpointMultiplier)
            && (NearEndpointMultiplier == 0.0f || NearEndpointMultiplier >= 1.0f)
            && FMath::IsFinite(NetworkGapMultiplier)
            && (NetworkGapMultiplier == 0.0f || NetworkGapMultiplier >= 1.0f)
            && FMath::IsFinite(JoinMaxDistance) && JoinMaxDistance >= 0.0f
            && FMath::IsFinite(TransferMaxDistance) && TransferMaxDistance >= 0.0f
            && FMath::IsFinite(LocalShortcutMaxDistance)
            && LocalShortcutMaxDistance >= 0.0f
            && FMath::IsFinite(DirectGraceDistance) && DirectGraceDistance >= 0.0f
            && FMath::IsFinite(DirectMinimumSavings)
            && DirectMinimumSavings >= 0.0f
            && DirectMinimumSavings <= 1.0f
            && FMath::IsFinite(SideKeeping) && SideKeeping >= 0.0f && SideKeeping <= 0.9f
            && FMath::IsFinite(Spacing) && Spacing >= 50.0f
            && FMath::IsFinite(Smoothing) && Smoothing >= 0.0f && Smoothing <= 1000.0f
            && FMath::IsFinite(Clearance) && Clearance >= 0.0f && Clearance <= 1000.0f
            && FMath::IsFinite(ResolvedRibbonTolerance)
            && ResolvedRibbonTolerance >= 0.0f
            && ResolvedRibbonTolerance <= 100.0f;
    }

    auto
        Get_Tuning(
            const FCk_Fragment_PathNetworkFollower_ParamsData& InParams)
        -> FCk_PathNetworkFollower_Tuning
    {
        auto Tuning = FCk_PathNetworkFollower_Tuning{};
        Tuning.Set_OffPathCostMultiplier(InParams.Get_OffPathCostMultiplier());
        Tuning.Set_NearEndpointCostMultiplier(InParams.Get_NearEndpointCostMultiplier());
        Tuning.Set_NetworkGapCostMultiplier(InParams.Get_NetworkGapCostMultiplier());
        Tuning.Set_EndpointJoinMaxDistance(InParams.Get_EndpointJoinMaxDistance());
        Tuning.Set_ComponentTransferMaxDistance(InParams.Get_ComponentTransferMaxDistance());
        Tuning.Set_LocalNetworkShortcutMaxDistance(
            InParams.Get_LocalNetworkShortcutMaxDistance());
        Tuning.Set_DirectTripGraceDistance(InParams.Get_DirectTripGraceDistance());
        Tuning.Set_DirectRouteMinimumSavingsFraction(
            InParams.Get_DirectRouteMinimumSavingsFraction());
        Tuning.Set_SideKeepingFraction(InParams.Get_SideKeepingFraction());
        Tuning.Set_CorridorWaypointSpacing(InParams.Get_CorridorWaypointSpacing());
        Tuning.Set_CornerSmoothingDistance(InParams.Get_CornerSmoothingDistance());
        Tuning.Set_DesiredNavmeshClearance(InParams.Get_DesiredNavmeshClearance());
        Tuning.Set_NavmeshResolvedRibbonTolerance(
            InParams.Get_NavmeshResolvedRibbonTolerance());
        return Tuning;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_PathNetwork_ParamsData& InParams)
    -> FCk_Handle_PathNetwork
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle [{}] passed to UCk_Utils_PathNetwork_UE::Add"), InOwner)
    { return {}; }

    auto NewNetworkEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_PathNetwork>(InOwner);

    NewNetworkEntity.Add<ck::FFragment_PathNetwork_Params>(InParams);
    NewNetworkEntity.Add<ck::FFragment_PathNetwork_Graph>();
    NewNetworkEntity.Add<ck::FTag_PathNetwork_NeedsBuild>();

    ck::pathnetwork::Verbose(TEXT("PathNetwork added to [{}] -> [{}] ([{}] authored ribbons)"),
        InOwner, NewNetworkEntity, InParams.Get_Ribbons().Num());

    return NewNetworkEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_PathNetwork_Graph>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Get_AllRibbonsInWorld(
        FCk_Handle InAnyHandleInWorld)
    -> TArray<FCk_PathNetwork_Ribbon>
{
    if (ck::Is_NOT_Valid(InAnyHandleInWorld))
    { return {}; }

    auto Ribbons = TArray<FCk_PathNetwork_Ribbon>{};
    InAnyHandleInWorld.View<
        ck::FFragment_PathNetwork_Params,
        ck::FFragment_PathNetwork_Graph,
        CK_IGNORE_PENDING_KILL>().ForEach(
        [&Ribbons](
            FCk_Entity,
            const ck::FFragment_PathNetwork_Params& InParams,
            const ck::FFragment_PathNetwork_Graph&)
        {
            Ribbons.Append(InParams.Get_Ribbons());
        });
    return Ribbons;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Request_Rebuild(
        FCk_Handle_PathNetwork& InNetwork,
        const FCk_Request_PathNetwork_Rebuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetwork
{
    const auto NetworkIsValid = ck::IsValid(InNetwork);
    CK_ENSURE_IF_NOT(NetworkIsValid,
        TEXT("Invalid PathNetwork handle [{}] passed to Request_Rebuild"), InNetwork)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InNetwork);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Rebuild on PathNetwork [{}] dropped — caller does not have authority. "
             "The network is server-only."), InNetwork)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InNetwork.AddOrGet<ck::FFragment_PathNetwork_Requests>()._Requests.Emplace(InRequest);
    return InNetwork;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Request_RebuildFromDetector(
        FCk_Handle_PathNetwork& InNetwork,
        const UCk_PathNetwork_Detector_UE* InDetector,
        FBox InWorldBounds,
        const FCk_PathNetwork_VectorizeParams& InVectorizeParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetwork
{
    const auto NetworkIsValid = ck::IsValid(InNetwork);
    CK_ENSURE_IF_NOT(NetworkIsValid,
        TEXT("Invalid PathNetwork handle [{}] passed to Request_RebuildFromDetector"), InNetwork)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto DetectorIsValid = ck::IsValid(InDetector);
    CK_ENSURE_IF_NOT(DetectorIsValid,
        TEXT("Invalid detector passed to Request_RebuildFromDetector on PathNetwork [{}]"), InNetwork)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto BoundsValidation = InDetector->Validate_DetectionBounds(InWorldBounds);
    const auto DetectorAcceptsBounds = BoundsValidation.Get_Succeeded();
    CK_ENSURE_IF_NOT(DetectorAcceptsBounds,
        TEXT("Request_RebuildFromDetector on [{}] rejected detection bounds: [{}]"),
        InNetwork,
        BoundsValidation.Get_FailureReason())
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto Mask = InDetector->Get_DetectionMask(InWorldBounds);

    auto GeneratedRibbons = TArray<FCk_PathNetwork_Ribbon>{};
    if (Mask.Get_IsValidMask())
    {
        auto Vectorized = ck::pathnetwork::Try_VectorizeDetectorMaskToRibbons(
            *InDetector,
            InWorldBounds,
            Mask,
            InVectorizeParams);
        const auto GeneratedRibbonsWereVectorized = Vectorized._Succeeded;
        CK_ENSURE_IF_NOT(GeneratedRibbonsWereVectorized,
            TEXT("Request_RebuildFromDetector on [{}] could not vectorize detector output: [{}]"),
            InNetwork,
            Vectorized._FailureReason)
        { }
        if (NOT GeneratedRibbonsWereVectorized)
        {
            InDelegate.ExecuteIfBound(
                InNetwork,
                ECk_Request_OperationResult::Failed_NotEnqueued);
            return InNetwork;
        }
        GeneratedRibbons = MoveTemp(Vectorized._Ribbons);
    }

    const auto Processed =
        InDetector->Process_GeneratedRibbons_WithVectorizeParams(
            InWorldBounds,
            GeneratedRibbons,
            InVectorizeParams);
    const auto GeneratedRibbonsWereProcessed = Processed.Get_Succeeded();
    CK_ENSURE_IF_NOT(GeneratedRibbonsWereProcessed,
        TEXT("Request_RebuildFromDetector on [{}] could not process generated detector output: [{}]"),
        InNetwork, Processed.Get_FailureReason())
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }
    GeneratedRibbons = Processed.Get_GeneratedWorldRibbons();

    const auto ProcessedSourcesAreGenerated =
        ck::pathnetwork::Get_AreAllRibbonSourcesGenerated(
            GeneratedRibbons);
    CK_ENSURE_IF_NOT(ProcessedSourcesAreGenerated,
        TEXT("Request_RebuildFromDetector on [{}] rejected detector processing output containing a non-Generated ribbon"),
        InNetwork)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto Validation = InDetector->Validate_GeneratedRibbons(
        InWorldBounds, GeneratedRibbons);
    const auto GeneratedRibbonsAreValid = Validation.Get_Succeeded();
    CK_ENSURE_IF_NOT(GeneratedRibbonsAreValid,
        TEXT("Request_RebuildFromDetector on [{}] rejected generated detector output: [{}]"),
        InNetwork, Validation.Get_FailureReason())
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    auto NewRibbons = ck::algo::Filter(
        InNetwork.Get<ck::FFragment_PathNetwork_Params>().Get_Ribbons(),
        [](const FCk_PathNetwork_Ribbon& InRibbon)
        { return InRibbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored; });

    if (Mask.Get_IsValidMask())
    { NewRibbons.Append(MoveTemp(GeneratedRibbons)); }
    else
    {
        ck::pathnetwork::Verbose(TEXT("Request_RebuildFromDetector on [{}]: detector returned an empty mask — "
            "rebuilding with authored ribbons only"), InNetwork);
    }

    return Request_Rebuild(InNetwork, FCk_Request_PathNetwork_Rebuild{NewRibbons}, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Get_IsBuilt(
        const FCk_Handle_PathNetwork& InNetwork)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNetwork),
        TEXT("Invalid PathNetwork handle [{}] passed to Get_IsBuilt"), InNetwork)
    { return false; }

    const auto& Graph = InNetwork.Get<ck::FFragment_PathNetwork_Graph>();
    return Graph.Get_Epoch() > 0 && Graph.Get_Network()._Edges.Num() > 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Get_NumNodes(
        const FCk_Handle_PathNetwork& InNetwork)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNetwork),
        TEXT("Invalid PathNetwork handle [{}] passed to Get_NumNodes"), InNetwork)
    { return 0; }

    return InNetwork.Get<ck::FFragment_PathNetwork_Graph>().Get_Network()._Nodes.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Get_NumEdges(
        const FCk_Handle_PathNetwork& InNetwork)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNetwork),
        TEXT("Invalid PathNetwork handle [{}] passed to Get_NumEdges"), InNetwork)
    { return 0; }

    return InNetwork.Get<ck::FFragment_PathNetwork_Graph>().Get_Network()._Edges.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    Get_BuildEpoch(
        const FCk_Handle_PathNetwork& InNetwork)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNetwork),
        TEXT("Invalid PathNetwork handle [{}] passed to Get_BuildEpoch"), InNetwork)
    { return 0; }

    return InNetwork.Get<ck::FFragment_PathNetwork_Graph>().Get_Epoch();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    TryGet_RecommendedFollowerTuning(
        const FCk_Handle_PathNetwork& InNetwork,
        FCk_PathNetworkFollower_Tuning& OutTuning)
    -> bool
{
    using namespace ck_pathnetwork_utils;

    OutTuning = {};

    const bool NetworkIsValid =
        ck::IsValid(InNetwork)
        && InNetwork.Has<ck::FFragment_PathNetwork_Params>()
        && Has(InNetwork);
    CK_ENSURE_IF_NOT(NetworkIsValid,
        TEXT("Invalid PathNetwork handle [{}] passed to TryGet_RecommendedFollowerTuning"),
        InNetwork)
    { return false; }

    const auto& Params = InNetwork.Get<ck::FFragment_PathNetwork_Params>();
    if (Params.Get_UseRecommendedFollowerTuning() != ECk_EnableDisable::Enable)
    { return false; }

    const auto& Recommendation = Params.Get_RecommendedFollowerTuning();
    const bool RecommendationIsValid =
        Is_PathNetworkFollowerTuningValid(Recommendation);
    CK_ENSURE_IF_NOT(RecommendationIsValid,
        TEXT("PathNetwork [{}] contains an invalid recommended follower tuning profile"),
        InNetwork)
    { return false; }

    OutTuning = Recommendation;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_UE::
    TryGet_ClosestPointOnNetwork(
        const FCk_Handle_PathNetwork& InNetwork,
        FVector InLocation,
        float InSearchRadius,
        FVector& OutClosestPoint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNetwork),
        TEXT("Invalid PathNetwork handle [{}] passed to TryGet_ClosestPointOnNetwork"), InNetwork)
    { return false; }

    const auto& Network = InNetwork.Get<ck::FFragment_PathNetwork_Graph>().Get_Network();

    const auto NearbyEdges = Network.Query_EdgesNear(InLocation, InSearchRadius);

    auto BestDistance = InSearchRadius;
    auto Found = false;

    for (const auto EdgeId : NearbyEdges)
    {
        const auto Projection = Network.Project_OntoEdge(EdgeId, InLocation);

        if (Projection._Distance <= BestDistance)
        {
            BestDistance = Projection._Distance;
            OutClosestPoint = Projection._Location;
            Found = true;
        }
    }

    return Found;
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_Utils_PathNetworkFollower_UE
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams)
    -> FCk_Handle_PathNetworkFollower
{
    using namespace ck_pathnetwork_utils;

    const bool HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid handle [{}] passed to UCk_Utils_PathNetworkFollower_UE::Add"), InHandle)
    { return {}; }

    const bool FeatureIsAbsent = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(FeatureIsAbsent,
        TEXT("Entity [{}] already has the PathNetworkFollower feature"), InHandle)
    { return Cast(InHandle); }

    const auto Tuning = Get_Tuning(InParams);
    const bool TuningIsValid = Is_PathNetworkFollowerTuningValid(Tuning);
    CK_ENSURE_IF_NOT(TuningIsValid,
        TEXT("PathNetworkFollower parameters contain invalid tuning"))
    { return {}; }

    InHandle.Add<ck::FFragment_PathNetworkFollower_Params>(InParams);
    InHandle.Add<ck::FFragment_PathNetworkFollower_Corridor>();

    ck::pathnetwork::Verbose(TEXT("PathNetworkFollower added to [{}] (multiplier=[{}])"),
        InHandle, InParams.Get_OffPathCostMultiplier());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Try_AddOrAdoptByOwnerToken(
        FCk_Handle& InHandle,
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams,
        ECk_PathNetworkFollower_OwnershipResult& OutResult)
    -> FCk_Handle_PathNetworkFollower
{
    using namespace ck_pathnetwork_utils;

    OutResult = ECk_PathNetworkFollower_OwnershipResult::RejectedInvalidInput;

    const bool HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid handle [{}] passed to Try_AddOrAdoptByOwnerToken"), InHandle)
    { return {}; }

    const auto OwnerToken = InParams.Get_OwnerToken();
    const bool OwnerTokenIsValid = NOT OwnerToken.IsNone();
    CK_ENSURE_IF_NOT(OwnerTokenIsValid,
        TEXT("Try_AddOrAdoptByOwnerToken requires a non-empty owner token"))
    { return {}; }

    const auto Tuning = Get_Tuning(InParams);
    const bool TuningIsValid = Is_PathNetworkFollowerTuningValid(Tuning);
    CK_ENSURE_IF_NOT(TuningIsValid,
        TEXT("Try_AddOrAdoptByOwnerToken received invalid follower tuning"))
    { return {}; }

    if (Has(InHandle))
    {
        const auto ExistingFollower = Cast(InHandle);
        if (Get_OwnerToken(ExistingFollower) != OwnerToken)
        {
            OutResult = ECk_PathNetworkFollower_OwnershipResult::RejectedExistingOwner;
            return {};
        }

        OutResult = ECk_PathNetworkFollower_OwnershipResult::Adopted;
        ck::pathnetwork::Verbose(TEXT("PathNetworkFollower [{}] reacquired by owner token [{}]"),
            ExistingFollower, OwnerToken);
        return ExistingFollower;
    }

    auto Follower = Add(InHandle, InParams);
    if (ck::Is_NOT_Valid(Follower))
    { return {}; }

    OutResult = ECk_PathNetworkFollower_OwnershipResult::Added;
    return Follower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_PathNetworkFollower_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Get_OwnerToken(
        const FCk_Handle_PathNetworkFollower& InFollower)
    -> FName
{
    const bool FollowerIsValid = ck::IsValid(InFollower);
    CK_ENSURE_IF_NOT(FollowerIsValid,
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Get_OwnerToken"), InFollower)
    { return NAME_None; }

    return InFollower.Get<ck::FFragment_PathNetworkFollower_Params>().Get_OwnerToken();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Get_IsTuningValid(
        const FCk_PathNetworkFollower_Tuning& InTuning)
    -> bool
{
    return ck_pathnetwork_utils::Is_PathNetworkFollowerTuningValid(InTuning);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Remove(
        FCk_Handle_PathNetworkFollower& InFollower)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFollower),
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Remove"), InFollower)
    { return InFollower; }

    InFollower.Try_Remove<ck::FFragment_PathNetworkFollower_Params>();
    InFollower.Try_Remove<ck::FFragment_PathNetworkFollower_Corridor>();
    InFollower.Try_Remove<ck::FFragment_PathNetworkFollower_Requests>();

    ck::pathnetwork::Verbose(TEXT("PathNetworkFollower removed from [{}]"), InFollower);

    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Request_FindRoute(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Request_PathNetworkFollower_FindRoute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetworkFollower
{
    const auto FollowerIsValid = ck::IsValid(InFollower);
    CK_ENSURE_IF_NOT(FollowerIsValid,
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Request_FindRoute"), InFollower)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InFollower);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_FindRoute on PathNetworkFollower [{}] dropped — caller does not have authority. "
             "Routing is server-only."), InFollower)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& Params = InFollower.Get<ck::FFragment_PathNetworkFollower_Params>();
    auto Request = InRequest;
    Request.Set_TuningRevision(Params.Get_TuningRevision());
    InFollower.AddOrGet<ck::FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(Request);
    auto& Corridor = InFollower.Get<ck::FFragment_PathNetworkFollower_Corridor>();
    Corridor._Network = ck::IsValid(Request.Get_Network())
        ? Request.Get_Network()
        : Params.Get_Network();
    Corridor._NavQueryFilter = Request.Get_NavQueryFilter();
    auto& Result = Corridor._Result;
    Result._Status = ECk_PathNetwork_RouteStatus::Pending;
    Result._GoalLocation = Request.Get_GoalLocation();
    Result._TuningRevision = Params.Get_TuningRevision();
    Result._RequestRevision = Request.Get_RequestRevision();

    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Request_AbandonRoute(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Request_PathNetworkFollower_AbandonRoute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetworkFollower
{
    const auto FollowerIsValid = ck::IsValid(InFollower);
    CK_ENSURE_IF_NOT(FollowerIsValid,
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Request_AbandonRoute"), InFollower)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InFollower);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_AbandonRoute on PathNetworkFollower [{}] dropped — caller does not have "
             "authority. Routing is server-only."), InFollower)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    // Undrained requests owe their callers a completion, and one left in the fragment would drain
    // next tick and re-park the corridor this call just released.
    if (InFollower.Has<ck::FFragment_PathNetworkFollower_Requests>())
    {
        const auto Queued = InFollower.Get<ck::FFragment_PathNetworkFollower_Requests>();
        InFollower.Try_Remove<ck::FFragment_PathNetworkFollower_Requests>();
        ck::request::FireCancelledForPending(InFollower, Queued.Get_Requests());
    }

    if (InFollower.Has<ck::FFragment_PathNetworkFollower_Corridor>())
    {
        auto& Result = InFollower.Get<ck::FFragment_PathNetworkFollower_Corridor>()._Result;
        Result._Status = ECk_PathNetwork_RouteStatus::None;
        Result._RequestRevision = InRequest.Get_RequestRevision();
    }

    InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Succeeded);
    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Request_UpdateTuningAndReplan(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_PathNetworkFollower_Tuning& InTuning)
    -> FCk_Handle_PathNetworkFollower
{
    using namespace ck_pathnetwork_utils;

    const bool FollowerIsValid = ck::IsValid(InFollower);
    CK_ENSURE_IF_NOT(FollowerIsValid,
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Request_UpdateTuningAndReplan"), InFollower)
    { return InFollower; }

    const bool HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InFollower);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_UpdateTuningAndReplan on PathNetworkFollower [{}] dropped — caller does not have authority"), InFollower)
    { return InFollower; }

    const auto Multiplier = InTuning.Get_OffPathCostMultiplier();
    const auto NearEndpointMultiplier = InTuning.Get_NearEndpointCostMultiplier();
    const auto NetworkGapMultiplier = InTuning.Get_NetworkGapCostMultiplier();
    const auto JoinMaxDistance = InTuning.Get_EndpointJoinMaxDistance();
    const auto TransferMaxDistance = InTuning.Get_ComponentTransferMaxDistance();
    const auto LocalShortcutMaxDistance =
        InTuning.Get_LocalNetworkShortcutMaxDistance();
    const auto DirectGraceDistance = InTuning.Get_DirectTripGraceDistance();
    const auto DirectMinimumSavings =
        InTuning.Get_DirectRouteMinimumSavingsFraction();
    const auto SideKeeping = InTuning.Get_SideKeepingFraction();
    const auto Spacing = InTuning.Get_CorridorWaypointSpacing();
    const auto Smoothing = InTuning.Get_CornerSmoothingDistance();
    const auto Clearance = InTuning.Get_DesiredNavmeshClearance();
    const auto ResolvedRibbonTolerance =
        InTuning.Get_NavmeshResolvedRibbonTolerance();
    const bool TuningIsValid = Is_PathNetworkFollowerTuningValid(InTuning);
    CK_ENSURE_IF_NOT(TuningIsValid,
        TEXT("Request_UpdateTuningAndReplan received invalid tuning "
             "(far/direct multiplier [{}], near multiplier [{}], network gap multiplier [{}], join max [{}], transfer max [{}], local shortcut max [{}], direct grace [{}], minimum direct savings [{}], "
             "side [{}], spacing [{}], smoothing [{}], clearance [{}], resolved ribbon tolerance [{}])"),
        Multiplier, NearEndpointMultiplier, NetworkGapMultiplier, JoinMaxDistance, TransferMaxDistance,
        LocalShortcutMaxDistance, DirectGraceDistance, DirectMinimumSavings,
        SideKeeping, Spacing, Smoothing, Clearance, ResolvedRibbonTolerance)
    { return InFollower; }

    InFollower.AddOrGet<ck::FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(
        FCk_Request_PathNetworkFollower_UpdateTuning{InTuning});
    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Request_UpdateTuningAndReplanByOwnerToken(
        FCk_Handle InAnyHandleInWorld,
        FName InOwnerToken,
        const FCk_PathNetworkFollower_Tuning& InTuning)
    -> int32
{
    using namespace ck_pathnetwork_utils;

    const bool ContextIsValid = ck::IsValid(InAnyHandleInWorld);
    CK_ENSURE_IF_NOT(ContextIsValid,
        TEXT("Invalid world handle [{}] passed to Request_UpdateTuningAndReplanByOwnerToken"),
        InAnyHandleInWorld)
    { return 0; }

    const bool OwnerTokenIsValid = NOT InOwnerToken.IsNone();
    CK_ENSURE_IF_NOT(OwnerTokenIsValid,
        TEXT("Request_UpdateTuningAndReplanByOwnerToken requires a non-empty owner token"))
    { return 0; }

    const bool TuningIsValid = Is_PathNetworkFollowerTuningValid(InTuning);
    CK_ENSURE_IF_NOT(TuningIsValid,
        TEXT("Request_UpdateTuningAndReplanByOwnerToken received invalid tuning"))
    { return 0; }

    const bool HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAnyHandleInWorld);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_UpdateTuningAndReplanByOwnerToken dropped — caller does not have authority"))
    { return 0; }

    // Snapshot matching handles before adding request fragments. This keeps the
    // registry view read-only for the full iteration.
    TArray<FCk_Handle_PathNetworkFollower> MatchingFollowers;
    InAnyHandleInWorld.View<ck::FFragment_PathNetworkFollower_Params>().ForEach(
        [&](FCk_Entity InEntity, const ck::FFragment_PathNetworkFollower_Params& InParams)
    {
        if (InParams.Get_OwnerToken() != InOwnerToken)
        { return; }

        MatchingFollowers.Emplace(Cast(ck::MakeHandle(InEntity, InAnyHandleInWorld)));
    });

    auto UpdatedCount = int32{0};
    for (auto& Follower : MatchingFollowers)
    {
        const bool FollowerHasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(Follower);
        if (NOT FollowerHasAuthority)
        { continue; }

        Request_UpdateTuningAndReplan(Follower, InTuning);
        ++UpdatedCount;
    }
    return UpdatedCount;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Request_SetNetwork(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Handle_PathNetwork& InNetwork,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetworkFollower
{
    const auto FollowerIsValid = ck::IsValid(InFollower);
    CK_ENSURE_IF_NOT(FollowerIsValid,
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Request_SetNetwork"), InFollower)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    InFollower.Get<ck::FFragment_PathNetworkFollower_Params>().Set_Network(InNetwork);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Succeeded);

    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Get_RouteResult(
        const FCk_Handle_PathNetworkFollower& InFollower)
    -> FCk_PathNetwork_RouteResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFollower),
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Get_RouteResult"), InFollower)
    { return {}; }

    return InFollower.Get<ck::FFragment_PathNetworkFollower_Corridor>().Get_Result();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    Get_RouteStatus(
        const FCk_Handle_PathNetworkFollower& InFollower)
    -> ECk_PathNetwork_RouteStatus
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFollower),
        TEXT("Invalid PathNetworkFollower handle [{}] passed to Get_RouteStatus"), InFollower)
    { return ECk_PathNetwork_RouteStatus::None; }

    return InFollower.Get<ck::FFragment_PathNetworkFollower_Corridor>().Get_Result().Get_Status();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkFollower_UE::
    BindTo_OnRouteReady(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_PathNetworkFollower
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_PathNetworkFollower_OnRouteReady, InFollower, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InFollower;
}

auto
    UCk_Utils_PathNetworkFollower_UE::
    UnbindFrom_OnRouteReady(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteReady& InDelegate)
    -> FCk_Handle_PathNetworkFollower
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_PathNetworkFollower_OnRouteReady, InFollower, InDelegate);
    return InFollower;
}

auto
    UCk_Utils_PathNetworkFollower_UE::
    BindTo_OnRouteFailed(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_PathNetworkFollower
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_PathNetworkFollower_OnRouteFailed, InFollower, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InFollower;
}

auto
    UCk_Utils_PathNetworkFollower_UE::
    UnbindFrom_OnRouteFailed(
        FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteFailed& InDelegate)
    -> FCk_Handle_PathNetworkFollower
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_PathNetworkFollower_OnRouteFailed, InFollower, InDelegate);
    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------
