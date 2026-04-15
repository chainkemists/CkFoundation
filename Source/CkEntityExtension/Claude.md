# CkEntityExtension

**Purpose:** Entity extension system — attaches additional 'extension' fragments to entities post-construction without modifying the entity's primary fragment set. Used for opt-in features added by external modules.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Modules that need to extend any entity with extra fragments without coupling to the entity's original module.

---

## Key API

- `UCk_Utils_EntityExtension_UE` — attach and query extensions on entity handles.

---

## Pattern

Extension pattern: module A defines an entity; module B adds an extension fragment to it without A knowing about B.

---

## Anti-patterns

Don't use extensions to sneak game logic into fragments that should be in a Processor.

---

## See also

- `CkEcs/Claude.md` — Processor access policies that govern who can read extension fragments.
