# Silent-breakage catalog

Reference for `ck-angelscript-interop`: every way a binding vanishes or misbehaves without a compile error, with the tell for each.

## 2. Silent-breakage catalog

Thirteen verified failure modes. Items 1-10 are silent (compile passes or the symptom appears far
from the cause); 11-12 are loud but misleading; 13 is the "where do errors even land" map.
Format per item: **Symptom → Why silent → Check → Fix.**

**1. Stale/absent `DynamicHandleTypes.json`** — the dynamic-handle registry
(`asset <X>Handle of UCkDynamic_HandleDefinition` needs a matching JSON entry; Script/CLAUDE.md §7).
- Symptom: project-wide cascade `Identifier 'FCk_Handle_<X>' is not a data type` at editor start,
  plus `Hot reload failed ... Keeping all old script code`.
- Why silent/misleading: with self-heal ON (default) these first-pass errors are **expected
  transients** — the dispatcher writes `_StubRecovery_DynamicHandleTypes.json` + a permissive
  validator, the deferred regen (OnPostEngineInit) writes the real entry, and the recompile goes
  clean, all in ONE boot. Panicking at the first-pass errors leads to wrong "fixes".
- Check: success gate = a clean reload after the deferred regen AND the entry present in the JSON.
  Path chain: `UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath()`
  (`Source/CkDynamic/Public/CkDynamic/Settings/CkDynamic_Settings.cpp` — default `ProjectConfigDir`,
  superprojects override; BusterBlock: `Config/DefaultCkFoundation.ini:118` →
  `<Project>/Script/Generated/DynamicHandleTypes.json`). PowerShell (handles the UTF-16 encoding):
  `Select-String -Path <Project>\Script\Generated\DynamicHandleTypes.json -Pattern 'FCk_Handle_<X>'`.
- Fix: nothing — boot once and let self-heal converge. **A second boot still red = real problem**
  (convergence banner in the log names the callsites). File is UTF-16 LE — preserve encoding if you
  ever hand-edit (rebase staleness needs a UTF-16-preserving union merge). Manual regen: §3 buttons.

**2. Raw `FCk_Handle` in an f-string** — RUNTIME throw, invisible to every compile gate.
- Symptom: `Invalid type to append to string.` thrown at PIE-start or whenever the line executes;
  script execution of that function aborts.
- Why silent: the throw site is the engine's dynamic append fallback
  (`Private/Binds/Bind_FString.cpp:597`) — **not a compile error**. A clean editor/headless boot
  (with or without `-skipcompile`) proves nothing about this; the line must actually run.
- Check: exercise the code path (PIE, or an AutoTest that executes the line) and grep the log for
  the throw message.
- Fix: `f"{SomeHandle.ToString()}"` — every typesafe handle has `.ToString()` bound (§1.3).

**3. EntitySpawnParams phantom namespace** — deleted entity script + surviving generated block.
- Symptom: an entity-script class deleted and later re-added **with the same name** never registers
  as a live UClass — `UObjectIterator` misses it, the AutoTest populator silently drops the test,
  the class never appears in Session Frontend. Reproduced 2026-05-12.
- Why silent: `<Plugin>_EntitySpawnParams.as` emits the class name as a real AS identifier
  (`namespace U<Script> { ... }` + `F<Script>_SpawnParams`); the stale block makes AS treat the
  re-added name as already-known and it skips UClass registration without any error.
- Check: with the class `.as` deleted, `rg --no-ignore -n "U<ClassName>"
  <Plugin>/Script/Generated/*_EntitySpawnParams.as` — a surviving block is the smoking gun.
- Fix: when reverting generator/test state, revert **every** file under `Script/Generated/*.as`
  atomically, never `AutoTestActors.as` alone (canonical:
  `Source/CkAngelscriptGenerator/Claude.md`, "EntitySpawnParams.as is NOT resilient..."). Emergency
  unblock: rename the class.

**4. BFL suffix-strip collision** — engine rewrites your namespace.
- Symptom: `No matching signatures to 'UMyFeature_FunctionLibrary::Foo()'` — looks like a param
  mismatch; actually the class name was silently rewritten.
- Why silent: the engine strips suffixes `Statics/Library/BlueprintLibrary/BlueprintFunctionLibrary/FunctionLibrary`
  and prefixes `UKismet/UBlueprint` from BFL names when namespacing
  (`Public/AngelscriptSettings.h:124-139`).
- Check: does your BFL name end in a strip-list suffix?
- Fix: name it `UCk_Utils_<X>_UE` (house rule; `_UE` round-trips), or override with
  `UCLASS(meta = (ScriptName = "..."))`. Details: Script/CLAUDE.md §16.1.

**5. Unchecked parent handle up-conversion** — the implicit conversion carries no guarantee.
- Symptom: an ensure/crash **deep inside a downstream util** ("fragment missing") far from the call
  that introduced the bad handle.
- Why silent: derived→parent implicit conversion is a byte pass-through — `opImplConv` is literally
  `return InOther;` (`CkHandle_TypeSafe_AngelScript.h:48-57`), and the parent-chain pass binds the
  same unchecked shape for every ancestor (`CkHandle_AngelScript_Registry.h:200-215`). No
  CastChecked or fragment-presence ensure runs at the boundary.
- Check: when a util ensures on a handle you converted, audit where the typed handle came from —
  especially handles produced while a **permissive dynamic-handle validator** was live (§3) or
  default-constructed.
- Fix: when provenance is uncertain, use the explicit `As_<Parent>()` / typed cast so the boundary
  diagnostic fires at the conversion, not three calls later. (Script/CLAUDE.md §6.)

**6. New C++ ScriptMixin function — wrong call form, wrong build order.**
- Symptom: fresh C++ util function compiles, but AS `UCk_Utils_X_UE::Func(Handle, ...)` reports
  "No matching signatures".
- Why silent/misleading: if arg0's type equals the class's ScriptMixin target, the function was
  bound as a **member only** (§1.2) — the static spelling never existed. Compounding: C++ must be
  **built before** the AS that calls it compiles (next editor boot regenerates the wrapper).
- Check: compare arg0's type to the `ScriptMixin` meta string on the UCLASS; then check
  `Script/Generated/utils_<feature>.as` for the emitted wrapper.
- Fix: build C++ → boot editor (wrapper regenerates) → call `utils_<feature>::Func(...)`, or the
  member form on a mutable local.

**7. Blanket-deleting or touching `Script/Generated/`** — the mtime trap.
- Symptom: multi-second frozen editor right after boot (full AS reload sweep, literal-asset
  re-init), or an endless reload loop; historical incident: ~8s frozen on EVERY launch (2026-06-11).
- Why silent: the Hazelight hot-reload checker baselines `.as` **mtimes** at its first scan — ANY
  later mtime change under `Script/Generated/`, byte-identical or not, triggers a game-thread
  reload. The generator's own hygiene depends on this: manifest-based cleanup via `_index.as` and
  `SaveWrapperFile_IfChanged` (LF-normalized compare) exist precisely so unchanged files keep their
  mtimes (`CkAngelscriptWrapperGenerator.cpp:40,94,190-191`; incident write-up in the module
  Claude.md, "Mtime stability").
- Check: post-init structural reload of a `Generated.*` module in the log; the ES Params generator
  logs a **rewrite reason** whenever a bucket rewrites.
- Fix: **never** `rm Script/Generated/*` and never script anything that rewrites those files while
  an editor runs. Recovery from bad generated state = git revert of the whole directory (item 3)
  with the editor closed, or the §3 regen buttons.

**8. Two editor/headless instances of one project** — Rev 12 single-writer lock.
- Symptom: in the second instance, generated files never update; regen buttons appear to do
  nothing beyond a Warning.
- Why silent-ish: an exclusive-write OS file lock on
  `<ProjectSavedDir>/CkAngelscriptGenerator_RegenOwner.lock`
  (`CkAngelscriptGenerator_RegenOwnership.cpp:115-118`) makes every later instance a **read-only
  secondary**: it compiles against the owner's generated files and writes nothing. One prominent
  startup Warning; per-site skips are VeryVerbose. This *replaced* the pre-Rev-12 failure mode
  (two writers ping-ponging mirror rewrites — 496 rewrites/686 reloads in the 2026-06-12 incident).
  OS releases the handle on any process exit, so stale locks are impossible; a surviving secondary
  lazily takes over at its next regen event ("Ownership ACQUIRED" log line).
- Check: the startup Warning ("SECONDARY"), or the lock file's breadcrumb (pid + cmdline) in Saved.
- Fix: intentional. If you need this instance to generate, close the owner first. A headless
  compile-check alongside an open editor is now safe by design.

**9. Spawn-params codegen lag** — new entity script or new `ExposeOnSpawn` property.
- Symptom: `No matching signatures to '<Class>::Params(...)'` (or
  `Identifier 'F<X>_SpawnParams' is not a data type` on a fresh clone) on the first compile pass
  after the change.
- Why silent/misleading: `<Plugin>_EntitySpawnParams.as` is emitted by a **post-compile** generator
  — a brand-new class + its callsite in the same pass can't see the accessor yet. With self-heal ON
  this is an expected transient: the dispatcher synthesizes a sibling `_StubRecovery_*` stub
  (source-derived full shape when it can find your class's `.as`), compile succeeds, the real
  generator regenerates, the stub is deleted.
- Check: second compile pass clean + the accessor present in the canonical file.
- Fix: none needed normally. Self-heal disabled: break the callsite, compile, restore (the manual
  two-phase). Superproject detail: BusterBlock `Script/CLAUDE.md`, "Codegen lag" section.

**10. Typed delegates can't ride `ExposeOnSpawn`.**
- Symptom: a `::Params(...)` overload taking your `FCk_Delegate_*` / typed AS delegate never
  matches, no matter what you pass.
- Why silent: the generated SpawnParams struct coerces the delegate property to a generic script
  delegate; there is no implicit conversion from the typed delegate at the callsite.
- Check: read the emitted struct in `<Plugin>_EntitySpawnParams.as` — the field isn't your type.
- Fix: `ExposeOnSpawn` the `(UObject Target, FName FunctionName)` pair and rebuild the typed
  delegate inside `DoConstruct`. Worked example: BusterBlock `Script/CLAUDE.md`, "Typed delegates
  can't ride through ExposeOnSpawn".

**11. Adjacent string literals** (loud but misleading).
- Symptom: compile error pair `Expected ')' or ','` + `Instead found '<string constant>'`.
- Cause: `"foo " "bar"` C-style splicing — AS does not splice adjacent literals. The self-heal
  parser recognizes the shape and banners file:line:col with suggested fixes, but **never edits
  user source** (author-side bug, out of its contract).
- Fix: one literal, or f-string interpolation.

**12. By-value struct params are read-only; const propagates hard** (loud but misleading).
- Symptom: `Cannot assign, variable is const or is not a valid l-value` at any nesting depth, or
  baffling "no matching signature" when passing a const value to a non-const value param.
- This is AS language semantics, not a Ck binding bug — full rules and fixes:
  Script/CLAUDE.md §9.1 (by-value read-only) and §9.2 (const propagation, `Cast<T>` preserves
  const, `TArray::Add` of const rejected).

**13. Where AS errors land, and what triggers a recompile** (the map).
- Editing any watched `.as` → engine `FAngelscriptManager::CheckForHotReload`
  (`Private/AngelscriptManager.cpp:1531`) → `PerformHotReload` (`:1211`). Compile diagnostics land
  in the editor log under the **`Angelscript`** category (`DEFINE_LOG_CATEGORY(Angelscript)`,
  `AngelscriptManager.cpp:64`) — grep for `Angelscript: Error`. Verdict lands ~2s after save; check
  immediately, never poll with sleep loops.
- Boot-time compile failure opens the Hazelight modal — that's where the self-heal dispatcher (§3)
  intervenes from a deferred modal-tick callback.
- A parse error anywhere in a file kills **every class in that file** silently downstream: placed
  actors fall back to native base classes, `default X = ...` class references resolve to nothing.
  Fix the first error before chasing ghost symptoms.

---

