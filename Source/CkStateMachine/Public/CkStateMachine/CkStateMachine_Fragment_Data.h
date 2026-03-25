#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkStateMachine_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLES
// ====================================================================================================================

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
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SmState);

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

// ====================================================================================================================
// ENUMS
// ====================================================================================================================

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
enum class ECk_SmConditionResetBehavior : uint8
{
    ResetEveryFrame,
    Manual
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SmConditionResetBehavior);

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
