# CkCueEditor

**Editor module.** Base cue graph editor used by Audio, VFX, and Objective editors.

**Runtime twin:** `CkCue`.

For the full reference — purpose, unique editor additions, rules, and shared infrastructure — see [`/Source/EDITOR_MODULES.md`](/Source/EDITOR_MODULES.md).

## Implementation notes

### `UCk_K2Node_Cue_Base` — the spawn-params struct cache and why it exists

The node caches the cue's generated spawn-params `UScriptStruct` in `_CachedSpawnParamsStruct`. That
`UPROPERTY` is what puts the struct into the Blueprint's **import table** on save; without it the cook's
asset discovery never finds the struct and packaged/cooked runs fail with "Unknown structure". Every
piece of machinery below exists to keep that pointer non-null and non-stale in the *saved* asset.

- **`DoAllocate_DefaultPins` re-syncs the cache eagerly**, before `DoExpandNode` runs. Comparing against
  the freshly fetched pointer (not just null-checking) also catches a stale reference after an
  EntityScript is renamed or recreated — non-null but wrong still fails the cook.
- **The `IsAsyncLoading()` guard on that re-sync is load-bearing.**
  `GetOrCreate_SpawnParamsStructForEntity` can `LoadObject` internally when the struct is not yet in the
  subsystem's cache (e.g. during editor startup while `UCk_EntityScript_Subsystem_UE::Initialize` is
  still iterating assets). That load can pull in another Blueprint package and try to enqueue it for
  compile while `FBlueprintCompilationManager` is already flushing its queue — an ensure. Skipping is
  safe: `PreSave` and the next editor open both repopulate the cache.
- **`PreSave` repeats the same sync** because Unreal can save a Blueprint that was never opened (startup
  resave prompts, "Save All"), in which case `DoAllocate_DefaultPins` never ran and the struct would be
  serialised as null.
- **`DoExpandNode`'s null-cache fallback** covers Blueprints saved before the cache existed (i.e. any
  Blueprint whose cue-name pin has not been touched since the node was placed). On recovery it restores
  the reference and marks the node dirty so the next save writes the struct into the import table.
- **The `DoesPackageExist` hard error** catches a struct that exists in memory but not yet on disk — it
  is written by a deferred save ticker when the EntityScript compiles. Cook-time asset discovery would
  miss it, so the user is told to Save All, recompile, then resave.
- **The stale-cache path warns, it must never `return`.** In the editor a null cache is not a reliable
  problem signal (transient during async load, during startup compile passes, or when `GetOrCreate`
  returns null because `_ActiveCompilation` is set), so the warning is gated on `IsRunningCommandlet()`.
  Even during cook, aborting node expansion here makes the compiler emit "Unexpected node type" and
  breaks the cook — warn and let expansion proceed on the fallback-found struct.
