# Spec naming & fragment granularity — design proposal

**Date:** 2026-08-05 · **Status:** PROPOSAL — two maintainer rulings (2026-08-05): the reflected
authoring struct is named **`FCk_<Feature>_Spec`** (F1), and the internal C++ fragment for
retained immutable data **keeps the `FFragment_<Feature>_Params` name** (F7). Remaining forks in
§7 await ruling.
**Scope:** the `ParamsData` / `Params` / `Current` triad across all of CkFoundation (and, by contract, CkTests, CkGameplayDebugger, and downstream game repos).

Evidence basis: first-hand reads of the CkTimer quartet and CkEcsExt/Transform; four full-source
sweeps (naming census, mechanical-coupling map, field-level data-flow audit of 6 modules, modern-
module drift survey) run 2026-08-05. Counts below are from those sweeps; file:line cites were
spot-verified. Anything marked *inferred* was not independently re-derived.

---

## 1. Problem statement

The house pattern — immutable `FFragment_X_Params` (usually an alias of the reflected
`FCk_Fragment_X_ParamsData`) + mutable `FFragment_X_Current` — has three observed failure modes:

1. **Start-values masquerading as config.** Much of a Params fragment is *initial state*: consumed
   once at `Add()`, copied into Current/tags, then dead weight — or worse, a stale second copy.
2. **Two sources of truth.** When runtime state shadows a Params field, readers split between them.
   **Live bug in the canonical module:** `Request_ChangeCountDirection` flips only
   `FTag_Timer_Countdown` (`CkTimer_Utils.cpp:372-384`), but the Reset/Complete/Jump/Consume
   handlers all branch on the stale `Params.Get_CountDirection()` (`CkTimer_Processor.cpp:99, 124,
   223, 283`). After a runtime direction flip, the tick processors follow the tag while every
   request handler follows the original config. The save-hydration path
   (`CkTimer_Fragment.cpp:56-62`) threads through the same divergence.
3. **Monolithic Current.** The census found the fat Currents cluster around three growth patterns:
   (a) runtime copies of tunable params living beside unrelated state
   (`FFragment_VoiceTalker_Current`, 21 members, its own comment admits it), (b) scratch buffers
   stored on the fragment (`FFragment_Minimap_Current`, `_Compass_Current` — 3 scratch arrays
   each), (c) previous/transition snapshot sets merged in (`FFragment_VatProxy_Current`,
   `_Homing_Current`).

Meanwhile the newest modules have already left the pattern — without updating doctrine:

- **CkAttribute** has *no* Params fragment at all: config is consumed entirely inside `Add`
  (values → fragment base values, structure → fragment presence, name → GameplayLabel). Its
  `Has()` anchors on Current alone (`CkFloatAttribute_Utils.cpp:172`).
- **CkEcsExt/Transform** — the shape this proposal generalizes: mutable pose is the bare-noun
  `FFragment_Transform`, flanked by purpose-named small fragments (`_Previous`, `_RootComponent`,
  `_MeshSocket`, `_NewGoal_Location/_Rotation`) and mode tags. Its
  `using FFragment_Transform_Params = FCk_Transform_ParamsData;` (`CkTransform_Fragment.h:36`) is
  **dead code** — never added to any entity, never read. `Add` takes the initial transform as a
  plain argument.
- **CkDialog, CkPathNetwork, CkVoxelNav, CkPoi, CkEntityVisualizer** — no `_Current` anywhere;
  purpose-named fragments instead (`_Cooldowns`, `_PendingQueries`, `_Graph`, `_Corridor`,
  `_BuildState`, `_BuiltOctree`, `_Result`). CkPoi deleted its Params/Current pair outright
  (recorded in `CkPoi/REFACTOR_MultiProjectorPoi.md`).
- **CkPoiDisplayDefinition** documents the halfway doctrine: "Params is the SEED; Current is the
  truth — for the MUTABLE half only" (`CkPoiDisplayDefinition/CLAUDE.md`).

Suffix census across all 421 unique `FFragment_*` structs: `_Requests` 77 · `_Current` 74 ·
`_Params` 70 · a ~185-name long tail of purpose nouns. Nearly half the fragment population already
doesn't use the classic triad. The docs (`Source/CLAUDE.md`, `CkEcs/Claude.md`) still teach the old
monolith shape; every new module written from them regresses.

**Counter-evidence — the pattern is NOT universally wrong.** The field-level audit found genuine
hot-path immutable config where wholesale Params storage is the *correct* shape:

| Feature | Steady-state Params reads |
|---|---|
| CkTween | 5 fields **every tick** per playing tween (`CkTween_Processor.cpp:137-149`) |
| CkStateMachine | `_Replication`/`_AuthorityModel` per SM element per frame via `Get_IsTransitionAuthority` (`CkStateMachine_NetContextUtils.cpp:97-98`) — already frame-memoized *because* it's hot |
| CkSpatialQuery/Probe | matching half (`_ProbeName`, `_ResponsePolicy`, `_Filter`, `_ContextOverlapPolicy`, `_SurfaceInfo`) per contact pair (`CkProbe_Utils.cpp:228-241`) |

So the doctrine cannot be "kill Params." It is: **a config fragment is earned by steady-state
reads, never granted by default.**

---

## 2. The analytical tool — read-class taxonomy

Classify every field of a feature's reflected config struct:

| Class | Definition | Runtime home |
|---|---|---|
| **(a) construction-only** | consumed inside `Add`/Setup, never read again | none — unpack into seeded state, tags, labels, fragment presence |
| **(b) steady-state** | read repeatedly by processors/utils during normal operation | retained immutable config fragment |
| **(c) event-time** | re-read on specific events (completion, reset, loop boundary, re-trigger) | config fragment if genuinely immutable; mutable state if it can change at runtime |

Two hard rules fall out:

- **One home per datum.** A value lives in the config fragment XOR in mutable state/tags — never
  both. (The Timer direction bug is the case study; class (c) "starting values needed on reset"
  does NOT justify a second copy — reset semantics either derive from the state itself, as
  `FCk_Chrono::Reset()` does from its own `_GoalValue`, or the field is class (b) and lives in
  config only.)
- **Class (a) fields never ride along.** If a feature keeps a config fragment, that fragment holds
  ONLY the (b)/(c) residue — not the full reflected struct out of convenience.

---

## 3. Naming convention

### 3.1 The reflected config struct

**`FCk_Fragment_<Feature>_ParamsData` → `FCk_<Feature>_Spec`** — **RESOLVED (F1, maintainer
2026-08-05).**

- `_Spec` incumbents: exactly 4, all CkParticles/CkParticlesEditor internal authoring descriptors
  (`FCk_ParticlesRendererSpec`, `FCk_ParticlesRibbonEmitterSpec`, `FCk_ParticlesTemplateSpec`,
  `FCk_VfxTextureSpec` — verified 2026-08-05). None follows the `FCk_<Feature>_Spec` pattern and
  all already mean "description an instance is built from" — consistent, not colliding.
  `_Settings` stays taken (= a cohesive slice *inside* a spec struct: `FCk_Probe_RayCast_Settings`,
  `FCk_Vfx_TransformSettings`, …). `_Definition` stays taken as a feature noun
  (CkPoiDisplayDefinition). `FCk_EntityReplicationDriver_ConstructionInfo` — the one incumbent
  doing this job under another name — folds into `_Spec` late in the migration.
- Feature-first grammar (`FCk_Timer_Spec`, not `FCk_Spec_Timer`): all 7 organic off-pattern
  drifts already went feature-first (`FCk_Transform_ParamsData`, `FCk_Objective_ParamsData`,
  `FCk_Substep_ParamsData`, …), and it reads naturally at authoring sites in all three
  environments. This knowingly diverges from the kind-first grammar of `FCk_Handle_X` /
  `FCk_Request_X_Y` — those are framework plumbing types; the spec struct is the designer-facing
  type and optimizes for authoring legibility.
- Satellites rename mechanically: `FCk_MultipleTimer_Spec`, `FCk_FloatAttributeRefill_Spec`,
  `FCk_FloatAttributeModifier_Spec`.

Candidate table considered (F1 resolved to Spec):

| Candidate | Verdict |
|---|---|
| `FCk_<F>_Spec` | **CHOSEN.** Shortest, near-zero incumbents (4 consistent internals), GAS-familiar ("spec → instance") |
| `FCk_<F>_Config` | Original recommendation; free at feature granularity but implies steady-state tunables more than construction payload |
| `FCk_<F>_ConstructionInfo` | Precise but long; loses the brevity win that motivates the rename |
| `FCk_<F>_Params` | Smallest delta, but keeps the overloaded word this refactor retires; greps can't separate migrated from unmigrated |
| `FCk_<F>_Recipe` / `_Seed` / `_Setup` | Collide with v3 "spawn recipe" vocabulary / too cute / verb-shaped |

### 3.2 Runtime fragments

| Role | Name | Notes |
|---|---|---|
| Primary state (single obvious blob) | `ck::FFragment_<Feature>` | Transform precedent (`FFragment_Transform`). Timer's chrono, SceneNode's relative transform |
| Other mutable state | `ck::FFragment_<Feature>_<PurposeNoun>` | Codify the organic vocabulary: `_State`/`_BuildState` (in-flight machine), `_Result`/`_Graph`/`_Corridor` (published output), `_Pending<X>` (one-shot intent; presence = view filter), `_<X>Inbox` (net ingress), `_<X>Registry`/`_Cache`/`_History`/`_Cooldowns` (tables), `_Previous` (last-frame snapshot), `_<X>Identity`/`_Anchor`/`_Ref`/`_RootComponent` (linkage), `_Debug` (retained diagnostics) |
| Retained immutable config residue | `ck::FFragment_<Feature>_Params` | **RESOLVED (F7, maintainer 2026-08-05): keep the `Params` name for the internal C++ immutable fragment.** Semantics NARROW: it holds ONLY the (b)/(c) residue — never start-values (`Params ⊆ Spec`). For all-hot features (Tween, StateMachine) this is legitimately `using FFragment_Tween_Params = FCk_Tween_Spec;` — the alias is honest there, and those features' fragments don't rename at all |
| Request queue | `ck::FFragment_<Feature>_Requests` | Unchanged (77 uses, healthy) |
| Boolean modes / switches | `ck::FTag_<Feature>_<Mode>` | Unchanged |

Retired vocabulary: **`_Current`** (for new code; existing uses migrate with Track B),
**`ParamsData`** (after Track A, any surviving `ParamsData` grep hit is by definition an
unmigrated reflected struct), and the **wrapper-fragment shape**
(`struct FFragment_X_Params { ParamsType _Params; }` with its `Get__Params().Get_Y()` double
indirection — 28 instances; becomes either the alias or a residue struct). **`_Params` itself is
NOT retired** (F7): it remains the name for retained immutable config fragments, with narrowed
semantics — a migrated `FFragment_X_Params` contains no construction-only fields, so legacy
wholesale-Params can only be told apart by inspection, not by grep. Do not introduce `_Runtime`,
`_Tracker`, `_Live`, `_Status` — zero organic uses.

Additional codification (from CkVoiceChat's organic practice): `FFragment_<Module>_<X>` prefix for
world-scoped/module-singleton fragments vs `FFragment_<Feature>_<X>` for per-entity feature state.

### 3.3 What does NOT change

- `FCk_Handle_X`, `FCk_Request_X_Y`, `FCk_Delegate_*`, `FCk_RepData_*`, `FCk_SaveData_*` — all
  fully decoupled (the coupling sweep confirmed zero ParamsData references in rep/persistence
  structs, CkSnapshot, CkRecord).
- Processor names/phases, Utils classes, signal macros, request-completion contract.
- The quartet file layout. `X_Fragment_Data.h` keeps its filename for now (fork F5) — a file
  rename is pure include churn on top of an already cross-repo diff.
- Nested `_Settings` slices inside config structs.
- `CK_*` macros — the coupling sweep verified none token-paste or pattern-match the suffix; the
  rename is free at the macro layer.

---

## 4. Data-placement doctrine (the rules)

1. **The Spec struct is the single construction payload.** `Add(Handle, Spec)` unpacks it:
   - start-values → seed the state fragments (`FCk_Chrono{Spec.Get_Duration()}`),
   - boolean/enum modes that gate processors → tags,
   - identity/name → GameplayLabel (never also kept in a fragment),
   - structural options → fragment presence or child entities (Attribute's MinMax shape),
   - (b)/(c) residue → `FFragment_X_Params`.
2. **One home per datum** (§2). No field of the Spec struct may be reachable at runtime through
   two stores.
3. **A config fragment is earned, not granted.** New features start with no `FFragment_X_Params`;
   it appears when the first steady-state read appears, and holds only what that read needs.
4. **Split mutable state by consumer, not by mutability.** The three monolith diseases each get a
   named cure:
   - runtime copies of request-tunable values → their own fragment (they are the feature's
     *tunables*, distinct from its derived/tracked state);
   - scratch buffers → not fragment members (processor-local, or a dedicated `_Scratch` fragment
     if reuse across ticks is measured to matter);
   - previous/transition snapshots → `_Previous` / purpose-named fragment (Transform precedent).
5. **Declare the membership anchor.** Each feature names the fragment (or tag) that `Has()`/
   `Cast()` key on — prefer the primary state fragment (FloatAttribute precedent) or the feature
   tag (Poi precedent). 27 of 91 `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` invocations currently
   key on a Params fragment (5 on Params *alone* — Velocity, Camera, Spline, EntityCollection,
   CrowdAgent); any Track-B refactor of those features MUST move the anchor in the same change or
   `Has()` silently breaks. This is invisible to the compiler — it needs a test per feature.
6. **Replication/persistence read state, never config.** Already true everywhere
   (`FCk_RepData_*` / `FCk_SaveData_*` are hand-authored projections; Produce reads Current-side
   fragments). Codified so it stays true.

---

## 5. Worked example — CkTimer (the pilot)

Today: `FCk_Fragment_Timer_ParamsData { _TimerName, _Duration, _CountDirection, _Behavior,
_StartingState }` stored wholesale; Current = `{ FCk_Chrono }`; direction + run-state also live in
tags; name also lives in the label. Field classes: `_TimerName` (a — label; STATS stat-id can read
the label), `_Duration` (a — chrono `_GoalValue`), `_CountDirection` (a — tag; the four handler
reads are the *bug*, they must read the tag), `_StartingState` (a — tag), `_Behavior` (c —
completion-time, immutable).

After:

```cpp
// Reflected (BP/AS/C++ authoring — unchanged shape, renamed):
FCk_Timer_Spec { _TimerName, _Duration, _CountDirection, _Behavior, _StartingState }

// Add() unpacks:
//   GameplayLabel            ← _TimerName
//   FFragment_Timer          ← FCk_Chrono{_Duration}          (primary state, bare noun)
//   FTag_Timer_Countdown     ← _CountDirection
//   FTag_Timer_NeedsUpdate   ← _StartingState == Running
//   FFragment_Timer_Params   ← { _Behavior }                  (the entire (b)/(c) residue)
```

Behavioral fixes bundled with the pilot: request handlers read `FTag_Timer_Countdown` instead of
the retired Params field (fixes the stale-direction bug); `MakeStatIdFromParams` becomes
label-based; `Has()` anchor moves from `{Current, Params}` to `FFragment_Timer`;
save-hydration's absolute-Jump math reads the tag (it already restores direction via the tag).

**Explicit non-targets:** CkTween and CkStateMachine keep wholesale config storage (their
existing `FFragment_X_Params` names, re-aliased to the renamed Spec struct) — the audit proved
their Params are hot immutable config, and
StateMachine's `_NetContextMemo` exists precisely because those reads are frame-hot. CkProbe
splits: matching half stays config; body-construction half (`_MotionType`, `_MotionQuality`,
`_StartingState`, `_PersistContacts`) is already fully tag-shadowed and dissolves.

---

## 6. Migration — two independent tracks

### Track A — the rename (mechanical, cross-repo, scriptable)

`FCk_Fragment_X_ParamsData → FCk_X_Spec` for all ~124 structs (113 canonical + 11 off-pattern).

- **C++:** pure find-replace; zero macro coupling; zero inheritance from ParamsData; editor detail
  customizations resolve via `StaticStruct()->GetFName()` (no string literals — follow
  automatically).
- **CoreRedirects: mandatory.** 16 `.uasset`s carry the struct FName — including two
  `EntitySpawnParams_*` assets holding the struct inside `FInstancedStruct` (whose serialized
  payload silently drops if the redirect isn't in place before load) and 11 BP function-library
  assets. Precedent exists in `Config/DefaultCkFoundation.ini` (305 entries; 54 StructRedirects,
  4 already touching ParamsData types). One `+StructRedirects` line per struct; then a BP
  recompile-and-resave pass.
- **AngelScript: the dominant cost, and non-atomic.** No AS redirect mechanism exists — every
  hand-written reference is a hard compile break: 30 sites in CkFoundation `Script/`, **1136
  sites across 550 files in CkTests**, 2 in the superproject, plus the BusterBlock trees
  (game + BusterBlockTests — same sweep obligation as any API change). `Script/Generated/` is
  untracked and regenerates free. The rename must land as coordinated same-day commits across
  CkFoundation + CkTests (+ CkGameplayDebugger C++ + game repos), scripted, with the build+test
  gate run per repo.
- Recommended split: per-module batches are possible (each struct's rename is independently
  atomic), but a single scripted campaign keeps the codebase out of a months-long mixed state —
  see fork F3.

### Track B — data placement (judgment, per-feature, incremental)

Priority order, from the audit:

1. **CkTimer** — pilot; smallest surface, fixes a live bug, becomes the new canonical exemplar
   the docs point at.
2. **Shrinks:** CkAudio/AudioTrack (Params never on a tick path; 4 library soft-pointers dead
   after Setup), CkProbe (construction half), CkPmg shapes (Current ≈ Params duplicate).
3. **Monolith splits (case-by-case):** VoiceTalker (21 members, 5 concerns), Camera (16),
   AudioTrack Current (14, 6 delegate handles), VatProxy (13, prev/transition set), Homing (12),
   Minimap/Compass (scratch buffers).
4. **Identity migrations:** the 27 Params-keyed `Has()` anchors, each with a membership test.
5. **Leave-alone list (documented as correct):** CkTween, CkStateMachine, Probe matching half,
   CkVisibleRange (clean 4-field Current, low value).

Cheap adjacent wins found by the audit (can land independently): three processors declare a
Params view member they never read (`FProcessor_Probe_UpdateTransform`, `FProcessor_Probe_EndPlay`,
`FProcessor_Tween_HandleYoyoDelays`); Transform's dead `FFragment_Transform_Params` alias.

### Doctrine updates (part of whichever track lands first)

Root `CLAUDE.md` (two-tier naming table, request/params sections), `Source/CLAUDE.md` §"Add a
feature" ritual + the `FFragment_VfxCue_Current` worked example, `CkEcs/Claude.md` processor
templates, `Script/CLAUDE.md` examples, the `ck-macros-and-codegen` skill, and a DECISIONS.md
entry superseding §4. Without the doc pass, the next module regresses to the old shape.

---

## 7. Forks needing a maintainer ruling

| # | Fork | Status / Recommendation |
|---|---|---|
| F1 | `Config` vs `Spec` vs `ConstructionInfo` | **RESOLVED 2026-08-05: `Spec`** (maintainer) |
| F2 | Feature-first `FCk_Timer_Spec` vs kind-first `FCk_Spec_Timer` | **Feature-first** (organic drift + authoring legibility; accepts grammar divergence from Handle/Request) |
| F3 | Track A as one scripted cross-repo campaign vs incremental per-module (mixed state for months) | **One campaign**, executed after the Timer pilot validates the end-state shape |
| F4 | Primary-state fragment bare (`FFragment_Timer`) vs always-suffixed (`FFragment_Timer_Chrono`) | **Bare when there is one obvious primary blob** (Transform precedent); purpose-suffix otherwise |
| F5 | Rename `X_Fragment_Data.h` files (→ `X_Spec.h`?) | **Defer** — include churn on top of an already cross-repo diff; revisit after Track A |
| F6 | Timer `_Behavior`: keep as 1-field Params residue vs promote to runtime-mutable state (new feature: change behavior at runtime) vs 3 tags | **1-field Params residue** for the pilot; runtime-mutable is a separate feature decision |
| F7 | Runtime immutable fragment name: mirror the Spec name vs keep `FFragment_<Feature>_Params` | **RESOLVED 2026-08-05: keep `Params`** (maintainer) — with narrowed residue-only semantics |
