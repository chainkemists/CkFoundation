#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"

#include "GameplayTagContainer.h"

#include "CkStateMachine_Fragment_Data.generated.h"

// Forward-declared to avoid the circular include with CkSmState_EntityScript.h, which includes us.
class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_StateMachine : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_StateMachine);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_StateMachine);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_SmState : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SmState);
    friend struct FCk_Handle_SmState_UnderConstruction;
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmState);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_SmState_UnderConstruction : public FCk_Handle_SmState
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_DERIVED(FCk_Handle_SmState_UnderConstruction, FCk_Handle_SmState);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmState_UnderConstruction);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_SmTask : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SmTask);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmTask);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_SmTransition : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SmTransition);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmTransition);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATEMACHINE_API FCk_Handle_SmCondition : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SmCondition);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmCondition);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmTaskResult : uint8
{
    Running,
    Succeeded,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmTaskResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmTaskMode : uint8
{
    EnterExitOnly,
    Tick
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmTaskMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmConditionMode : uint8
{
    Polled,
    EventDriven
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmConditionMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmRunStatus : uint8
{
    Stopped,
    Running,
    Paused
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmRunStatus);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmAutoStart : uint8
{
    Disabled,
    OnSetup
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmAutoStart);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmTask_SubSm_CompletionBehavior : uint8
{
    KeepRunning,
    SucceedOnStop
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmTask_SubSm_CompletionBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmCondition_TaskResultsCheck : uint8
{
    AnySucceeded,
    AnyFailed,
    AllSucceeded,
    AllFailed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmCondition_TaskResultsCheck);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmConditionResult : uint8
{
    Undetermined,
    Pass,
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmConditionResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SmTransitionResult : uint8
{
    Undetermined,
    Pass,
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmTransitionResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_StateMachine_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_StateMachine_Spec);

    // Save opt-in is the SaveGame SPECIFIER, never `meta=(SaveGame)` — the latter is inert metadata
    // and round-trips nothing.

private:
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _InitialStateClass;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_SmAutoStart _AutoStart = ECk_SmAutoStart::OnSetup;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Replication _Replication = ECk_Replication::DoesNotReplicate;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_Replication == ECk_Replication::Replicates"))
    ECk_Sm_AuthorityModel _AuthorityModel = ECk_Sm_AuthorityModel::AutoDetect;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_Replication == ECk_Replication::Replicates"))
    ECk_Sm_ReplicationModel _ReplicationModel = ECk_Sm_ReplicationModel::WithHistory;

public:
    CK_PROPERTY_GET(_InitialStateClass);
    CK_PROPERTY(_AutoStart);
    CK_PROPERTY(_Replication);
    CK_PROPERTY(_AuthorityModel);
    CK_PROPERTY(_ReplicationModel);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_StateMachine_Spec, _InitialStateClass);
};

// --------------------------------------------------------------------------------------------------------------------

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

// Installs state-override entries on the target StateMachine. The override class's CDO is queried
// via Get_StatesToOverride() for the states it replaces; multiple states per class are supported.
USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Request_Sm_AddOverrideState : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sm_AddOverrideState);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sm_AddOverrideState);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _OverrideStateClass;

public:
    CK_PROPERTY_GET(_OverrideStateClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sm_AddOverrideState, _OverrideStateClass);
};

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

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
