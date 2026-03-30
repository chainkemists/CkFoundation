#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkContextReceiver.generated.h"

class UCk_Utils_ContextReceiver_UE;

struct FCk_Handle_ContextReceiver;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ContextReceiver_InjectResult : uint8
{
    Success,
    Failed_InvalidObject,
    Failed_InvalidContext,
    Failed_ContextReceiverPropertyNotFound,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ContextReceiver_InjectResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ContextReceiver_OnContextInjected_MC,
    FCk_Handle_ContextReceiver,
    FCk_Handle);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_ContextReceiver_OnContextCleared_MC,
    FCk_Handle_ContextReceiver);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Lightweight context injection handle.
 *
 * Stores an entity handle representing the current context, plus multicast
 * delegates that fire when context is injected or cleared. Designed to be
 * a persistent UPROPERTY on objects (widgets, EntityScripts, etc.).
 *
 * Context propagation:
 * - TryInjectContextIntoObject() finds all FCk_Handle_ContextReceiver properties
 *   on a UObject (via reflection) and calls Request_Inject on each.
 *
 * All mutation goes through UCk_Utils_ContextReceiver_UE.
 */
USTRUCT(BlueprintType)
struct CKECS_API FCk_Handle_ContextReceiver
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_ContextReceiver);

    friend class UCk_Utils_ContextReceiver_UE;

private:
    UPROPERTY()
    FCk_Handle _ContextEntity;

    FCk_Delegate_ContextReceiver_OnContextInjected_MC _OnContextInjected;
    FCk_Delegate_ContextReceiver_OnContextCleared_MC _OnContextCleared;

public:
    CK_PROPERTY_GET(_ContextEntity);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ContextReceiver_OnContextInjected,
    FCk_Handle_ContextReceiver, InContextReceiver,
    FCk_Handle, InContextEntity);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_ContextReceiver_OnContextCleared,
    FCk_Handle_ContextReceiver, InContextReceiver);

// --------------------------------------------------------------------------------------------------------------------
