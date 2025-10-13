#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHFSM/State/CkState_Fragment.h"
#include "CkHFSM/Service/CkService_Fragment_Data.h"
#include "CkHFSM/Transition/CkTransition_Fragment_Data.h"

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
    // Feature Management
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|HFSM|State",
        DisplayName="[Ck][State] Add Feature")
    static FCk_Handle_State
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_State_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|HFSM|State",
        DisplayName="[Ck][State] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|HFSM|State",
        DisplayName="[Ck][State] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_State
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|HFSM|State",
        DisplayName="[Ck][State] Handle -> State Handle",
        meta = (CompactNodeTitle = "<AsState>", BlueprintAutocast))
    static FCk_Handle_State
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid State Handle",
        Category = "Ck|Utils|HFSM|State",
        meta = (CompactNodeTitle = "INVALID_StateHandle", Keywords = "make"))
    static FCk_Handle_State
    Get_InvalidHandle() { return {}; }

public:
    // Service Management (C++ only)
    static FCk_Handle_Service
    AddService(
        FCk_Handle_State& InStateHandle,
        const FCk_Fragment_Service_ParamsData& InServiceParams);

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