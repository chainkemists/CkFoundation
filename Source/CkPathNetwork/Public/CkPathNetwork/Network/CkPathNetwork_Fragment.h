#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_PathNetworkFollower_Tunables = FCk_PathNetworkFollower_Spec;

    /** The retained immutable residue of FCk_PathNetwork_Spec. The Spec's `_Ribbons` are not here:
     *  Request_Rebuild replaces them, so they belong with the network they build -- see
     *  FFragment_PathNetwork_Graph. */
    struct CKPATHNETWORK_API FFragment_PathNetwork_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetwork_Params);

    private:
        FCk_PathNetwork_BuildParams _BuildParams;

        ECk_EnableDisable _UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;

        FCk_PathNetworkFollower_Tuning _RecommendedFollowerTuning;

    public:
        CK_PROPERTY_GET(_BuildParams);
        CK_PROPERTY_GET(_UseRecommendedFollowerTuning);
        CK_PROPERTY_GET(_RecommendedFollowerTuning);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_PathNetwork_Params, _BuildParams,
            _UseRecommendedFollowerTuning, _RecommendedFollowerTuning);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // _Epoch bumps on every (re)build; corridors planned against an older one replan.
    // _Ribbons is the source the rest of this fragment is derived from -- Rebuild replaces the two
    // together, and keeping them apart is what let a stale ribbon set outlive its network.
    struct CKPATHNETWORK_API FFragment_PathNetwork_Graph
    {
    public:
        CK_GENERATED_BODY(FFragment_PathNetwork_Graph);

        friend class FProcessor_PathNetwork_Setup;
        friend class FProcessor_PathNetwork_HandleRequests;
        friend class FProcessor_PathNetworkFollower_HandleRequests;
        friend class ::UCk_Utils_PathNetwork_UE;

    private:
        TArray<FCk_PathNetwork_Ribbon> _Ribbons;
        pathnetwork::FBuiltNetwork _Network;
        int32 _Epoch = 0;
        TMap<
            pathnetwork::FRouteGraphStaticDataKey,
            TSharedPtr<
                const pathnetwork::FRouteGraphStaticData>>
            _RouteGraphStaticDataByPolicy;

    public:
        CK_PROPERTY_GET(_Ribbons);
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
        using UpdateTuningRequestType = FCk_Request_PathNetworkFollower_UpdateTuning;
        using RequestType = std::variant<FindRouteRequestType, UpdateTuningRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The last result plus what it was planned against, so staleness is detectable without the graph.
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
        FGameplayTag _NavQueryFilter;
        FCk_Nav_QueryFilterOverlay _QueryFilterOverlay;
        // The radius the corridor was planned for; restored onto a replan request.
        float _AgentRadiusUu = 0.0f;
        int32 _NetworkEpoch = 0;

    public:
        CK_PROPERTY_GET(_Result);
        CK_PROPERTY_GET(_Network);
        CK_PROPERTY_GET(_NavQueryFilter);
        CK_PROPERTY_GET(_QueryFilterOverlay);
        CK_PROPERTY_GET(_AgentRadiusUu);
        CK_PROPERTY_GET(_NetworkEpoch);
    };

    // --------------------------------------------------------------------------------------------------------------------

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
