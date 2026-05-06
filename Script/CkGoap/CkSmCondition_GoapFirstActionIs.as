// ============================================================================
// GOAP -> StateMachine bridge: "Plan[0] equals ExpectedAction"
// ============================================================================
//
// Polled SM condition that reads the GOAP plan on the SM's context entity and
// returns true when the plan's FIRST action class matches a configured class.
// This is the only bridge primitive needed to drive an SM from a GOAP plan:
//
//   - Idle dispatches to per-action states by adding one transition per action
//     class, gated by a subclass of this condition pinning ExpectedAction.
//   - Per-action states can self-preempt by adding the same condition with
//     _NegateResult = true (inherited from UCk_SmCondition_Polled). When
//     GOAP replans and the new Plan[0] differs from "my action class", the
//     state falls back to Idle and the next dispatch fires.
//
// The plan-as-cursor model: there is no separate step index. GOAP only emits
// actions whose preconditions match the current world state, so once an
// action's effects are applied (and the planner replans on either-dirty), the
// next Plan[0] is automatically the next legal action. The SM polls per-tick.
//
// Subclass convention (mirrors UBb_SmCondition_ByteAttribute):
//
//     class UFoo_SmCondition_FirstAction_DoX : UCk_SmCondition_GoapFirstActionIs
//     {
//         default ExpectedAction = UFoo_GoapAction_DoX;
//     }
//
// The condition resolves the GOAP feature from the SM's context entity. The
// SM and the GOAP feature must live on the same entity (the typical NPC
// composition pattern).
// ============================================================================

UCLASS()
class UCk_SmCondition_GoapFirstActionIs : UCk_SmCondition_Polled
{
    UPROPERTY(Category = "Config")
    TSubclassOf<UCk_GoapAction_EntityScript> ExpectedAction;

    UFUNCTION(BlueprintOverride)
    bool DoEvaluate(FCk_Handle_SmCondition InHandle, FCk_Time InDeltaT) const
    {
        if (ck::Is_NOT_Valid(ExpectedAction))
        { return false; }

        auto ContextEntity = ck::Ctx(InHandle);
        if (ck::Is_NOT_Valid(ContextEntity))
        { return false; }

        auto Goap = ContextEntity.As_Goap(ECk_SanityCheck::UnChecked);
        if (ck::Is_NOT_Valid(Goap))
        { return false; }

        auto Plan = utils_goap::Get_Plan(Goap);
        if (Plan.Num() == 0)
        { return false; }

        return Plan[0].Get() == ExpectedAction.Get();
    }
}
