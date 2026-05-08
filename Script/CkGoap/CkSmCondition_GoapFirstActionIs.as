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
// GOAP lookup uses utils_goap::Find_Goap to handle both "context entity is the
// GOAP" and "context entity owns the GOAP via back-ref" cases — the orchestration
// pattern (e.g. NPC entity-script with SM + GOAP child) needs the second path.
// ============================================================================

UCLASS()
class UCk_SmCondition_GoapFirstActionIs : UCk_SmCondition_Polled
{
    UPROPERTY(Category = "Config")
    TSubclassOf<UCk_GoapAction_EntityScript> ExpectedAction;

    UFUNCTION(BlueprintOverride)
    bool DoEvaluate(FCk_Handle_SmCondition InHandle, FCk_Time InDeltaT) const
    {
        // ExpectedAction must be set by the subclass via `default ExpectedAction = ...`.
        // A null value means the subclass forgot the default — fail loudly so the author
        // sees it instead of getting a silently-never-firing transition.
        if (ck::EnsureIfNot(ck::IsValid(ExpectedAction),
            f"GoapFirstActionIs subclass [{this.GetClass().GetName()}] is missing `default ExpectedAction = ...`"))
        { return false; }

        auto ContextEntity = ck::Ctx(InHandle);
        if (ck::EnsureIfNot(ck::IsValid(ContextEntity),
            f"GoapFirstActionIs [{this.GetClass().GetName()}] has no context entity"))
        { return false; }

        // Find_Goap covers both placement patterns:
        //   1. Context entity IS the GOAP entity (the SM was added directly on the planner) →
        //      returns the cast of the context entity.
        //   2. Context entity OWNS a GOAP child via utils_goap::Add (the typical orchestration
        //      pattern: SM and GOAP both belong to a parent entity, e.g. an NPC script entity)
        //      → returns the back-referenced child handle.
        // Returns invalid only when neither path resolves — that's a real configuration error
        // (the SM is on an entity that has no associated GOAP planner) and ensures loudly.
        auto Goap = utils_goap::Find_Goap(ContextEntity);
        if (ck::EnsureIfNot(ck::IsValid(Goap),
            f"GoapFirstActionIs [{this.GetClass().GetName()}] context entity has no GOAP feature (and no owner-→-GOAP back-reference). Did you forget utils_goap::Add on this entity?"))
        { return false; }

        const auto Plan = utils_goap::Get_Plan(Goap);
        if (Plan.Num() == 0)
        {
            // No plan yet — legitimate transient state during the planner's first cycle, or
            // when the goal is already satisfied. Quietly false; this is not an error.
            return false;
        }

        return Plan[0].Get() == ExpectedAction.Get();
    }
}
