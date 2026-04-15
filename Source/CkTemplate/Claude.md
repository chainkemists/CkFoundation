# CkTemplate

**Purpose:** Entity template system — defines reusable entity configurations (fragment sets + default values) as data assets. Templates are the content-authored blueprint for spawning a specific kind of entity without hardcoding fragment setup in C++.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Any system that spawns entities from designer-defined presets.

---

## Key API

- Template data assets define which EntityScript + Params fragments to set on spawn.
- `UCk_Utils_EntityLifetime_UE::Request_SpawnEntity(World, TemplateAsset, SpawnParams)` — spawn from template.

---

## Pattern

Designer creates a template data asset; programmer passes it to the spawn utility. No C++ changes needed to add new entity configurations.

---

## Anti-patterns

Don't hardcode fragment setup in `Construct()` when a template asset would make it data-driven.

---

## See also

- `CkEcs/Claude.md` — entity lifetime / spawn utilities.
