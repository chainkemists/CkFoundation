#pragma once

#include "CkEntityTag/Query/CkEntityTagQuery_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityTagQuery_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityTagQuery_HandleRequests;
    class FProcessor_EntityTagQuery_Evaluate;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYTAG_API FFragment_EntityTagQuery_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTagQuery_Current);

        friend class FProcessor_EntityTagQuery_HandleRequests;
        friend class FProcessor_EntityTagQuery_Evaluate;
        friend class ::UCk_Utils_EntityTagQuery_UE;

    private:
        TArray<FCk_EntityTagQuery_Requirement> _Requirements;
        TArray<TArray<FCk_Handle>>             _ResultsPerRequirement;
        bool                                   _IsSatisfied  = false;
        bool                                   _HasFiredOnce = false;

    public:
        CK_PROPERTY_GET(_Requirements);
        CK_PROPERTY_GET(_ResultsPerRequirement);
        CK_PROPERTY_GET(_IsSatisfied);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYTAG_API FFragment_EntityTagQuery_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTagQuery_Requests);

        friend class FProcessor_EntityTagQuery_HandleRequests;
        friend class ::UCk_Utils_EntityTagQuery_UE;

        using AddRequirementType    = FCk_Request_EntityTagQuery_AddRequirement;
        using RemoveRequirementType = FCk_Request_EntityTagQuery_RemoveRequirement;
        using RequestType           = std::variant<AddRequirementType, RemoveRequirementType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTagQuery_OnSatisfied,
        FCk_Delegate_EntityTagQuery_OnSatisfied,
        FCk_Handle_EntityTagQuery,
        TArray<FCk_EntityTagQuery_Result>);

    // --------------------------------------------------------------------------------------------------------------------
}
