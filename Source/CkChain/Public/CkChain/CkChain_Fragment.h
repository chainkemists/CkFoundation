#pragma once

#include "CkChain/CkChain_Fragment_Data.h"
#include "CkChain/CkChain_PathHistory.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Chain_UE;
class UCk_Utils_ChainLink_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Chain_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Chain_Disabled);
    CK_DEFINE_ECS_TAG(FTag_Chain_RosterDirty);

    // --------------------------------------------------------------------------------------------------------------------

    // Wholesale alias (CkTween precedent): six of the eight fields are read by Update every frame.
    // _ChainName lives in the GameplayLabel and _StartingState in FTag_Chain_Disabled after Add; nothing
    // reads those two from here.
    using FFragment_Chain_Params = FCk_Chain_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAIN_API FFragment_Chain
    {
    public:
        CK_GENERATED_BODY(FFragment_Chain);

    public:
        friend class FProcessor_Chain_Setup;
        friend class FProcessor_Chain_HandleRequests;
        friend class FProcessor_Chain_Update;
        friend class FProcessor_Chain_EndPlay;
        friend class FProcessor_ChainLink_EndPlay;
        friend class UCk_Utils_Chain_UE;
        friend class UCk_Utils_ChainLink_UE;

    private:
        FCk_Handle_Transform _Head;
        TArray<FCk_Handle_ChainLink> _Links;
        chain::FPathHistory _History;
        FTransform _LastHeadTransform = FTransform::Identity;
        FVector _UpVectorNormalized = FVector::UpVector;

    public:
        CK_PROPERTY_GET(_Head);
        CK_PROPERTY_GET(_Links);
        CK_PROPERTY_GET(_History);
        CK_PROPERTY_GET(_LastHeadTransform);
        CK_PROPERTY_GET(_UpVectorNormalized);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Chain, _Head);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAIN_API FFragment_Chain_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Chain_Requests);

        friend class FProcessor_Chain_HandleRequests;
        friend class FProcessor_Chain_CancelPendingRequests;
        friend class UCk_Utils_Chain_UE;

        using RequestType = std::variant<
            FCk_Request_Chain_AttachLink, FCk_Request_Chain_DetachLink, FCk_Request_Chain_SetLinkDistance,
            FCk_Request_Chain_Split, FCk_Request_Chain_ReseedHistory, FCk_Request_Chain_EnableDisable>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Immutable residue of FCk_ChainLink_Spec. The distance is request-mutable (SetLinkDistance, Split) and
    // therefore lives in FFragment_ChainLink, never here.
    struct CKCHAIN_API FFragment_ChainLink_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ChainLink_Params);

    private:
        FTransform _LocalOffset = FTransform::Identity;
        ECk_Chain_LinkOrientation _Orientation = ECk_Chain_LinkOrientation::FollowPath;

    public:
        CK_PROPERTY_GET(_LocalOffset);
        CK_PROPERTY_GET(_Orientation);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ChainLink_Params, _LocalOffset, _Orientation);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAIN_API FFragment_ChainLink
    {
    public:
        CK_GENERATED_BODY(FFragment_ChainLink);

    public:
        friend class FProcessor_Chain_HandleRequests;
        friend class FProcessor_Chain_Update;
        friend class FProcessor_Chain_EndPlay;
        friend class FProcessor_ChainLink_EndPlay;
        friend class UCk_Utils_Chain_UE;
        friend class UCk_Utils_ChainLink_UE;

    private:
        FCk_Handle_Chain _Chain;
        float _DistanceFromHeadCm = 0.0f;

    public:
        CK_PROPERTY_GET(_Chain);
        CK_PROPERTY_GET(_DistanceFromHeadCm);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ChainLink, _Chain, _DistanceFromHeadCm);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The pose the link was last driven to. Present from the link's first publish; removed when the link is detached,
    // reassigned by a split, or its chain is destroyed (FFragment_Transform_Previous shape). Absence means "not yet driven".
    struct CKCHAIN_API FFragment_ChainLink_TargetPose
    {
    public:
        CK_GENERATED_BODY(FFragment_ChainLink_TargetPose);

    public:
        friend class FProcessor_Chain_HandleRequests;
        friend class FProcessor_Chain_Update;
        friend class UCk_Utils_ChainLink_UE;

    private:
        FTransform _Pose = FTransform::Identity;

    public:
        CK_PROPERTY_GET(_Pose);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ChainLink_TargetPose, _Pose);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfChains, FCk_Handle_Chain);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCHAIN_API, OnChainLinkAttached, FCk_Delegate_Chain_OnLinkAttached, FCk_Handle_Chain, FCk_Chain_Payload_LinkAttached);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCHAIN_API, OnChainLinkDetached, FCk_Delegate_Chain_OnLinkDetached, FCk_Handle_Chain, FCk_Chain_Payload_LinkDetached);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCHAIN_API, OnChainSplit, FCk_Delegate_Chain_OnSplit, FCk_Handle_Chain, FCk_Chain_Payload_Split);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCHAIN_API, OnChainHeadTeleported, FCk_Delegate_Chain_OnHeadTeleported, FCk_Handle_Chain, FCk_Chain_Payload_HeadTeleported);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Chain_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
