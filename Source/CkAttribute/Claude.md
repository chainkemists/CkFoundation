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

## See also

- `CkProvider/Claude.md` — modifier values come from providers.
- `CkRecord/Claude.md` — attributes are Record entries.
- `CkMeter/README.md` — a lighter-weight alternative for single-float bars that don't need modifiers or replication.
