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

## Anti-patterns

Don't encode state transition logic in raw if-else chains in a Processor — define states and conditions as data assets consumed by the state machine processor.

---

## See also

- `CkDynamic/Claude.md` — state behaviors.
- `CkStateTree/Claude.md` — UE5 StateTree alternative.
- `CkTimer/Claude.md` — timeouts within states.
