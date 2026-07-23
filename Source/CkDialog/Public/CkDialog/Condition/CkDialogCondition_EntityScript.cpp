#include "CkDialogCondition_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogCondition_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    return ECk_Replication::DoesNotReplicate;
}

auto
    UCk_DialogCondition_EntityScript::
    Evaluate(
        FCk_Handle InCondition,
        FCk_Handle_DialogLine InLine,
        FCk_Handle_DialogEmitter InEmitter) const
    -> ECk_Dialog_ConditionResult
{
    // Default routes to the BP/AS hook (unimplemented => Pass, the enum's default). Native subclasses override.
    return DoEvaluate(InCondition, InLine, InEmitter);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogCondition_AlwaysTrue::
    Evaluate(
        FCk_Handle InCondition,
        FCk_Handle_DialogLine InLine,
        FCk_Handle_DialogEmitter InEmitter) const
    -> ECk_Dialog_ConditionResult
{
    return ECk_Dialog_ConditionResult::Pass;
}

// --------------------------------------------------------------------------------------------------------------------
