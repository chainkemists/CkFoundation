#pragma once

#include "CkEntityTag/Query/CkEntityTagQuery_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityTagQuery_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityTagQuery_HandleRequests;
    class FProcessor_EntityTagQuery_Evaluate;
    class FProcessor_EntityTagQuery_TrackedEntity_Destructor;
    class FProcessor_EntityTagQuery_Query_Destructor;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYTAG_API FFragment_EntityTagQuery_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTagQuery_Current);

        friend class FProcessor_EntityTagQuery_HandleRequests;
        friend class FProcessor_EntityTagQuery_Evaluate;
        friend class FProcessor_EntityTagQuery_Query_Destructor;
        friend class FProcessor_EntityTagQuery_TrackedEntity_Destructor;
        friend class ::UCk_Utils_EntityTagQuery_UE;

    private:
        TArray<FCk_EntityTagQuery_Requirement> _Requirements;
        TArray<TArray<FCk_Handle>>             _ResultsPerRequirement;
        TArray<TArray<FCk_Handle>>             _PendingAdded;      // parallel; cleared after each Evaluate pass
        TArray<TArray<FCk_Handle>>             _PendingRemoved;    // parallel; populated by Evaluate + destructor
        bool                                   _IsSatisfied  = false;
        bool                                   _HasFiredOnce = false;
        int32                                  _ContinuousUpdateListenerCount = 0;

    public:
        CK_PROPERTY_GET(_Requirements);
        CK_PROPERTY_GET(_ResultsPerRequirement);
        CK_PROPERTY_GET(_PendingAdded);
        CK_PROPERTY_GET(_PendingRemoved);
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

    // Placed on a tagged entity (the entity carrying CkEntityTag) when one or more queries
    // hold this entity in their _ResultsPerRequirement. The destruction processor walks this
    // list at the entity's EndPlay to proactively prune queries — no per-pass IsValid scan.
    struct CKENTITYTAG_API FFragment_EntityTagQuery_TrackedByQueries
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTagQuery_TrackedByQueries);

        friend class FProcessor_EntityTagQuery_Evaluate;
        friend class FProcessor_EntityTagQuery_TrackedEntity_Destructor;
        friend class FProcessor_EntityTagQuery_Query_Destructor;
        friend class ::UCk_Utils_EntityTagQuery_UE;

    private:
        TArray<FCk_Handle_EntityTagQuery> _Queries;

    public:
        CK_PROPERTY_GET(_Queries);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTagQuery_OnSatisfied,
        FCk_Delegate_EntityTagQuery_OnSatisfied,
        FCk_Handle_EntityTagQuery,
        TArray<FCk_EntityTagQuery_Result>);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTagQuery_OnContinuousUpdate,
        FCk_Delegate_EntityTagQuery_OnContinuousUpdate,
        FCk_Handle_EntityTagQuery,
        bool,
        TArray<FCk_EntityTagQuery_Result>);

    // --------------------------------------------------------------------------------------------------------------------
}
