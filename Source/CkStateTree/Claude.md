# CkStateTree

**Purpose:** UE5 StateTree integration for ECS — runs a `UStateTree` asset on an entity, bridging UE's StateTree evaluation with ECS fragment reads/writes.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Complex AI behaviour trees, hierarchical game-mode logic.

---

## Key API

- `UCk_Utils_StateTree_UE` — attach a StateTree asset to an entity and start evaluation.
- StateTree evaluators read ECS fragment state; StateTree tasks write to ECS fragments.

---

## Pattern

Use when the state logic is complex enough to benefit from UE's StateTree visual editor and hierarchy. For simpler cases, prefer `CkStateMachine`.

---

## Anti-patterns

Don't mix `CkStateMachine` and `CkStateTree` on the same entity — choose one state strategy.

---

## See also

- `CkStateMachine/Claude.md` — simpler code-driven alternative.
- UE StateTree docs.
