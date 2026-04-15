# CkIsmRenderer

**Purpose:** Instanced Static Mesh (ISM) rendering via ECS. ISM proxy entities manage ISM instances and update transforms/params from ECS fragments without going through UE's actor system.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkGraphics`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Procedural foliage, projectile trails, crowd rendering — any feature that needs many instances without per-actor overhead.

---

## Key API

- `UCk_Utils_IsmProxy_UE` — add ISM proxy to entity, update transform batch, set custom data.
- `FProcessor_IsmProxy_Setup` — creates the ISM component; `FProcessor_IsmProxy_Update` syncs transforms each tick.

---

## Pattern

Create an IsmProxy entity per mesh type, not per instance. Each IsmProxy entity manages all instances of one mesh type. Instances are indexed, not entity-per-instance.

---

## Anti-patterns

1. Don't use `CkIsmRenderer` for meshes that need per-instance collision or interactability — those need actors.
2. Don't update ISM transforms outside the Setup/Update processors.

---

## See also

- `CkGraphics/Claude.md` — lower-level graphics utilities.
- `CkEcs/Claude.md` — processor and fragment patterns.
