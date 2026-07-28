#include "CkPathNetwork_Utils.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Vectorize.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

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
    Request_Rebuild(
        FCk_Handle_PathNetwork& InNetwork,
        const FCk_Request_PathNetwork_Rebuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PathNetwork
{
    const auto NetworkIsValid = ck::IsValid(InNetwork);
    CK_ENSURE_IF_NOT(NetworkIsValid,
        TEXT("Invalid PathNetwork handle [{}] passed to Request_Rebuild"), InNetwork)
    {}
    if (NOT NetworkIsValid)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InNetwork);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Rebuild on PathNetwork [{}] dropped — caller does not have authority. "
             "The network is server-only."), InNetwork)
    {}
    if (NOT HasAuthority)
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
    {}
    if (NOT NetworkIsValid)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto DetectorIsValid = ck::IsValid(InDetector);
    CK_ENSURE_IF_NOT(DetectorIsValid,
        TEXT("Invalid detector passed to Request_RebuildFromDetector on PathNetwork [{}]"), InNetwork)
    {}
    if (NOT DetectorIsValid)
    {
        InDelegate.ExecuteIfBound(InNetwork, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InNetwork;
    }

    const auto Mask = InDetector->Get_DetectionMask(InWorldBounds);

    auto NewRibbons = ck::algo::Filter(
        InNetwork.Get<ck::FFragment_PathNetwork_Params>().Get_Ribbons(),
        [](const FCk_PathNetwork_Ribbon& InRibbon)
        { return InRibbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored; });

    if (Mask.Get_IsValidMask())
    { NewRibbons.Append(ck::pathnetwork::Vectorize_MaskToRibbons(Mask, InVectorizeParams)); }
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
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid handle [{}] passed to UCk_Utils_PathNetworkFollower_UE::Add"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Entity [{}] already has the PathNetworkFollower feature"), InHandle)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_PathNetworkFollower_Params>(InParams);
    InHandle.Add<ck::FFragment_PathNetworkFollower_Corridor>();

    ck::pathnetwork::Verbose(TEXT("PathNetworkFollower added to [{}] (multiplier=[{}])"),
        InHandle, InParams.Get_OffPathCostMultiplier());

    return Cast(InHandle);
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
    {}
    if (NOT FollowerIsValid)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InFollower);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_FindRoute on PathNetworkFollower [{}] dropped — caller does not have authority. "
             "Routing is server-only."), InFollower)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InFollower, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InFollower;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InFollower.AddOrGet<ck::FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(InRequest);
    InFollower.Get<ck::FFragment_PathNetworkFollower_Corridor>()._Result._Status = ECk_PathNetwork_RouteStatus::Pending;

    return InFollower;
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
    {}
    if (NOT FollowerIsValid)
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
