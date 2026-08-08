# CkInput

**Purpose:** Input context management — adds/removes `UInputMappingContext` objects to entities. Bridges UE's Enhanced Input system with the ECS lifecycle so input contexts are automatically revoked when an entity is destroyed.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`.
**Used by:** Player character entities, ability systems that grant input during active use.

---

## Key API

- Add/remove input mapping contexts on entity handles.
- Context lifetime is tied to entity lifetime.
- `UCk_Utils_Input_UE` — mapping-context add/remove/swap, input-key/chord queries.
- `UCk_Utils_KeyBinding_UE` — the player-rebinding surface over `UEnhancedInputUserSettings`:
  `Get_AllRemappableKeys` / `Get_KeyForMapping` / `Get_KeyForInputAction` (slot-addressed, and read from
  the key profile so they resolve while the owning context is NOT applied), `RemapKey(s)`, `SwapKeys`,
  `UnbindConflictAndRemap`, `Get_HasKeyConflicts` (`ECk_KeyConflictScope::All|SameCategory`),
  `ResetMappingToDefault` / `ResetAllToDefaults`, `SaveKeyBindings`.
- `UCk_KeyBinding_Subsystem` — LocalPlayerSubsystem watching `OnSettingsChanged`; fires a per-mapping-name
  delegate only when that mapping's key actually changed. Handle-based (`FCk_Handle_KeybindListener`).
- `UCk_Utils_KeyIcon_UE` — `Get_BrushForKey` / `Get_BrushForInputAction` / `Get_ActiveControllerData`.
  **`Get_BrushForKey` returns a default-constructed `FSlateBrush` on a miss, which is `DrawAs=Image` with a
  null resource — not `NoDrawType`.** Callers testing "did I get an icon" must check the resource too.

---

## Pattern

Attach an input context to an ability entity; when the ability entity is destroyed, the context is automatically removed from the player controller.

---

## Anti-patterns

Don't manually call `UEnhancedInputLocalPlayerSubsystem::AddMappingContext` in a processor — route through `CkInput` so entity destruction cleanup is guaranteed.

---

## See also

- UE Enhanced Input docs.
- `CkEcs/Claude.md` — entity lifetime and `EndPlay` processor pattern.
