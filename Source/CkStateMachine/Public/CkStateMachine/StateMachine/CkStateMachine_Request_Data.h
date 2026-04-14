#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkEcs/Request/CkRequest_Data.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "GameplayTagContainer.h"

#include "CkStateMachine_Request_Data.generated.h"

// ====================================================================================================================
// PARAMS DATA
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Fragment_StateMachine_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_StateMachine_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _InitialStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_SmAutoStart _AutoStart = ECk_SmAutoStart::OnSetup;

public:
    CK_PROPERTY_GET(_InitialStateClass);
    CK_PROPERTY(_AutoStart);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_StateMachine_ParamsData, _InitialStateClass);
};

// ====================================================================================================================
// REQUEST STRUCTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_Start : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_Start);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_Start);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_Pause : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_Pause);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_Pause);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_Resume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_Resume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_Resume);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_Transition : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_Transition);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_Transition);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _TargetStateClass;

public:
    CK_PROPERTY_GET(_TargetStateClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sm_Transition, _TargetStateClass);
};

// --------------------------------------------------------------------------------------------------------------------

// Installs a state-override entry on the target StateMachine.
// Processed by FProcessor_Sm_HandleRequests which appends the entry to FFragment_Sm_StateOverrides._Overrides.
// Thereafter any state the SM would spawn whose hierarchy matches _OverriddenStateHierarchy will be spawned
// as _OverridingStateClass instead. Match semantics:
//   - _OverriddenStateHierarchy.Num() == 1 -> loose match by leaf tag only
//   - _OverriddenStateHierarchy.Num() >  1 -> exact root->leaf element-wise match
USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_OverrideState : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_OverrideState);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_OverrideState);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FGameplayTag> _OverriddenStateHierarchy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _OverridingStateClass;

public:
    CK_PROPERTY_GET(_OverriddenStateHierarchy);
    CK_PROPERTY_GET(_OverridingStateClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sm_OverrideState,
        _OverriddenStateHierarchy, _OverridingStateClass);
};

// ====================================================================================================================
// SIGNAL PAYLOADS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Sm_Payload_OnStateChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_Payload_OnStateChanged);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _PreviousStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _NewStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_SmState _NewStateHandle;

public:
    CK_PROPERTY_GET(_PreviousStateClass);
    CK_PROPERTY_GET(_NewStateClass);
    CK_PROPERTY_GET(_NewStateHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sm_Payload_OnStateChanged, _PreviousStateClass, _NewStateClass, _NewStateHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Sm_Payload_OnStarted
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_Payload_OnStarted);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Sm_Payload_OnStopped
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_Payload_OnStopped);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Sm_Payload_OnSubSmConstructed
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_Payload_OnSubSmConstructed);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        Category = "Ck|SM|Payload",
        meta = (AllowPrivateAccess = true))
    FCk_Handle_StateMachine _SubStateMachineHandle;

public:
    CK_PROPERTY_GET(_SubStateMachineHandle);
    CK_DEFINE_CONSTRUCTORS(FCk_Sm_Payload_OnSubSmConstructed, _SubStateMachineHandle);
};

// ====================================================================================================================
// DELEGATES
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sm_OnStateChanged,
    FCk_Handle_StateMachine, InHandle,
    FCk_Sm_Payload_OnStateChanged, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sm_OnStarted,
    FCk_Handle_StateMachine, InHandle,
    FCk_Sm_Payload_OnStarted, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sm_OnStopped,
    FCk_Handle_StateMachine, InHandle,
    FCk_Sm_Payload_OnStopped, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_SmTask_OnFinished,
    FCk_Handle_SmTask, InTaskHandle,
    ECk_SmTaskResult, InResult);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_SmTask_OnSubSmConstructed,
    FCk_Handle_SmTask, InTaskHandle,
    FCk_Sm_Payload_OnSubSmConstructed, InPayload);

// --------------------------------------------------------------------------------------------------------------------
