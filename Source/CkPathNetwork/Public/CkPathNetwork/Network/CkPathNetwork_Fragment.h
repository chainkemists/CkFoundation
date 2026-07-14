#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_PathNetwork_Params = FCk_Fragment_PathNetwork_ParamsData;
    using FFragment_PathNetworkFollower_Params = FCk_Fragment_PathNetworkFollower_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Derived graph data on the network entity. _Epoch bumps on every (re)build; follower
    // corridors record the epoch they were planned against and replan when it moves on.
    struct CKPATHNETWORK_API FFragment_PathNetwork_Graph
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetwork_Graph);

        friend class FProcessor_PathNetwork_Setup;
        friend class FProcessor_PathNetwork_HandleRequests;
        friend class FProcessor_PathNetworkFollower_HandleRequests;
        friend class ::UCk_Utils_PathNetwork_UE;

    private:
        pathnetwork::FBuiltNetwork _Network;
        int32 _Epoch = 0;

    public:
        CK_PROPERTY_GET(_Network);
        CK_PROPERTY_GET(_Epoch);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FFragment_PathNetwork_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetwork_Requests);

        friend class FProcessor_PathNetwork_HandleRequests;
        friend class ::UCk_Utils_PathNetwork_UE;

    public:
        using RebuildRequestType = FCk_Request_PathNetwork_Rebuild;
        using RequestType = std::variant<RebuildRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FFragment_PathNetworkFollower_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetworkFollower_Requests);

        friend class FProcessor_PathNetworkFollower_HandleRequests;
        friend class FProcessor_PathNetworkFollower_InvalidateOnRebuild;
        friend class ::UCk_Utils_PathNetworkFollower_UE;

    public:
        using FindRouteRequestType = FCk_Request_PathNetworkFollower_FindRoute;
        using RequestType = std::variant<FindRouteRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-follower routing state: the last result plus what it was planned against (network +
    // epoch), so the invalidation processor can detect staleness without touching the graph.
    struct CKPATHNETWORK_API FFragment_PathNetworkFollower_Corridor
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetworkFollower_Corridor);

        friend class FProcessor_PathNetworkFollower_HandleRequests;
        friend class FProcessor_PathNetworkFollower_InvalidateOnRebuild;
        friend class ::UCk_Utils_PathNetworkFollower_UE;

    private:
        FCk_PathNetwork_RouteResult _Result;
        FCk_Handle_PathNetwork _Network;
        int32 _NetworkEpoch = 0;

    public:
        CK_PROPERTY_GET(_Result);
        CK_PROPERTY_GET(_Network);
        CK_PROPERTY_GET(_NetworkEpoch);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // One-shot build gate: stamped by UCk_Utils_PathNetwork_UE::Add, consumed by
    // FProcessor_PathNetwork_Setup on the next tick.
    CK_DEFINE_ECS_TAG(FTag_PathNetwork_NeedsBuild);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKPATHNETWORK_API,
        PathNetworkFollower_OnRouteReady,
        FCk_Delegate_PathNetworkFollower_OnRouteReady,
        FCk_Handle_PathNetworkFollower,
        FCk_PathNetwork_RouteResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKPATHNETWORK_API,
        PathNetworkFollower_OnRouteFailed,
        FCk_Delegate_PathNetworkFollower_OnRouteFailed,
        FCk_Handle_PathNetworkFollower);
}

// --------------------------------------------------------------------------------------------------------------------
