#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"
#include "CkDialog/Emitter/CkDialogEmitter_Fragment_Data.h"

#include "CkDialogCondition_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Long-lived condition child entity of a dialogue line, spawned ONCE at registration; Evaluate runs synchronously
// per query, possibly many times per frame. CONTRACT: the verdict must be a PURE function of (InCondition, InLine,
// InEmitter) — the SAME line can Pass for one emitter and Fail for another, so never cache a per-caller verdict.
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
    // BP/AS authoring hook that the base Evaluate routes to; C++ subclasses override Evaluate directly.
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
