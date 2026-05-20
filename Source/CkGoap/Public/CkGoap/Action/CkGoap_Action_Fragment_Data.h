#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap_Action_Fragment_Data.generated.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;

// ====================================================================================================================
// TYPESAFE HANDLE — one entity per registered Action in an ActionSet's catalog.
// Action entities carry the action's def (CDO-extracted), tree edges
// (_ParentAction, _ChildActions), runtime planner state, and an active-parent
// breadcrumb used by the ActionSet's ChainUpdate processor.
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Action : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Action);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Action);

// ====================================================================================================================
// ACTION PARAMS — BlueprintType data shape for an Action entity. Carries the
// Action's class (CDO source), optional WS override, and per-Action planner
// knobs. In the unified ActionSet/Action model, every Action has its own
// planner state and can itself extend the active chain.
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_ActionParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Goap_ActionParamsData);

private:
    // The Action's EntityScript class. Its CDO drives the Action's
    // Preconditions / Effects / Cost at Setup time.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

    // Optional WS override. If unset, this Action inherits its parent Action's
    // resolved WS at activation time. Top-level (parentless) Actions MUST set
    // this — there is no parent for them to inherit from.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_Goap_WorldState _WorldStateSource_Override;

    // Top-level only: initial goal world state. Ignored on non-top-level
    // Actions (their goal is injected from the parent Action's Effects at
    // activation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_GoapWS_Condition_Authored> _InitialGoal_RootOnly;

    // Per-tick A* time slice (microseconds). 0 = unbounded (finish in one tick).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    int64 _SearchBudgetMicroseconds = 50000;

    // Early-out threshold on best A* frontier FScore. If > 0, planner returns
    // CostThresholdReached rather than committing to a plan exceeding it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _CostThreshold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MinReplanIntervalSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _PlanOnStart = true;

public:
    CK_PROPERTY_GET(_ActionClass);
    CK_PROPERTY_SET(_ActionClass);
    CK_PROPERTY(_WorldStateSource_Override);
    CK_PROPERTY(_InitialGoal_RootOnly);
    CK_PROPERTY(_SearchBudgetMicroseconds);
    CK_PROPERTY(_CostThreshold);
    CK_PROPERTY(_ReplanPolicy);
    CK_PROPERTY(_MinReplanIntervalSeconds);
    CK_PROPERTY(_PlanOnStart);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_ActionParamsData, _ActionClass);
};

// ====================================================================================================================
// PER-ACTION REQUESTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_Plan
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_Plan);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_CancelPlan
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_CancelPlan);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetGoal
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetGoal);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_GoapWS_Condition_Authored> _Goal;

public:
    CK_PROPERTY_GET(_Goal);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetGoal, _Goal);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetActionCost
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetActionCost);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Cost = 0.0f;

public:
    CK_PROPERTY_GET(_ActionClass);
    CK_PROPERTY_GET(_Cost);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetActionCost, _ActionClass, _Cost);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetReplanInterval
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetReplanInterval);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MinReplanIntervalSeconds = 0.0f;

public:
    CK_PROPERTY_GET(_MinReplanIntervalSeconds);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetReplanInterval, _MinReplanIntervalSeconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetReplanPolicy
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetReplanPolicy);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Goap_ReplanPolicy _Policy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

public:
    CK_PROPERTY_GET(_Policy);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetReplanPolicy, _Policy);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetSearchBudget
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetSearchBudget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int64 _SearchBudgetMicroseconds = 50000;

public:
    CK_PROPERTY_GET(_SearchBudgetMicroseconds);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetSearchBudget, _SearchBudgetMicroseconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Action_SetCostThreshold
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Action_SetCostThreshold);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _CostThreshold = 0.0f;

public:
    CK_PROPERTY_GET(_CostThreshold);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Action_SetCostThreshold, _CostThreshold);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — Action-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActionActivated
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Goap_Payload_OnActionActivated);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActionDeactivated
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Goap_Payload_OnActionDeactivated);
};

// ====================================================================================================================
// DELEGATES — Action-scoped
//
// FCk_Goap_Payload_OnPlanComplete / _OnPlanFailed already exist in
// CkGoap_Fragment_Data.h and are reused here — only the SOURCE handle type
// differs from the planner-era delegates.
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Goap_OnActionPlanComplete,
    FCk_Handle_Goap_Action, InAction,
    FCk_Goap_Payload_OnPlanComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Goap_OnActionPlanFailed,
    FCk_Handle_Goap_Action, InAction,
    FCk_Goap_Payload_OnPlanFailed, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Goap_OnActionActivated,
    FCk_Handle_Goap_Action, InAction,
    FCk_Goap_Payload_OnActionActivated, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Goap_OnActionDeactivated,
    FCk_Handle_Goap_Action, InAction,
    FCk_Goap_Payload_OnActionDeactivated, InPayload);

// ====================================================================================================================
