#pragma once

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkTargetable_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Targetable_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Targetable_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targetable_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Targetable_Params);

    public:
        using ParamsType = FCk_Fragment_Targetable_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Targetable_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targetable_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Targetable_Current);

    public:
        friend class FProcessor_Targetable_Setup;
        friend class FProcessor_Targetable_HandleRequests;
        friend class UCk_Utils_Targetable_UE;

    private:
        TWeakObjectPtr<USceneComponent> _AttachmentNode;
        ECk_EnableDisable _EnableDisable;

    public:
        CK_PROPERTY_GET(_AttachmentNode);
        CK_PROPERTY_GET(_EnableDisable);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Targetable_Current, _EnableDisable);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targetable_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Targetable_Requests);

    public:
        friend class FProcessor_Targetable_HandleRequests;
        friend class UCk_Utils_Targetable_UE;

    public:
        using EnableDisableRequestType = FCk_Request_Targetable_EnableDisable;

        using RequestType = std::variant<EnableDisableRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfTargetables, FCk_Handle_Targetable);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTARGETING_API,
        OnTargetableEnableDisable,
        FCk_Delegate_Targetable_OnEnableDisable_MC,
        FCk_Targetable_BasicInfo,
        ECk_EnableDisable);
}

// --------------------------------------------------------------------------------------------------------------------
