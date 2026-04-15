# CkInput

**Purpose:** Input context management — adds/removes `UInputMappingContext` objects to entities. Bridges UE's Enhanced Input system with the ECS lifecycle so input contexts are automatically revoked when an entity is destroyed.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`.
**Used by:** Player character entities, ability systems that grant input during active use.

---

## Key API

- Add/remove input mapping contexts on entity handles.
- Context lifetime is tied to entity lifetime.

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
