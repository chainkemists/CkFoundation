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

## Anti-patterns

1. Don't store attribute values as plain floats in a feature fragment. Use the attribute system so modifiers and signals work correctly.
2. Don't read attribute values by iterating the Record every frame — cache the attribute handle at setup time.

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
