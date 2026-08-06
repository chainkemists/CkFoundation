#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap_Action_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GoapAction_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Action : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Action);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Action);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Action_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Goap_Action_Spec);

private:
    // EntityScript class whose CDO drives Preconditions / Effects / Cost at Setup time.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

    // Optional WS override. If unset, this Action inherits its parent Action's
    // resolved WS at activation time. Top-level (parentless) Actions MUST set
    // this — there is no parent for them to inherit from.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_Goap_WorldState _WorldStateSource_Override;

    // Per-tick A* time slice (microseconds). 0 = unbounded (finish in one tick).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    int64 _SearchBudgetMicroseconds = 50000;

    // Early-out threshold on best A* frontier FScore. If > 0, planner returns
    // CostThresholdReached rather than committing to a plan exceeding it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _CostThreshold = 0.0f;

public:
    CK_PROPERTY_GET(_ActionClass);
    CK_PROPERTY_SET(_ActionClass);
    CK_PROPERTY(_WorldStateSource_Override);
    CK_PROPERTY(_SearchBudgetMicroseconds);
    CK_PROPERTY(_CostThreshold);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Goap_Action_Spec, _ActionClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_Plan : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_Plan);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_Plan);

private:
    // Who asked for this plan — recorded onto the Planner's ReplanCause when consumed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Goap_ReplanOrigin _Origin = ECk_Goap_ReplanOrigin::Explicit;

public:
    CK_PROPERTY(_Origin);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_CancelPlan : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_CancelPlan);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_CancelPlan);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetGoal : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetGoal);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetGoal);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_GoapWS_Condition_Authored> _NewGoal;

public:
    CK_PROPERTY_GET(_NewGoal);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetGoal, _NewGoal);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetActionCost : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetActionCost);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetActionCost);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Cost = 0.0f;

public:
    CK_PROPERTY_GET(_ActionClass);
    CK_PROPERTY_GET(_Cost);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetActionCost, _ActionClass, _Cost);
};

// Carries no cost — it only stamps FTag_Goap_Action_HasCostProvider. The value
// itself is pushed via FCk_Request_Goap_Planner_SetActionCost.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_RegisterActionCostProvider : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_RegisterActionCostProvider);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_RegisterActionCostProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

public:
    CK_PROPERTY_GET(_ActionClass);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_RegisterActionCostProvider, _ActionClass);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetReplanInterval : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetReplanInterval);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetReplanInterval);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MinReplanIntervalSeconds = 0.0f;

public:
    CK_PROPERTY_GET(_MinReplanIntervalSeconds);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetReplanInterval, _MinReplanIntervalSeconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetReplanPolicy : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetReplanPolicy);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetReplanPolicy);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Goap_ReplanPolicy _Policy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

public:
    CK_PROPERTY_GET(_Policy);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetReplanPolicy, _Policy);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetSearchBudget : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetSearchBudget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetSearchBudget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int64 _SearchBudgetMicroseconds = 50000;

public:
    CK_PROPERTY_GET(_SearchBudgetMicroseconds);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetSearchBudget, _SearchBudgetMicroseconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Planner_SetCostThreshold : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Goap_Planner_SetCostThreshold);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_Planner_SetCostThreshold);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _CostThreshold = 0.0f;

public:
    CK_PROPERTY_GET(_CostThreshold);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Planner_SetCostThreshold, _CostThreshold);
};

// --------------------------------------------------------------------------------------------------------------------

// Broadcast source is the sub-Planner whose _IsActive flips, fired from the
// parent's UpdateActivation when it flips its Plan[0].
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlannerActivated
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Goap_Payload_OnPlannerActivated);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlannerDeactivated
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Goap_Payload_OnPlannerDeactivated);
};

// --------------------------------------------------------------------------------------------------------------------
