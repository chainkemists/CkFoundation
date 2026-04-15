# CkAnimation

**Purpose:** Animation assets on entities. Each entity can hold a Record of AnimAsset entities; each AnimAsset carries animation params (asset reference, slot, play mode). The Utils class follows the standard Add / Has / Cast / Record pattern.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`.
**Used by:** Character movement, ability systems, any feature that plays animations on an entity.

---

## Key API

- `UCk_Utils_AnimAsset_UE::Add(InHandle, InParams)` — add a new animation asset to the entity's Record.
- `UCk_Utils_AnimAsset_UE::AddMultiple(InHandle, InParams)` — batch add.
- `UCk_Utils_AnimAsset_UE::Has(InHandle)` — entity has an anim asset Record.
- Standard Cast / CastChecked / InvalidHandle helpers.

---

## Pattern

All anim asset entities live in a Record on the owning entity. Use `CkLabel` to distinguish named anim slots (e.g., `Tag_Anim_Attack`, `Tag_Anim_Idle`).

```cpp
auto TrackHandle = UCk_Utils_AnimAsset_UE::Add(InOwnerHandle, AnimParams);
UCk_Utils_GameplayLabel_UE::Add(TrackHandle, Tag_Anim_Attack);
```

---

## Anti-patterns

1. Don't drive animations from Processors directly — route through the AnimAsset entity so the animation system's own processors manage lifecycle.
2. Don't skip labeling anim entries when an entity needs multiple anim slots.

---

## See also

- `CkRecord/Claude.md` — Record entity pattern.
- `CkLabel/Claude.md` — naming anim slots.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure used by AnimAsset.
