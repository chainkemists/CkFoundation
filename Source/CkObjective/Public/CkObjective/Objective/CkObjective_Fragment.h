#pragma once

#include "CkObjective_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Objective_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKOBJECTIVE_API FFragment_Objective_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Objective_Params);

    private:
        FCk_Objective_ParamsData _Data;

    public:
        CK_PROPERTY_GET(_Data);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Objective_Params, _Data);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOBJECTIVE_API FFragment_Objective_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Objective_Current);

    public:
        friend class FProcessor_Objective_Setup;
        friend class FProcessor_Objective_HandleRequests;
        friend class FProcessor_Objective_Teardown;
        friend class UCk_Utils_Objective_UE;

    private:
        ECk_ObjectiveStatus _Status = ECk_ObjectiveStatus::NotStarted;
        int32 _Progress = 0;
        FGameplayTag _CompletionTag;
        FGameplayTag _FailureTag;

    public:
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_Progress);
        CK_PROPERTY_GET(_CompletionTag);
        CK_PROPERTY_GET(_FailureTag);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOBJECTIVE_API FFragment_Objective_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Objective_Requests);

    public:
        friend class FProcessor_Objective_HandleRequests;
        friend class UCk_Utils_Objective_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_Objective_Start,
            FCk_Request_Objective_Complete,
            FCk_Request_Objective_Fail,
            FCk_Request_Objective_UpdateProgress,
            FCk_Request_Objective_AddProgress
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjective_StatusChanged,
        FCk_Delegate_Objective_StatusChanged_MC,
        FCk_Handle_Objective,
        ECk_ObjectiveStatus);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjective_ProgressChanged,
        FCk_Delegate_Objective_ProgressChanged_MC,
        FCk_Handle_Objective,
        FGameplayTag,
        int32,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjective_Completed,
        FCk_Delegate_Objective_Completed_MC,
        FCk_Handle_Objective,
        FGameplayTag);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOBJECTIVE_API,
        OnObjective_Failed,
        FCk_Delegate_Objective_Failed_MC,
        FCk_Handle_Objective,
        FGameplayTag);
}

// --------------------------------------------------------------------------------------------------------------------