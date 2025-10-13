#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHfsm/State/CkState_Fragment.h"
#include "CkHfsm/Service/CkService_Fragment.h"
#include "CkHfsm/StateMachine/CkStateMachine_Fragment.h"
#include "CkHfsm/Transition/CkTransition_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkState_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_State_Setup;
    class FProcessor_State_Enter;
    class FProcessor_State_Exit;
    class FProcessor_State_Evaluate;
    class FProcessor_State_Update;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_State"))
class CKHFSM_API UCk_Utils_State_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_State_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_State);

public:
    friend class ck::FProcessor_State_Setup;
    friend class ck::FProcessor_State_Enter;
    friend class ck::FProcessor_State_Exit;
    friend class ck::FProcessor_State_Evaluate;
    friend class ck::FProcessor_State_Update;

public:
    struct RecordOfServices_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfServices> {};
    struct RecordOfTransitions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTransitions> {};
    struct RecordOfStates_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfStates> {};

public:
    // Feature Management
    static FCk_Handle_State
    Create(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Fragment_State_ParamsData& InParams);

    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    static FCk_Handle_State
    DoCast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    static FCk_Handle_State
    DoCastChecked(
        FCk_Handle InHandle);

    static FCk_Handle_State
    Get_InvalidHandle() { return {}; }

public:
    // Service Management (C++ only) - No params needed for base service
    static FCk_Handle_Service
    AddService(
        FCk_Handle_State& InStateHandle);

    static TArray<FCk_Handle_Service>
    GetAllServices(
        const FCk_Handle_State& InStateHandle);

    // Transition Management (C++ only)
    static FCk_Handle_Transition
    AddTransition(
        FCk_Handle_State& InStateHandle,
        const FCk_Fragment_Transition_ParamsData& InTransitionParams);

    static TArray<FCk_Handle_Transition>
    GetAllTransitions(
        const FCk_Handle_State& InStateHandle);

public:
    // Commands (C++ only)
    static FCk_Handle_State
    Request_Enter(
        FCk_Handle_State& InHandle);

    static FCk_Handle_State
    Request_Exit(
        FCk_Handle_State& InHandle);

    static FCk_Handle_State
    Request_Evaluate(
        FCk_Handle_State& InHandle);

public:
    // Query (C++ only)
    static FGameplayTag
    Get_Name(
        const FCk_Handle_State& InHandle);

    static bool
    Get_IsReadyToTransition(
        const FCk_Handle_State& InHandle);

    static FCk_Handle
    Get_NextState(
        const FCk_Handle_State& InHandle);

public:
    // Signals (C++ only)
    static FCk_Handle_State
    BindTo_OnEnter(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate);

    static FCk_Handle_State
    BindTo_OnExit(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate);

    static FCk_Handle_State
    BindTo_OnUpdate(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate);

    static FCk_Handle_State
    UnbindFrom_OnEnter(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate);

    static FCk_Handle_State
    UnbindFrom_OnExit(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate);

    static FCk_Handle_State
    UnbindFrom_OnUpdate(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------