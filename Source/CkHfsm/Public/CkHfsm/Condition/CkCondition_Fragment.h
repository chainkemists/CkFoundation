#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkCondition_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lifecycle tags
    CK_DEFINE_ECS_TAG(FTag_Condition_Setup);
    CK_DEFINE_ECS_TAG(FTag_Condition_Enter);
    CK_DEFINE_ECS_TAG(FTag_Condition_Exit);
    CK_DEFINE_ECS_TAG(FTag_Condition_EvaluationPaused);
    CK_DEFINE_ECS_TAG(FTag_Condition_EvaluationPassed);
    CK_DEFINE_ECS_TAG(FTag_Condition_EvaluationFailed);
    
    // Behavior tags
    CK_DEFINE_ECS_TAG(FTag_Condition_NegateResult);
    CK_DEFINE_ECS_TAG(FTag_Condition_IsEventDriven);
    CK_DEFINE_ECS_TAG(FTag_Condition_IsNotEventDriven);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Condition_Params = FCk_Fragment_Condition_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_Condition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Condition_Current);

    public:
        friend class FProcessor_Condition_Setup;
        friend class FProcessor_Condition_Enter;
        friend class FProcessor_Condition_Exit;
        friend class FProcessor_Condition_Evaluate;

    private:
        // Extensible for derived condition types
        int32 _ReservedForFutureUse = 0;

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Condition_Current);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_Condition_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Condition_Requests);

    public:
        friend class FProcessor_Condition_HandleRequests;
        friend class UCk_Utils_Condition_UE;

    public:
        using RequestType = std::variant<FCk_Request_Condition_Command, FCk_Request_Condition_MarkResult>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Signals
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnConditionEnter, FCk_Delegate_Condition_MC, 
        FCk_Handle_Condition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnConditionExit, FCk_Delegate_Condition_MC, 
        FCk_Handle_Condition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnConditionPassed, FCk_Delegate_Condition_MC, 
        FCk_Handle_Condition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnConditionFailed, FCk_Delegate_Condition_MC, 
        FCk_Handle_Condition, FCk_Time);
}

// --------------------------------------------------------------------------------------------------------------------