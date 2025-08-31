#pragma once

#include "CkObjective_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CKOBJECTIVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ByteAttribute_Objective_Status);
CKOBJECTIVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FloatAttribute_Objective_Progress);

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Objective_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_Objective_Params = FCk_Objective_ParamsData;

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
        FCk_Handle_ByteAttribute _StatusAttribute;

        FGameplayTag _CompletionTag;
        FGameplayTag _FailureTag;

    public:
        CK_PROPERTY_GET(_StatusAttribute);
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
            FCk_Request_Objective_Fail>;
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