---
name: ck-game-angelscript-gameplay
description: 'Use when writing CkFoundation gameplay AngelScript: utils and mixin idioms, deferred requests, signal binding, hot reload, or deciding between AS and C++.'
---

# ck-game-angelscript-gameplay

## Overview

AngelScript is the daily-driver language for gameplay on CkFoundation. Corpus proof (BusterBlock,
measured 2026-07-03, HEAD `52a75e13d`): ~186k hand-written gameplay AS lines vs ~2.2k game C++
(~1%); 135 AS processors, 592 AS entity scripts, **zero** game-defined C++ fragments or processors.
Maintainer ruling (2026-07-03, settled doctrine): features drop to C++ "rarely, unless it's _very_
clear it's going to be a perf concern."

This skill teaches the consumer gameplay layer: the mental model, the verified idioms, the
hot-reload workflow, the silent failure modes you will actually hit, and the when-to-drop-to-C++
rule. Language reference and framework-binding machinery are cited, not restated.

**Read first, in this order** (both are prerequisites, not alternatives to this skill):

1. `Plugins/CkFoundation/Script/CLAUDE.md` — the AS language deltas from C++ (no lambdas, `float`
   is 64-bit, RPCs reliable-by-default, by-value struct params read-only, const propagation), the
   `utils_*` layer, dynamic handles, `asset ... of ...` blocks, the §21 mistakes table.
2. `ck-angelscript-interop` skill — the binding machinery (ScriptMixin rule, generated wrappers,
   the 13-item silent-breakage catalog, self-heal, verification recipe).

Jargon used below: a **handle** (`FCk_Handle`, and typesafe subtypes like `FCk_Handle_Timer`) is a
lightweight reference to an ECS entity; a **fragment** is a data component on an entity; a
**feature** is a composable capability (fragments + utils + processors); an **EntityScript** is the
AS class that composes an entity and is the placeable unit of gameplay (No-Actors doctrine:
EntityScripts replace ~95% of Actor use).

## When NOT to use this skill

| You are actually doing | Load instead |
|---|---|
| AS compile walls, "not a data type", `_StubRecovery_` files, generator/self-heal behavior, DynamicHandleTypes.json | `ck-angelscript-interop` |
| Looking up AS syntax (structs, delegates, actors, networking keywords) | `Plugins/CkFoundation/Script/CLAUDE.md` |
| Structuring a new feature (Feature/Utils/EntityScript/Processor files, the arc, definition of done) | `ck-game-feature-recipe` |
| Entity lifetime, ownership, trait composition, archetypes | `ck-game-entity-composition-patterns` |
| Replicated spawns, ActorRelay lifetime owners, authority routing | `ck-game-replication-patterns` |
| Writing or running tests for gameplay code | `ck-game-testing-discipline` |
| A gameplay symptom you can't attribute ("signal never fires", "request swallowed") | `ck-game-debugging-playbook` |
| WHY requests are deferred / signal binding policies | `ckecs-architecture-contract` §3, §5 |

## 1. The gameplay-AS mental model

Everything goes through **`utils_<feature>` namespaces operating on typed handles**. You never
touch fragments of a framework feature directly; you call its utils. This is mechanical, not style:
a C++ util whose first param matches its class's ScriptMixin target binds as a handle **member
only** — the static `UCk_Utils_X_UE::` spelling does not resolve for it
(`ck-angelscript-interop` §1.2, §1.4; `Script/CLAUDE.md` §5).

```angelscript
auto SelfEntity = ck::ToEntity(this);                       // entity script / actor → handle
auto Timer = utils_timer::Add(InHandle, TimerParams);       // compose a feature → typed handle
if (ck::IsValid(Timer) == false)
{ return; }
```

**`ck::IsValid` before everything.** It is the single most-used call in the corpus — 5,727
call sites of `ck::IsValid`/`ck::Is_NOT_Valid` in BusterBlock gameplay AS (verified 2026-07-03).
Handles go stale when entities are destroyed (destruction is deferred and staged —
`ckecs-domain-reference` §3.4); validate at every callback boundary and after every `TryGet_*`.

### The top surfaces, by real usage

Call-site counts from BusterBlock's hand-written gameplay AS (`Script/`, Generated excluded),
measured 2026-07-03; top rows re-verified by rerunning the counts. Venus (second consumer)
corroborates the same data model, so treat these as generic framework doctrine, not BB-isms.

| # | Surface | Count | What it's for |
|---|---|---|---|
| 1 | `ck::IsValid` / `ck::Is_NOT_Valid` | 5,727 | Handle/object validity — the universal guard |
| 2 | `Request_*` calls | 2,658 | Deferred mutations — the ONLY way state changes (§2.2) |
| 3 | `BindTo_On*` typed signals | 717 | Per-feature event subscription (+152 `Promise_On*` one-shots) |
| 4 | CkStateMachine (`UCk_Sm*`) | 450 | HFSM states/tasks/conditions for stateful gameplay logic |
| 5 | Attributes (integer/byte/float/vector) | 270 | Replicated numeric state with clamping + change signals |
| 6 | `utils_entity_tag` + `utils_entity_tag_query` | 238 | Tag entities for later discovery; query populations |
| 7 | `utils_transform` | 206 | Entity position/rotation (`Get_EntityCurrentTransform`) |
| 8 | `utils_gameplay_tag` | 195 | Resolve/compare gameplay tags at runtime |
| 9 | `utils_unreal_component` | 190 | Reach actor components from entity code |
| 10 | Inventory family (`utils_inventory*`, `utils_lootable_inventory`) | 168 | Items, containers, loot |
| 11 | `utils_net` | 156 | Authority gating (HostOnly checks) — see ck-game-replication-patterns |
| 12 | `utils_entity_script` + `utils_pending_entity_script` | 141 | Spawn entity scripts, get typed script back from a handle |
| 13 | `utils_scene_node` | 120 | Parent/child spatial attachment between entities |
| 14 | `utils_entity_lifetime` | 106 | Destroy entities, lifetime ownership |
| 15 | Iskm/Ism renderer (`utils_iskm_proxy` + assets) | ~100 | Pawn-less instanced-mesh rendering (NPCs without actors) |

Next tier: interaction (`utils_interactable`, channel-based), GOAP (`utils_goap_planner`), grid,
camera, cues (`utils_cue_generic`), tween, timer, probe. A consuming game also authors its **own**
`utils_<gamefeature>` namespaces in the same shape (BusterBlock: `utils_economy`,
`utils_store_driver`, …) — the composer/mixin pattern below is how.

## 2. Idioms — each verified against shipping gameplay code

House style throughout: Allman braces, 4-space indent, `auto`, `== false`, no `b` bool prefix.
`<Prefix>` is your game's class prefix (BusterBlock uses `Bb_`, Venus uses `Vns_`).

### 2.1 Signal binding: `BindTo_On*` vs `Promise_On*`

Two distinct shapes; picking the wrong one is a common review catch.

- **`BindTo_On<Event>`** — recurring subscription to a feature's typed signal. Delegate FIRST;
  binding policy and post-fire behavior are optional trailing params (`Script/CLAUDE.md` §11–12;
  policy semantics: `ckecs-architecture-contract` §5).
- **`Promise_On<Event>`** — one-shot continuation on a pending operation (entity-script spawn,
  replication readiness). Fires once, then the pending handle is done.

Corpus example (BusterBlock) — recurring attribute subscription, and reading the new value from the
**payload** (`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:22-23,96`):

```angelscript
MinutesAttr.BindTo_OnValueChanged(ECk_MinMaxCurrent::Current,
    FCk_Delegate_IntegerAttribute_OnValueChanged(this, n"OnAnyTimeAttrChanged"));

UFUNCTION()
private void OnAnyTimeAttrChanged(FCk_Handle InAttrOwnerEntity, FCk_Payload_IntegerAttribute_OnValueChanged InPayload)
{
    // InPayload carries the post-mutation value — read it here, NOT via a fresh Get_ this frame (§2.2)
}
```

Corpus example (BusterBlock) — one-shot spawn continuation
(`Script/ECS/CandyDealer/BB_CandyDealer_EntityScript.as:380`; pattern spec:
`Script/CLAUDE.md` §10):

```angelscript
auto Pending = utils_entity_script::Request_SpawnEntity(LifetimeOwner, UMyFeature_EntityScript, Params);
utils_pending_entity_script::Promise_OnConstructed(
    Pending, FCk_Delegate_EntityScript_Constructed(this, n"OnFeatureConstructed"));
```

Replication caveat (one line, owned by `ck-game-replication-patterns`): on clients,
`OnConstructed` means *composed*, not *values applied* — read replicated values only after
`Promise_OnReplicationComplete` (`ckecs-architecture-contract` §7).

### 2.2 Request-then-read: mutations are deferred

`Request_*` calls do not mutate immediately — they queue, and a processor consumes them later in
the frame (or on the authority). The WHY — determinism, replication routing, teardown safety — is
`ckecs-architecture-contract` §3 (root doctrine non-negotiable #5). Consumer consequences:

1. **Never `Request_X` then immediately `Get_X` and expect the new value.** Read the result from
   the change-signal **payload** (§2.1), or settle a frame/tick before reading.
2. Read-modify-write goes through the request too. Corpus example (BusterBlock),
   `Script/ECS/WorkoutStation/BB_WorkoutStation_Hfsm.as:115`:

```angelscript
utils_float_attribute::Request_Override_Current(Attr, Attr.Get_FinalValue() + InAmount);
```

3. Attribute values are `float32`, not AS's 64-bit `float` — cast explicitly. Corpus example
   (BusterBlock), `Script/Objectives/BB_Objective_StoreProgress_EntityScript.as:190`:

```angelscript
utils_float_attribute::Request_Override(Attr, float32(Sum), ECk_MinMaxCurrent::Current);
```

4. A request queued on a dying entity is **never consumed** — don't destroy mid-operation; use the
   feature's Cancel verb (`ck-lifecycle-teardown-campaign`, Mechanics primer).

Your own game features follow the same contract: expose `Request_*` functions that enqueue onto a
Requests fragment, drained by your processor (structure: `ck-game-feature-recipe`).

### 2.3 Mixin call forms — and the two traps

Feature accessors are authored as `mixin` functions on the typesafe handle, so consumers call them
member-style. Corpus example (BusterBlock), `Script/ECS/Entryway/BB_Entryway_Utils.as:113-117`:

```angelscript
mixin const FBb_Fragment_Entryway_Params& Get_Params(const FCk_Handle_Entryway& Self)
{
    auto _CkPerfScope = ck::ScopedStat();
    return Self.Get_Fragment(FBb_Fragment_Entryway_Params);
}
// consumer:  auto Params = MyEntryway.Get_Params();
```

**Trap 1 — the namespace-qualified form doesn't exist for mixins.**
`utils_store_driver::Get_EmployeeManager(InDriver)` → "No matching signatures"; the member form
`InDriver.Get_EmployeeManager()` works. Verified fix commit (BusterBlock `ad077a510`). Same rule
for C++ ScriptMixin-bound statics (`ck-angelscript-interop` §1.2, catalog item 6).

**Trap 2 — a non-const `Self&` mixin won't bind on a by-value or const source.** AS reports
"expected T&, got T" (or no matching signature). Copy to a mutable local first. Verified fix
commit (BusterBlock `5c4fd4572`, `Script/Debugger/Pages/Bb_DebugPage_NamedNpc.as:834`):

```angelscript
auto MutableNamed  = Named;   // mixin Self& won't bind a by-value param — copy to a local
const bool Became  = MutableNamed.Add_Loyalty(constants_named_npc::k_LoyaltyPerItem);
```

The same applies to a handle fresh out of `As_<X>()` in an expression — bind it to a local before
calling mutating mixins (BusterBlock fix `48678651a`).

### 2.4 Handles in f-strings need `.ToString()`

A raw handle in an f-string compiles clean and **throws at runtime** ("Invalid type to append to
string") — invisible to every compile gate including `-skipcompile` headless boots
(`Script/CLAUDE.md` §22.1; `ck-angelscript-interop` catalog item 2). Corpus example (BusterBlock),
`Script/ECS/BalloonDarts/BB_BalloonDarts_EntityScript.as:50`:

```angelscript
ck::EnsureIfNot(ck::IsValid(Actor), f"BalloonDarts entity script [{InHandle.ToString()}] has no owner");
```

### 2.5 Fluent `Set_` chains on Params/Request structs

Framework `_ParamsData` and `Request_` structs expose generated `Set_X` setters that return the
struct for chaining — construct with the required args, then `Set_` the optional ones. Never hunt
for a mega-constructor. Corpus example (BusterBlock),
`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:44-46`:

```angelscript
auto TickParams = FCk_Fragment_Timer_ParamsData(FCk_Time(1.0f));
TickParams.Set_StartingState(ECk_Timer_State::Running)
          .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
auto TickTimer = utils_timer::Add(InHandle, TickParams);
```

(Also `Script/ECS/CandyDealer/BB_CandyDealer_EntityScript.as:107-109` for SM params.)

### 2.6 Your own signals: delegate + `_MC` event pair in a Signals fragment

The house shape for game-feature signals — one single-cast `delegate` (the bind parameter type) and
one multicast `event` suffixed `_MC` (the storage), gathered in a `_Signals` fragment, with lazy
`BindTo_`/`UnbindFrom_` mixins. Corpus example (BusterBlock),
`Script/ECS/Entryway/BB_Entryway_Feature.as:106-117` + `BB_Entryway_Utils.as:141-156`:

```angelscript
delegate void F<Prefix>_Delegate_MyFeature_OnThing(FCk_Handle_MyFeature InFeature, FCk_Handle InEntity);
event    void F<Prefix>_Delegate_MyFeature_OnThing_MC(FCk_Handle_MyFeature InFeature, FCk_Handle InEntity);

USTRUCT()
struct F<Prefix>_Fragment_MyFeature_Signals
{
    F<Prefix>_Delegate_MyFeature_OnThing_MC OnThing;
}

mixin void BindTo_OnThing(FCk_Handle_MyFeature& Self, F<Prefix>_Delegate_MyFeature_OnThing InDelegate)
{
    auto& Fragment = Self.AddOrGet_Fragment(F<Prefix>_Fragment_MyFeature_Signals);   // lazy — added on first bind
    Fragment.OnThing.AddUFunction(InDelegate.GetUObject(), InDelegate.GetFunctionName());
}
```

`UnbindFrom_*` checks `Has_Fragment` first and returns quietly if absent (same file, :148-156).
Venus uses the identical shape — this is generic doctrine.

### 2.7 Tags-as-state: empty USTRUCTs gate processor passes

State that is *presence*, not *data*, is an empty struct added/removed as a fragment; processors
declare it as their dirty trigger. Corpus example (BusterBlock),
`Script/ECS/Entryway/BB_Entryway_Feature.as:119` and
`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:1-12,71`:

```angelscript
struct FBb_Tag_DayCycle_NeedsSetup {}                       // in the Feature file

class UBb_Processor_DayCycle_Setup : UCk_Processor_Script_Base_UE
{
    default _Group = n"FGroup_Gameplay_Script";
    default _MarkedDirtyBy = FBb_Tag_DayCycle_NeedsSetup;   // pass runs only while the tag exists

    UFUNCTION(BlueprintOverride)
    void Tick(FCk_Time DeltaSeconds)
    {
        _Handle.ForEach_EntityWithTwoFragments(
            FBb_Tag_DayCycle_NeedsSetup, FBb_Feature_DayCycle,
            FCk_DynamicFragment_ForEachEntity(this, n"ForEach_Setup"));
    }
    // ForEach_Setup ends with: InHandle.Request_TryRemove(FBb_Tag_DayCycle_NeedsSetup);
}
```

Discovery uses the sibling mechanism `utils_entity_tag::Add(Handle, n"TAG_<Prefix>MyFeature")` so
owners/drivers can find instances later (`ck-game-driver-architecture` owns discovery + delta-gating;
warning: an `EntityTagQuery` continuous-update fires **every frame** — delta-gate on
`_Added`/`_Removed` or you rebuild the population per frame).

### 2.8 `assets::` accessors for Blueprint/content references

Never hardcode content paths in gameplay AS. An editor-side registry config
(`UCkAssetRegistryConfig`, `Script/CLAUDE.md` §13) generates typed accessors into
`Script/Generated/<Game>Assets.as`; gameplay code calls them (3,114 call sites in BusterBlock).
Corpus example (BusterBlock), `Script/UI/InfoWidget/BB_InfoWidgetContainer_Widget.as:63`:

```angelscript
if (ContentClass == nullptr)
{ ContentClass = assets::load::InfoWidgetContent_BB_WBP_Class(); }
```

Blueprints stay data/visual shells (widget trees, meshes, spawn-params); logic lives in AS. Only 3
`BlueprintImplementableEvent`s exist in all of BusterBlock's C++ — BP is not a logic tier here.

### 2.9 Per-function perf scope

Every utils/processor function body in the corpus opens with:

```angelscript
auto _CkPerfScope = ck::ScopedStat();
```

This feeds `stat CkScript` for free (`ck-performance-and-analysis` §1.4). Adopt `ck::ScopedStat()`
project-wide or not at all; the corpus does — it is what makes the drop-to-C++ decision measurable
later (§5). `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md A6]`

## 3. The hot-reload workflow

The Hazelight fork hot-reloads `.as` on save. The loop:

1. Save the file.
2. The compile verdict lands in the editor log in **~2 seconds**. Do not arm sleep-loop watchers —
   check immediately (`ck-angelscript-interop` catalog item 13).
3. **Read the fresh log before claiming anything works.** Grep
   `<Project>/Saved/Logs/<Project>.log` for `Angelscript: Error` and `Warning:` lines naming your
   file. This is a standing rule: never report an AS change done without reading the log — a parse
   error anywhere in a file silently kills **every class in that file** (placed instances fall back
   to native base classes, `default X = ...` references resolve to nothing), so downstream symptoms
   look like gameplay bugs, not compile errors.

```powershell
# PowerShell — check the verdict after a save (log rotates per editor boot)
Select-String -Path "$env:PROJECT_ROOT\Saved\Logs\<Project>.log" -Pattern 'Angelscript: (Error|Warning)' | Select-Object -Last 20
```

### What reloads vs what needs more

| Change | Cycle |
|---|---|
| Edit existing `.as` (function bodies, new classes, new properties) | Save → ~2s hot reload → read log |
| New dynamic-handle type (`asset <X>Handle of UCkDynamic_HandleDefinition`) | Land declaration + marker struct + first consumer in ONE edit, then **boot the editor** — self-heal converges in one boot (first-pass "not a data type" errors are expected transients). Live alternative: the `ForceRefreshDynamicHandleBindings()` editor button. Full cycle: `Script/CLAUDE.md` §7 |
| New EntityScript class or changed `ExposeOnSpawn` set | The `Params()` accessor is emitted by a **post-compile** generator — first pass self-heals with a stub, real accessor next pass (`ck-angelscript-interop` catalog item 9) |
| New/changed C++ (including a new ScriptMixin util) | Close the editor → build C++ → boot; the AS wrapper regenerates at boot, and only then does the AS callsite compile (`ck-angelscript-interop` catalog item 6) |

### `Script/Generated/` — hands off

Generated wrappers, spawn-params accessors, and `DynamicHandleTypes.json` live in
`Script/Generated/`. Never blanket-delete or bulk-touch it (mtime-based reload sweep freezes the
editor), never revert `*_AutoTestActors.as` alone (phantom-namespace trap), and expect a second
editor/headless instance of the same project to be a read-only secondary that writes nothing there.
All owned by `ck-angelscript-interop` (catalog items 3, 7, 8 + §3 self-heal/regen internals) — go
there the moment generated files look wrong.

### Headless compile gate — and its blind spot

A headless editor boot is the CI-shaped compile check (exact commands: `ck-angelscript-interop` §4;
environment shapes: `ck-build-and-env` §4). Know its limit: a **clean compile proves nothing about
runtime-only failures** — the f-string handle throw (§2.4) and anything gated on PIE-start only
surface when the line executes. Gate real claims on an executed path (PIE or an autotest —
`ck-game-testing-discipline`).

## 4. Silent failure modes — the gameplay-facing catalogue

The binding-machinery versions of these live in `ck-angelscript-interop` §2 (13 items). Below is
the consumer's seat: what YOU see while writing gameplay, each verified against a real incident.

**4.1 "My mutation didn't happen."**
Symptom: `Request_X` then `Get_X` same frame returns the old value; no error anywhere.
Cause: requests are deferred by contract (`ckecs-architecture-contract` §3).
Fix: read from the change-signal payload or settle a frame (§2.2). If the value *never* changes,
check the entity isn't mid-destruction (requests on dying entities are never consumed) and, in MP,
that you're on the authority (`ck-game-replication-patterns`).

**4.2 "The utils function exists but doesn't resolve."**
Symptom: "No matching signatures" on a namespace-qualified call you can see defined.
Cause: it's a mixin (member-call only), or a non-const `Self&` mixin against a by-value/const
source, or a brand-new C++ util whose wrapper hasn't regenerated yet.
Fix: member form on a mutable local (§2.3); for new C++, build → boot → then compile the AS
(`ck-angelscript-interop` §1.2, item 6). Verified incidents: BusterBlock `ad077a510`, `5c4fd4572`.

**4.3 The ExposeOnSpawn positional-arg freeze.**
Symptom: after touching an EntityScript's `ExposeOnSpawn` properties, the generated `Params(...)`
call no longer matches callers — worst case the editor loops self-heal recoveries at every launch
(BusterBlock incident `01a39b58f`: declaration order diverged from caller arg order → self-heal
synthesized parallel overloads each cycle, ~724 recoveries per session, launch never converged).
Cause: the generated `Params()` signature is **positional in declaration order**. Reordering,
inserting mid-list, or exposing a new field changes the signature for every existing caller.
Fix: append new `ExposeOnSpawn` fields at the end; prefer non-exposed + auto-discover for fields
callers shouldn't pass (BusterBlock `8c7cc4f07` did exactly this to keep `Params()` stable). Treat
an EntityScript's ExposeOnSpawn list as a frozen public API.

**4.4 Runtime-only failures a green compile hides.**
Symptom: clean editor/headless boot, then a throw at PIE-start or when a code path first runs.
Canonical instance: raw handle in an f-string (§2.4). Same class: anything inside a callback that
only executes under real gameplay.
Fix: exercise the path — PIE it or autotest it — before claiming done. A `-skipcompile` boot is a
*compile* gate only.

**4.5 CDO `default` statement gotchas.**
- `default` works on class-level UPROPERTYs only; inside `asset ... of ...` blocks it does not
  compile — assign fields directly (`Script/CLAUDE.md` §13).
- EntityScripts **replicate by default**: `_Replication = ECk_Replication::Replicates` is the C++
  base default (`Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript.h:114`, verified
  2026-07-03). Omitting the `default` line does NOT give you a non-replicating script — spawning
  one under a non-replicated lifetime owner is the classic silent-flood trap
  (`ck-game-replication-patterns` owns the fix: ActorRelay channel owners).
- A parse error anywhere in the file makes every `default X = SomeClass` in it resolve to nothing,
  with symptoms far downstream (§3 step 3).

**4.6 Brand-new dynamic-handle types: first-pass errors are not failures.**
Symptom: adding `FCk_Handle_<X>` produces a wall of "not a data type" + "Hot reload failed" on the
next boot.
Cause: registration lag by design — self-heal writes a stub, the deferred regen writes the real
JSON entry, the recompile goes clean, all in one boot.
Fix: nothing — boot once and gate on the *post-regen* clean reload + the JSON entry. A **second**
boot still red is a real problem (`Script/CLAUDE.md` §7; `ck-angelscript-interop` item 1).

**4.7 Tag-query handlers firing every frame.**
Symptom: a discovery/driver handler shows up hot in `stat CkScript`; frame cost grows with entity
count.
Cause: `EntityTagQuery` continuous-update fires every evaluate pass by design (tested contract),
not only on change.
Fix: delta-gate on the payload's `_Added`/`_Removed` before rebuilding anything
(`ck-game-driver-architecture` owns the full pattern). Verified incident: BusterBlock `531b0c956`
(~250ms/frame before delta-gating).

## 5. What belongs in AS vs C++

**The ruling (maintainer, 2026-07-03, verbatim):** features drop to C++ "rarely, unless it's
_very_ clear it's going to be a perf concern". AS-first is the standard for all consumer gameplay.

**Corpus proof AS suffices.** BusterBlock ships a full store-management game with 135 AS
processors and 592 AS entity scripts, and **zero** game-defined C++ fragments, processors, or
requests (verified 2026-07-03: `rg -c 'CK_REGISTER_PROCESSOR|struct FFragment_'` over
`Source/BusterBlock` → only comment/read references). Venus is even more AS-pure. Last 200 BB
commits: 2,536 `.as` files changed vs 22 `.cpp`.

**What the rare C++ actually is** — engine-boundary glue, not gameplay (verified in
`Source/BusterBlock`, 16 files total):

| C++ | Why it can't be AS |
|---|---|
| `AI/Navigation/BB_NavQueryFilters.{h,cpp}` | Custom navmesh query filters — engine nav API surface |
| `SaveLoad/` (7 files: probes, restore, snapshot commands) | Save/load bridging into the snapshot system; reads framework fragments like `FFragment_SaveKey` |
| `Character/BbCharacter` + `BbCharacterMovementComponent` | The one player ACharacter + CMC tuning — engine movement internals |
| Module boilerplate + editor maintenance commandlets | Not gameplay |

(Venus's equivalents: a `CkNavigation` module and a custom projectile renderer module — same
category.) Traditional-Unreal plugin code (UMG/Slate minigames) also stays C++, but that's
pre-ECS legacy, not a pattern to copy.

**How to drop to C++ when you genuinely must:**

1. **Measure first.** No C++ port on a hunch — capture `stat CkScript` / `stat CkProcessors`
   numbers and follow the benchmark-claim discipline in `ck-performance-and-analysis` (§2: no
   perf claim without a recorded baseline; §3.1: the drill-down ladder attributes cost to the
   specific AS listener/scope). The `ck::ScopedStat()` habit (§2.9) is what makes this a
   ten-minute check instead of a profiling project.
2. **Author the C++ feature the house way.** A game C++ module inherits `CkModuleRules`
   (`ck-build-and-env`), and a fragment/processor/request/signal quartet follows the
   `ck-macros-and-codegen` §3 add-a-new-X checklists verbatim. Name any AS-exposed function
   library `U<Prefix>_Utils_<X>_UE` — the `_UE` suffix survives the AS namespace strip
   (`Script/CLAUDE.md` §16.1).
3. **Keep the AS surface.** The C++ drop is an implementation move; consumers should still call
   `utils_<feature>::` — build order caveat in §4.2 applies to the first compile after.

Decision heuristic: per-frame work over large populations (hundreds+) with measured cost →
candidate. Event-driven logic, composition, UI, AI decision layers → AS, always.

## 6. Style — the gameplay-code habits

Owner of all style/naming rules: root doctrine `Plugins/CkFoundation/CLAUDE.md` ("Code style") and
`Plugins/CkFoundation/Script/CLAUDE.md` (AS specifics). Observed uniformly across the gameplay
corpus and expected in review — the delta worth internalizing:

- **Allman braces, 4-space indent**, single-statement guards on one line:
  `{ return FCk_Handle_Entryway(); }` (`BB_Entryway_Utils.as:17`).
- **Minimal comments** — but *contract* comments are first-class: discovery contracts, coordinate
  conventions, and semantic warnings live as paragraph comments next to the code they govern
  (`BB_Entryway_Utils.as:24-27` documents the StoreDriver discovery contract at the tag-add site).
- **Naming**: `<Prefix>_` on every game class/struct (`UBb_...`, `FBb_Fragment_...`,
  `FBb_Tag_...`, `FVns_...`); `utils_<feature>` namespaces; `constants_<feature>` namespaces with
  `k_`-prefixed constants (BB-uniform; unconfirmed in the second consumer);
  `TAG_<Prefix><Feature>` entity tags; `Get_`/`TryGet_`/`Request_`
  function prefixes; no `b` bool prefix.
- **`ck::EnsureIfNot` for contract violations** — guard composers with it and return an invalid
  handle; never log-and-continue (`BB_Entryway_Utils.as:13-17`; doctrine:
  `ck-change-control`, non-negotiable 2).
- **`ck::ScopedStat()` first line** of every utils/processor function — if your project adopts
  the convention at all; project-wide or not at all (§2.9, ADJUDICATIONS A6).

## Common mistakes

| Mistake | Reality |
|---|---|
| `Request_X` then `Get_X` same frame | Deferred — read the signal payload or settle a tick (§2.2) |
| `utils_x::SomeMixin(Handle)` | Mixins are member-call only: `Handle.SomeMixin()` on a mutable local (§2.3) |
| `f"{SomeHandle}"` | Runtime throw; `.ToString()` — and a green compile won't catch it (§2.4) |
| Reordering / mid-inserting `ExposeOnSpawn` fields | Positional `Params()` signature breaks every caller; append-only (§4.3) |
| Claiming done off a clean save without reading the log | Parse error = every class in the file silently dead (§3) |
| Panicking at first-boot "not a data type" walls after adding a handle type | Expected self-heal transient; second boot red = real (§4.6) |
| Porting a "slow" feature to C++ on feel | Measure first — `ck-performance-and-analysis` §2; the corpus ships whole games in AS (§5) |
| Calling `UCk_Utils_X_UE::` directly from gameplay AS | Always `utils_*` — the static form doesn't even resolve for mixin-bound functions (§1) |
| Callback body without `ck::IsValid` guards | Handles outlive entities; validate at every callback boundary (§1) |
| Skipping the payload param and re-Getting state in a signal handler | The payload IS the post-change value; a fresh Get_ races the pump (§2.1) |

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock HEAD `52a75e13d` (submodule CkFoundation as pinned there)
and the Phase-0 usage census, with all load-bearing claims re-verified by the commands below.
Corpus examples are BusterBlock (`Bb_`) with Venus (`Vns_`) corroboration where noted; patterns
shown are generic framework doctrine unless labeled otherwise.

Re-verify volatile facts (Git Bash, from the consuming game's repo root — the agent Grep/Glob
tools are blind under `Script/` when a repo-root `.ignore` hides it; always `rg --no-ignore`):

```bash
# Idiom frequency (top-3 + processors/entity scripts)
rg --no-ignore -o 'ck::IsValid|ck::Is_NOT_Valid' Script -g '*.as' -g '!**/Generated/**' | wc -l   # 5,727
rg --no-ignore -o 'Request_\w+'  Script -g '*.as' -g '!**/Generated/**' | wc -l                   # 2,658
rg --no-ignore -o 'BindTo_On\w+' Script -g '*.as' -g '!**/Generated/**' | wc -l                   # 717
rg --no-ignore -o ': UCk_Processor_Script_Base_UE' Script -g '*.as' -g '!**/Generated/**' | wc -l # 135
rg --no-ignore -o 'class \w+ : U\w*EntityScript\w*' Script -g '*.as' -g '!**/Generated/**' | wc -l # 592
# Game C++ stays glue-only (expect no processor/fragment definitions)
rg -n 'CK_REGISTER_PROCESSOR|struct FFragment_' Source/<Game>
# EntityScript replication default (expect Replicates)
rg -n 'ECk_Replication _Replication' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript.h
# Incident commits cited in §2.3/§4.3 (BusterBlock)
git log --oneline ad077a510 5c4fd4572 01a39b58f 8c7cc4f07 48678651a --no-walk
```

If a count drifts by an order of magnitude, or `Source/<Game>` grows real fragments/processors,
re-open §1's table and §5's ruling against the maintainer before trusting this skill.
