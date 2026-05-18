# CkStateMachine

**Purpose:** ECS-driven state machine — entities with state machine fragments transition between states based on conditions. States are `UCk_SmCondition_EntityScript`-driven with data-asset conditions. Uses `CkDynamic` for state behaviors.

**Depends on:** `CkCore`, `CkDynamic`, `CkEcs`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`.
**Used by:** AI behavior, ability states, game mode state.

---

## Key API

- `UCk_Utils_SmCondition_UE` — add condition entities to state machine.
- `UCk_SmCondition_EntityScript` — override to define state transition conditions in C++ or Blueprint.

---

## Pattern

State machine entity has a Record of state entities; each state has a Record of condition entities; processors evaluate conditions each tick and drive transitions.

---

## Signals

- **`OnSmStarted`** — fires once when the SM enters Running state (auto-start or explicit `Request_Start`). Payload is empty.
- **`OnSmStateChanged`** — fires once on initial state entry with `PreviousStateClass == nullptr`, and again on every subsequent transition with the OLD class in `PreviousStateClass` and the NEW class in `NewStateClass`. Order: `OnSmStarted` fires *before* the initial `OnSmStateChanged`. A sink state (zero outgoing transitions) gets its initial entry fire and then no further fires — pinned by `CkAutoTest_StateMachine_NoTransitionAvailable_StaysInState`.
- **`OnSmStopped`** — fires once when the SM transitions to Stopped (via `Request_Stop` or `EndPlay`).

Consumers that need to react per-state-entry (e.g., apply per-state visuals, swap input modes) should bind `OnSmStateChanged` and treat the initial entry the same as any other transition — they get a clean per-entry pulse regardless of whether the SM ever transitions onward.

---

## Anti-patterns

Don't encode state transition logic in raw if-else chains in a Processor — define states and conditions as data assets consumed by the state machine processor.

---

## Event-driven condition resting state

`UCk_SmCondition_EventDriven::EnterCondition` writes the condition result to **`Fail`** (not `Undetermined`) immediately when the condition becomes active. This is the "not yet satisfied" resting state. Subclasses' `DoEnterCondition` runs after, so a subclass that synchronously calls `MarkSatisfied()` still ends up at `Pass` — the Fail write is the default that holds when no synchronous override happens.

**Why:** the state evaluator (`FProcessor_SmState_Evaluate`) walks transitions in declaration order and `Break`s on the first `Undetermined` it finds. With Undetermined-resting event-driven conditions, a state with two racing event-driven transitions could leave the second one's Pass forever-ignored if the first one's condition was still waiting for its event. Fail-resting lets the evaluator move past the first transition (`Fail` → `Reset` → `Continue`) and inspect the second within the same pump cycle.

**Trade-offs and constraints:**

- **Custom subclasses calling `MarkSatisfied()` synchronously in `BeginPlay`** must call `Super::BeginPlay()` *before* `MarkSatisfied()`. Reversing the order leaves the condition stuck at Fail. `UCk_SmCondition_AlwaysTrue` is the canonical example of this ordering.
- **Negation (`_NegateResult = true`) of an event-driven condition** keeps its prior "never fires" semantic. The Fail resting state is set via `Request_UpdateConditionResult` directly, not via `MarkUnsatisfied` — so the resting value isn't inverted. After the event fires, `MarkSatisfied` with negate still maps to Fail, so the transition still doesn't fire. Negating an event-driven condition has no meaningful gameplay shape; the choice here preserves the prior behavior.
- **Pump cost.** A state with N racing event-driven transitions costs ~2N pump iterations on first state entry (each transition activates → resolves Fail → state evaluator walks past it). The `_MaxPumpIterations` warning fires at 8. For typical 2–4 transition states this is invisible; for larger N consider whether all of those gates need to be on the same state.
- **`Request_ResetTransition` for fully-event-driven transitions** still preserves condition results (event-driven conditions never go back to Undetermined automatically), so a condition that flipped to Pass stays Pass across transition resets. This is unchanged by the resting-state fix.

---

## See also

- `CkDynamic/Claude.md` — state behaviors.
- `CkTimer/Claude.md` — timeouts within states.
