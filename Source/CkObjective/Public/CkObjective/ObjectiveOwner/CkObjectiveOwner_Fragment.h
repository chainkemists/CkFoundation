#pragma once

#include "CkObjectiveOwner_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEntityCollection/CkEntityCollection_Fragment_Data.h"

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

    struct CKOBJECTIVE_API FFragment_ObjectiveOwner_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ObjectiveOwner_Current);

    public:
        friend class FProcessor_ObjectiveOwner_Setup;
        friend class FProcessor_ObjectiveOwner_HandleRequests;
        friend class UCk_Utils_ObjectiveOwner_UE;

    private:
        FCk_Handle_EntityCollection _ObjectivesEntityCollection;

    public:
        CK_PROPERTY_GET(_ObjectivesEntityCollection);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ObjectiveOwner_Current, _ObjectivesEntityCollection);
    };

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

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjectiveOwner_ObjectiveStatusChanged,
        FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged_MC,
        FCk_Handle_ObjectiveOwner,
        FCk_Handle_Objective,
        ECk_ObjectiveStatus);
}

// --------------------------------------------------------------------------------------------------------------------