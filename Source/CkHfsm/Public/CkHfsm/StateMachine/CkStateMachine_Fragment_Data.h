#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkStateMachine_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKHFSM_API FCk_Handle_StateMachine : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_StateMachine); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_StateMachine);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Fragment_StateMachine_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_StateMachine_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _StartingState;

public:
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_StateMachine_ParamsData, _StartingState);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_StateMachine_Transition,
    FCk_Handle_StateMachine, InStateMachineHandle,
    FCk_Handle_State, InPreviousState,
    FCk_Handle_State, InCurrentState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_StateMachine_Transition_MC,
    FCk_Handle_StateMachine, InStateMachineHandle,
    FCk_Handle_State, InPreviousState,
    FCk_Handle_State, InCurrentState);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_StateMachine,
    FCk_Handle_StateMachine, InStateMachineHandle,
    FCk_Time, InDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_StateMachine_MC,
    FCk_Handle_StateMachine, InStateMachineHandle,
    FCk_Time, InDeltaT);

// --------------------------------------------------------------------------------------------------------------------