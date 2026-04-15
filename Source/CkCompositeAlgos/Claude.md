# CkCompositeAlgos

**Purpose:** Composite algorithmic utilities — `Any_Actors_If` and similar composite predicates that combine multiple queries into one. Thin utility module.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`.
**Used by:** Gameplay systems needing combinatorial actor/entity checks.

---

## Key API

- `UCk_Utils_CompositeAlgos_UE::Any_Actors_If(InActors, InPredicate)` — returns true if any actor satisfies the predicate.

---

## Pattern

Use for one-liner 'does any entity in this set satisfy X' checks that would otherwise be a for-loop.

---

## Anti-patterns

Don't use `CompositeAlgos` inside a hot `ForEachEntity` loop without profiling — the inner predicate can be expensive.

---

## See also

- `CkCore/Algorithms/README.md` — lower-level algorithm helpers.
