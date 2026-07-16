# CkJoltEditor

**Purpose:** Editor-side cooker for the CkJolt static world. Extracts static collision (the SAME
`ck::jolt::bake` extraction the runtime PIE path uses — one extraction path, no divergence),
partitions actor groups into bake-grid cells, serializes deduped Jolt shape blobs
(`Shape::SaveWithChildren`), and saves the per-cell + index data assets under the configured
cooked-data root (`UCk_Jolt_ProjectSettings_UE::_CookedDataRootPath`).

**Depends on:** `CkCore`, `CkJolt`, `CkLog`, `CkThirdParty` + `UnrealEd`, `EditorSubsystem`,
`AssetRegistry`, `ToolMenus`, `Landscape`.
**Type:** Editor (never loads in game/cooked).

---

## Key API

- `FCk_Jolt_WorldCooker::Cook_World(World, DryRun)` — the shared cooker (world in, assets out).
- `UCk_JoltCook_EditorSubsystem_UE` — the PRIMARY cook vehicle (world already booted by the
  editor): `Cook_CurrentWorld()` / `Cook_CurrentWorld_DryRun()` / `Validate_CurrentWorld()`,
  BlueprintCallable for editor-utility widgets + a Tools-menu entry.
- `UCk_JoltCook_Commandlet` — headless cross-map cook (`-run=CkJoltCook -Map=… | -AllMaps
  [-DryRun]`), CK_ENSUREs the DirectoriesToAlwaysCook ini entry. Boots worlds itself; if that
  proves flaky on WP/landscape maps, the documented pivot is a UWorldPartitionBuilder subclass.

## Anti-patterns

- Don't add a second extraction path — `ck::jolt::bake` is shared with the runtime deliberately.
- Don't hand-author cooked assets; only the cooker writes them (versioning + hashes).
- Orphaned cell assets from prior cooks are LOGGED, not auto-deleted (v1) — clean by hand.
