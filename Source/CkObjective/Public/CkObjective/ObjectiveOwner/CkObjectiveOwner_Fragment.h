#pragma once

#include "CkObjectiveOwner_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ObjectiveOwner_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ObjectiveOwner_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ObjectiveOwner_Params = FCk_ObjectiveOwner_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOBJECTIVE_API FFragment_ObjectiveOwner_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ObjectiveOwner_Requests);

    public:
        friend class FProcessor_ObjectiveOwner_HandleRequests;
        friend class UCk_Utils_ObjectiveOwner_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_ObjectiveOwner_AddObjective,
            FCk_Request_ObjectiveOwner_RemoveObjective
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfObjectives, FCk_Handle_Objective);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjectiveOwner_ObjectiveAdded,
        FCk_Delegate_ObjectiveOwner_ObjectiveAdded_MC,
        FCk_Handle_ObjectiveOwner,
        FCk_Handle_Objective);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjectiveOwner_ObjectiveRemoved,
        FCk_Delegate_ObjectiveOwner_ObjectiveRemoved_MC,
        FCk_Handle_ObjectiveOwner,
        FCk_Handle_Objective);
}

// --------------------------------------------------------------------------------------------------------------------