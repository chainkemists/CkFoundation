# CkAttribute

**Purpose:** Typed attribute system — Float, Byte, Int, Tag, and more. Each attribute type is its own entity in a Record on the owner. Supports min/max/current, modifiers (providers), replication, and signals for value changes.

**Depends on:** `CkAttribute` (itself), `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`.
**Used by:** Health, stamina, damage, speed — any stat-shaped numeric value on an entity.

---

## Key API

- `UCk_Utils_ByteAttribute_UE::Add(InOwnerHandle, InParams, ECk_Replication)` — create attribute entity.
- `Add` resolves min/max/current sub-attributes internally (they are `TUtils_Attribute<FFragment_ByteAttribute_Min>` etc.).
- Variants: `UCk_Utils_FloatAttribute_UE`, `UCk_Utils_IntAttribute_UE`, `UCk_Utils_TagAttribute_UE`, etc.
- All attributes have `Has`, `Cast`, `CastChecked` and fire signals on value change.

---

## Pattern

Attribute values (Min/Max/Current) are separate sub-fragments on the same attribute entity. Modifiers are applied through providers:

```cpp
// Add a float attribute named 'Health'
auto HealthHandle = UCk_Utils_FloatAttribute_UE::Add(InCharacterHandle, HealthParams);
UCk_Utils_GameplayLabel_UE::Add(HealthHandle, Tag_Attribute_Health);
```

---

## Modifier flavors

Two ways to apply a modifier, with very different semantics:

- **`Add_Revocable`** — always creates a *new* modifier entity per call (see `CkAttribute_Utils.inl.h` ~L147, `RevocablePolicy::Revocable`). Returns a modifier handle so the caller can later revoke it. Use for stackable equipment buffs, temporary status effects, or anything you need to remove individually.
- **`Add_NotRevocable`** — looks for an existing non-revocable modifier of the same operation on the attribute and **coalesces** into it (see `CkAttribute_Utils.inl.h` ~L166). Returns void; there is no per-call handle. Use for permanent, set-once-or-accumulate semantics.

Coalescing rules inside `Add_NotRevocable`:

| Operation | Coalesce behavior |
|---|---|
| `Override` | Replace — latest value wins |
| `Add` / `Subtract` | Accumulate — deltas sum into one modifier |
| `Multiply` / `Divide` | Multiply — factors compose into one modifier |

The `Request_*` utility entry points (e.g. `Request_Override`, `Request_Add`, `Request_Sub`, `Request_Mul`, `Request_Div` on `UCk_Utils_IntegerAttribute_UE` / Float / Byte) all funnel through `Add_NotRevocable`. They mutate persistent modifier state, they are not events.

---

## Anti-patterns

1. Don't store attribute values as plain floats in a feature fragment. Use the attribute system so modifiers and signals work correctly.
2. Don't read attribute values by iterating the Record every frame — cache the attribute handle at setup time.
3. **Don't expect two `Request_*` calls in the same frame to fire two signals.** Attribute mutations coalesce before the processor sees them. Two `Request_Override(attr, A)` then `Request_Override(attr, B)` in the same tick produce a single processor pass that sees only `B` — you get **one** `OnValueChanged` (and at most one `OnMinClamped` / `OnMaxClamped`), reflecting `B`. The `A` mutation is silently overwritten in the modifier. The same applies to `Request_Add`/`Sub` (deltas sum) and `Request_Mul`/`Div` (factors multiply) — only one combined signal fires.
4. For tests or code that needs to observe distinct mutation events, separate the calls across processor ticks. Drive the next mutation from the previous mutation's signal callback (signal-driven step machine) rather than queuing them back-to-back. See gotcha #10 in `Plugins/CkTests/Script/Common/CkAutoTest_CreationSpecification.txt` for the autotest-side implications.

---

## Pre-clamp / overflow polling

The attribute system writes a `TFragment_Attribute_PreClampFinalValue<T, Dir>` per direction at clamp time. Because the Min and Max Clamp processors run sequentially — each capturing `_Final` at *its own* start — the two fragments do NOT symmetrically capture the pre-any-clamp value. With Min-before-Max ordering:

| Scenario | `PreClamp<Min>` | `PreClamp<Max>` |
|---|---|---|
| Value overshoots Max | raw value | raw value |
| Value undershoots Min | raw value | already min-clamped value |

To abstract over this, the utility accessors are **direction-less** — they read both fragments and return the one that actually captured the pre-clamp state:

- `UCk_Utils_IntegerAttribute_UE::Get_PreClampFinalValue(attr)` / `Get_ClampOverflow(attr)` — signed delta, positive = over max, negative = under min
- Float / Byte equivalents
- Template-level `TUtils_Attribute<T>::Get_PreClampFinalValue(handle)` and `Get_ClampOverflow(handle)` if you're inside CkAttribute internals

Avoid reading `TFragment_Attribute_PreClampFinalValue<T, Dir>` directly unless you understand the asymmetry. The signal payload (`FCk_Payload_*Attribute_OnClamped`) is unaffected — it carries event-time values that are correct for the direction whose signal fires.

---

## Implementation notes

The non-obvious constraints the code cannot state for itself (recorded 2026-07-25).

### Clamping

- **`TTag_Attribute_MayRequireClamping<T_AttributeHandle>` is per-FAMILY, keyed on the family's shared
  `HandleType` — deliberately NOT one global tag.** `TProcessor_Attribute_MinMaxClamp` consumes it with a
  registry-wide `Clear` on the transient entity; a single global marker would let one family's clamp pass
  swallow every other family's pending clamps.
- `Attribute_IsWithinBounds` tests "clamping is a no-op" rather than comparing with `<`: composite attribute
  types (`FVector`, `FRotator`) have no well-defined lexicographic ordering.
- `TFragment_Attribute_PreClampFinalValue<T, Dir>` is written by the Clamp processor **every frame**, on both
  the normal path and the client-side-bypass path, so the clamp-signal processor can always read this frame's
  value. When nothing clamped it equals `Current.Final`, so the overflow falls out as 0. It lingers afterwards
  as a debug record. (The Min/Max read asymmetry is the "Pre-clamp / overflow polling" section above.)
- Client-side clamping is bypassed for a replicated attribute whose value is awaiting replication this frame:
  `Current` can arrive before its new bound, and clamping against the stale bound constrains it wrongly. A
  change that needs no replication (a refill) is still clamped client-side.

### Processor scheduling

- **A composite processor — the thing actually registered with the scheduler — must declare its own
  `MarkedDirtyBy` / `MarkedDirtyByAnyOf` surfacing every marker its internals consume.** The internals'
  own declarations never reach a descriptor, so without the alias on the composite the pump loop skips it
  entirely. Applies to `TProcessor_Attribute_FireSignals_CurrentMinMax`, `_MinMaxClamp`,
  `_RecomputeAll_CurrentMinMax`, `TProcessor_AttributeModifier_ComputeAll_CurrentMinMax`.
- `TProcessor_Attribute_Refill_Impl` carries `TExclude<FFragment_RefillAccumulator>` to keep the plain-refill
  query disjoint from the accumulated-refill one. Without it both match the same entity and the refill-target
  lookup ensures at runtime.
- One `RecordOfAttributeModifiers` holds the modifiers of all three components (Current/Min/Max); the
  RecomputeAll specialization must skip the entries it does not own or `Request_ComputeResult` ensures.

### Modifiers

- **`Request_ClearAllModifiers` preserves NotRevocable modifiers, and must keep doing so.** They are permanent
  (set-once / accumulate). Clearing them queued the EmptyTag Override modifier for deferred destroy and let a
  same-frame `Add_NotRevocable` coalesce into a doomed entity — the replicated-attribute alternating-override
  stick.
- `Add_NotRevocable` finding a **pending-destroy** coalesce target has two causes, and only one is a bug:
  (1) entity-lifetime cascade — the owning attribute (or its owner) is being destroyed this frame, which stamps
  the attribute and its child modifiers pending-destroy synchronously; the write is moot, drop it. (2) a
  NotRevocable modifier removed out from under a **live** attribute, which given the rule above should be
  impossible. The ensure discriminates on whether the attribute itself is also pending-destroy.
- `Get_ModifierDeltaValue` returns **by value**: NotRevocable compute `Reset()`s the `TOptional`, so a reference
  into it (or into the unset-case default temporary) dangles.

### Replication and persistence

- SAVE is keyed **per-attribute-entity** (the entity holding the Current component, via
  `ck::attribute_restore::Produce`) while the net Apply stays **owner-keyed**. `HydrationApply` therefore treats
  `InEntity` as the attribute entity itself and writes it directly; the owner-keyed net path never resolves it.
- In the net Apply loop, an attribute entity that is not composed yet leaves the **whole** container entry
  pending (`NotReady`) rather than aborting: siblings that already applied are skipped on the retry by the
  value-equality check, so the retry is idempotent.
- `ck::attribute_restore::HydrationApply` must take **all** `NotReady` exits before any mutation —
  `ApplyReplicated*Entry`'s `Add_Revocable` creates a NEW modifier per call, so a mutate-then-NotReady retry
  would stack a second replication modifier. A saved component the re-Constructed attribute no longer composes
  is skipped with a Verbose log (content drift since the save).
- **Byte diverges from Float/Integer/Vector/Rotator:** its Override delta is unsigned and the Final modifier's
  operation tag is frozen at creation, so a sign-flipped re-apply must RECREATE the modifier rather than
  `Override` it in place.
- The refill **RUN-STATE** (Running/Paused) is never on the wire — it is registered `Save-Only`, keyed on the
  refill CHILD entity, with the Current-component check scoping each per-kind instantiation to its own attribute
  kind. The fill **RATE** already round-trips through that kind's attribute VALUE handler. Hydration is
  authority-side and the NotReady guard precedes the only mutation; `Request_Pause`/`Request_Resume` are
  idempotent, so a retry cannot stack state.

---

## See also

- `CkProvider/Claude.md` — modifier values come from providers.
- `CkRecord/Claude.md` — attributes are Record entries.
- `CkMeter/README.md` — a lighter-weight alternative for single-float bars that don't need modifiers or replication.
