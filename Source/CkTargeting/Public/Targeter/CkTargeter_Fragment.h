#pragma once

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkTargeter_Fragment_Data.h"
#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Targeter_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Targeter_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Targeter_NeedsUpdate);
    CK_DEFINE_ECS_TAG(FTag_Targeter_NeedsCleanup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targeter_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Targeter_Params);

    public:
        using ParamsType = FCk_Fragment_Targeter_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Targeter_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targeter_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Targeter_Current);

    public:
        friend class FProcessor_Targeter_HandleRequests;
        friend class FProcessor_Targeter_Cleanup;
        friend class FProcessor_Targeter_Setup;
        friend class UCk_Utils_Targeter_UE;

    public:
        using CachedTargetsType = FCk_Targeter_TargetList;

    private:
        TOptional<CachedTargetsType> _CachedTargets;
        ECk_Targeter_TargetingStatus _TargetingStatus = ECk_Targeter_TargetingStatus::NotTargeting;
        TStrongObjectPtr<UCk_Targeter_CustomTargetFilter_PDA> _InstancedTargetFilter;

    public:
        CK_PROPERTY_GET(_CachedTargets);
        CK_PROPERTY_GET(_TargetingStatus);
        CK_PROPERTY_GET(_InstancedTargetFilter);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targeter_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Targeter_Requests);

    public:
        friend class FProcessor_Targeter_HandleRequests;
        friend class UCk_Utils_Targeter_UE;

    public:
        using QueryValidTargetsRequestType = FCk_Request_Targeter_QueryTargets;

        using RequestType = std::variant<QueryValidTargetsRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfTargeters, FCk_Handle_Targeter);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTARGETING_API, Targeter_OnTargetsQueried,
        FCk_Delegate_Targeter_OnTargetsQueried_MC, FCk_Targeter_BasicInfo, FCk_Targeter_TargetList);
}

// --------------------------------------------------------------------------------------------------------------------
