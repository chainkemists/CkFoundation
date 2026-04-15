# CkResolver

**Purpose:** Multi-phase computation pipeline — the Resolver batches 'operations' (e.g., incoming damage, buff applications) into a `ResolverDataBundle` entity, resolves them through a sequence of processor phases (gather → calculate → apply), then fires results.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTargeting`.
**Used by:** Damage calculation, buff/debuff resolution, any system with 'compute once from many sources, apply once'.

---

## Key API

- `UCk_Utils_ResolverDataBundle_UE` — create a data bundle, push operations into it.
- Processors: `StartNewPhase → HandleRequests → ResolveOperations → Calculate`.
- Signal: `OnResolved` with final output.

---

## Pattern

Multiple sources push operations into the bundle this frame; the Calculate processor combines them and applies the net result:

```cpp
// Push an incoming damage operation:
UCk_Utils_ResolverDataBundle_UE::Push_DamageOperation(BundleHandle, DamageParams);
// The Resolver processor phases evaluate and apply it at end of frame.
```

---

## Anti-patterns

1. Don't apply damage/buffs directly from event handlers — push to the Resolver bundle so all sources can be seen and arbitrated in one pass.
2. Don't write to attribute fragments during Resolve phases — write to the bundle output and let the Apply phase commit.

---

## See also

- `CkAttribute/Claude.md` — attributes are the primary output target.
- `CkTargeting/Claude.md` — target selection feeds into resolver operations.
