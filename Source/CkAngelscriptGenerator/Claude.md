# CkAngelscriptGenerator

**Purpose:** Code generator — scans the asset registry and generates AngelScript accessor files (`.as`) for discovered assets. Driven by `UCkAssetRegistryConfig` data assets. Editor-only.

**Depends on:** `CkCVar`, `CkCore`, `CkDynamic`, `CkEcs`, `CkEcsExt`, `CkEntityExtension`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Editor build process — invoked when asset registry configs change.

---

## Key API

- `UCk_Utils_AssetRegistry_UE::Generate_All_Asset_Registries()` — trigger generation for all `UCkAssetRegistryConfig` data assets.
- `Get_Asset_Registry_Subsystem()` — access the subsystem that tracks registered assets.

---

## Pattern

Define a `UCkAssetRegistryConfig` pointing at your content folder; the generator produces a `.as` file with typed accessors. Run via editor toolbar or console command.

---

## Anti-patterns

Don't commit generated `.as` files — regenerate on build or on asset registry change.
(See `/Source/CLAUDE.md` section 15 for `asset ... of UCkAssetRegistryConfig` syntax.)

---

## See also

- `/Source/CLAUDE.md` section 15 — AngelScript asset creation.
- `CkCore/Reflection/README.md` — property/class introspection used by the generator.
