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

## See also

- `CkProvider/Claude.md` — modifier values come from providers.
- `CkRecord/Claude.md` — attributes are Record entries.
- `CkMeter/README.md` — a lighter-weight alternative for single-float bars that don't need modifiers or replication.
