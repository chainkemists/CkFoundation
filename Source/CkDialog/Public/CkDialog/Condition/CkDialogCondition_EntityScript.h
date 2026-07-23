#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"
#include "CkDialog/Emitter/CkDialogEmitter_Fragment_Data.h"

#include "CkDialogCondition_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Long-lived condition entity-script attached (as a child EntityScript entity) to a dialogue line. Spawned ONCE per
// line at registration; the query processor calls Evaluate() synchronously per query, possibly many times per frame.
//
// CONTRACT: the verdict must be a PURE function of (InCondition, InLine, InEmitter) — never cache a per-caller
// verdict on the instance. The SAME line can Pass for one emitter and Fail for another in the same frame, so a
// cached result would corrupt sibling emitters' queries. Subclasses MAY pre-cache immutable world state in
// BeginPlay (the entity is long-lived); they must NOT cache anything keyed to a specific querying emitter.
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKDIALOG_API UCk_DialogCondition_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogCondition_EntityScript);

protected:
    // Conditions are registry-local content, never a net object (matches the SmCondition precedent).
    auto
    Get_EffectiveReplication() const -> ECk_Replication override;

public:
    virtual auto
    Evaluate(
        FCk_Handle InCondition,
        FCk_Handle_DialogLine InLine,
        FCk_Handle_DialogEmitter InEmitter) const -> ECk_Dialog_ConditionResult;

protected:
    // BP/AS authoring hook. C++ subclasses override Evaluate directly; BP/AS subclasses implement this and the
    // base Evaluate routes to it.
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Dialog|Condition",
        DisplayName = "Evaluate")
    ECk_Dialog_ConditionResult
    DoEvaluate(
        FCk_Handle InCondition,
        FCk_Handle_DialogLine InLine,
        FCk_Handle_DialogEmitter InEmitter) const;
};

// --------------------------------------------------------------------------------------------------------------------

// The always-passes workhorse used by tests and gyms (and as a "line has an unconditional gate" authoring sample).
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Dialog Condition: Always True"))
class CKDIALOG_API UCk_DialogCondition_AlwaysTrue : public UCk_DialogCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogCondition_AlwaysTrue);

    UCk_DialogCondition_AlwaysTrue() = default;

public:
    auto
    Evaluate(
        FCk_Handle InCondition,
        FCk_Handle_DialogLine InLine,
        FCk_Handle_DialogEmitter InEmitter) const -> ECk_Dialog_ConditionResult override;
};

// --------------------------------------------------------------------------------------------------------------------
