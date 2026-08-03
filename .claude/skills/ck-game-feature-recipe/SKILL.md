---
name: ck-game-feature-recipe
description: 'Use when adding a CkFoundation gameplay feature and need the standard AngelScript file set, composition order, discovery contract, processors, spawn vehicle, and tests.'
---

# The gameplay-feature recipe — from idea to placed, tested feature

This is THE canonical path for adding a gameplay feature to a CkFoundation game. A **feature**
here means a reusable gameplay capability composed onto entities — a door, a lootable bin, a
turnstile, an employee — expressed as fragments + a utils composer + processors, and delivered
into the world by an EntityScript. (Jargon — Entity/Fragment/Processor/Handle/Request/Signal —
is defined once in the framework root doctrine's Lingo table: `Plugins/CkFoundation/CLAUDE.md`.)

Two settled rulings frame everything below (maintainer, 2026-07-03):

1. **AngelScript-first is the standard.** Consumer gameplay features are written in AS under
   `Script/ECS/<Feature>/`. Drop to C++ only when it is *very* clear the feature is a perf
   concern. Corpus evidence: BusterBlock is ~186k lines of hand-written game AS vs <3% C++, with
   **zero** game-defined C++ fragments or processors — all 135 processors and 592 entity scripts
   are AS (usage census, verified 2026-07-03).
2. **No Actors.** The EntityScript is the placeable unit (~95% Actor replacement). Do not author
   `AActor` subclasses for features; do not copy Venus's Actor-tick processors (old-era).

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| AS language rules, hot-reload loop, silent AS failure modes | `ck-game-angelscript-gameplay` + `Plugins/CkFoundation/Script/CLAUDE.md` |
| Choose an entity archetype / parent-vs-child / lifetime owner in depth | `ck-game-entity-composition-patterns` |
| Wire the feature into a world-owner ("driver") and its discovery | `ck-game-driver-architecture` |
| Make the feature replicate correctly / spawn replicated entities | `ck-game-replication-patterns` |
| Write/run the AutoTests the arc requires | `ck-game-testing-discipline` |
| Fix `DynamicHandleTypes.json` / "not a data type" breakage | `ck-angelscript-interop` (framework skill) |
| Add C++ fragments/processors to CkFoundation itself | `ck-macros-and-codegen` + `ck-game-framework-boundary` |
| A "feature" that is pure UI, a one-shot cue, or a behavior node | §8 routing table below |

---

## 1. Before writing any code

**Step 0a — read a neighboring feature in full.** Root doctrine non-negotiable #1
(`Plugins/CkFoundation/CLAUDE.md`, "Non-negotiables"): *mimicry of adjacent code beats invention;
if your change doesn't resemble the code around it, you have not researched enough.* Pick the
existing feature in YOUR game closest in shape to what you're building and read its whole dir.
If your game is new and has none, read the framework's `CkTimer` quartet plus the corpus example
in §7. Corpus reference features (BusterBlock, verified 2026-07-03): `Script/ECS/Entryway/`
(minimal, fresh, child-entity composition), `Script/ECS/Trashcan/` (requests + composed framework
features + widget), `Script/ECS/Door/` (a feature that *composes another feature*).

**Step 0b — decide the entity composition.** Answer these before the first file exists
(full decision detail: `ck-game-entity-composition-patterns`; replication detail:
`ck-game-replication-patterns`):

| Question | Typical answer | Notes |
|---|---|---|
| One entity or parent + children? | One entity unless a part needs its own transform/probe/context | Entryway = 1 feature entity + 2 child Trigger entities; Trashcan = 1 entity, framework features composed on it. Features that internally call `Request_OverrideToSelf` (Interactable) MUST live on a child node — never re-root a shared entity. |
| Does it replicate? | Placed world features: yes. Local/cosmetic: no | `default _Replication = ECk_Replication::Replicates` on the EntityScript; the utils `Add` takes an `ECk_Replication` param defaulting to `Replicates` so tests can compose `DoesNotReplicate`. |
| Who owns its lifetime? | The entity it's composed onto | Replicated *spawns* (not compositions) need a replicated lifetime owner — see `ck-game-replication-patterns`. |
| Who discovers it? | A driver, via an entity tag | Read §4 before you write the tag. This is where features die. |

**Step 0c — write docs only if designer-ambiguous.** The doc tier is proportional to ambiguity,
not mandatory: BusterBlock's Employee feature got a design spec + implementation plan as two
commits before any code (`5df283d37`, `e9c388b90`); Entryway and Trashcan shipped with none —
their contract comments live in the code (all verified via `git show`).

---

## 2. Anatomy — what a finished feature looks like

The four-part data model below (dynamic handle + fragment split + utils composer + EntityScript
spawn vehicle) is **confirmed generic** CkFoundation-consumer doctrine — it appears essentially
unchanged in both corpus games (BusterBlock `Bb_`, Venus `Vns_`; Venus recon 2026-07-03). The
directory layout (everything incl. the EntityScript under `Script/ECS/<Feature>/`) and
per-concern processor files are the BusterBlock refinement, adopted here as the standard.

`<Prefix>` = your project prefix (BusterBlock uses `Bb`, Venus uses `Vns`). Files live in
`Script/ECS/<Feature>/`:

| File | Role | Rules that matter |
|---|---|---|
| `<Prefix>_<Feature>_Feature.as` | Data contract: dynamic-handle asset, marker struct, Params/Current(State)/Requests/Signals fragments, dirty tags, enums, constants, gameplay tags | Params is **gameplay-only** — no meshes, no widgets-as-behavior. Heavy contract comments live here. |
| `<Prefix>_<Feature>_Utils.as` | `namespace utils_<feature>` with the single `Add(...)` composer + `mixin` accessors/request-writers/signal binders on the typesafe handle | The ONLY public API surface of the feature. |
| `<Prefix>_<Feature>_Processor_<Concern>.as` | One class per concern (`_Setup`, `_HandleRequest`, ...), subclassing `UCk_Processor_Script_Base_UE`, driven by `_MarkedDirtyBy` | Never a ticking Actor (Venus old-era). |
| `<Prefix>_<Feature>_EntityScript.as` | The placeable "spawn vehicle": transform + `utils_<feature>::Add` + **visuals** | Visuals live HERE, never in the feature — so tests compose the feature with zero asset dependencies. |
| `<Prefix>_<Feature>_Assets.as` *(optional)* | `asset ... of ...` definitions; thin EntityScript subclasses that only set defaults (variants) | Only for heavyweight asset definitions — tags alone don't warrant it. |
| `<Prefix>_<Feature>_DevViz.as` + `_Processor_DevViz_Setup.as` *(optional)* | Opt-in dev visualization for art-less placeables | Default off in Params. |
| `<FEATURE>_INTERFACE.md` *(optional)* | Consumer-facing interface doc when the feature is a service for other features | `[SINGLE-EXEMPLAR]` — one instance in the corpus (`Script/ECS/EmployeeManager/EMPLOYEE_INTERFACE.md`, commit `665fa8e17`). |

Tests live in the game's test plugin, one scenario per file:
`<TestPlugin>/Script/Tests/<Feature>/<Prefix>_AutoTest_<Feature>_<Scenario>.as` (mechanics:
`ck-game-testing-discipline`).

Per-file skeletons with real-shaped code are in the worked example, §7. Framework-side rationale
for the fragment split and the deferred-request contract: `ckecs-architecture-contract`.

---

## 3. The ordered arc — build it in this sequence

Reconstructed from BusterBlock git history; every hash below verified via `git show --stat`
2026-07-03. The Employee feature (`5df283d37` → `cdf71b36a`, ~30 commits) is the textbook
milestone-sliced arc; Entryway (`582bddd26` → `345903a10`, 7 commits over 2 days) is the compact
form; Trashcan (`4713da549`) shows the whole thing can land big-bang in one commit *with* its
3 AutoTests and Content — the ORDER inside is the same either way.

1. **(If designer-ambiguous) design spec, then implementation plan** — own commits
   (`5df283d37`, `e9c388b90`).
2. **Designer data first**: enums + definition assets (`9efb7320a` — roster/enum before any
   behavior).
3. **`_Feature.as`** — handle asset + marker + fragments + tags, no behavior yet (`651da51ea`).
   Boot the editor once so the dynamic-handle registry regenerates, and **commit the regenerated
   `Script/Generated/DynamicHandleTypes.json` with the feature** (Entryway did:
   `582bddd26` includes the JSON). The first-boot "'FCk_Handle_X' is not a data type" errors are
   *expected transients* of the self-heal cycle — gate on the post-regen clean reload, not
   first-pass silence. Full regen mechanics and failure modes: `ck-angelscript-interop`.
4. **Pure logic + its unit AutoTest, SAME COMMIT** (`0ddfada98` — a plain-namespace evaluator and
   its test landed together). Feature-with-test is the house norm: not test-first, not
   test-after. *(Test culture is BusterBlock-only in the corpus — Venus has zero tests — adopted
   here as the standard.)*
5. **`_Utils.as` composer + compose AutoTest, same commit** (`c27dfc88a`). The compose test
   asserts the invariants `Add` promises (children exist, defaults hold, no foreign fragments).
6. **Processors + EntityScript spawn vehicle** (`665fa8e17` lands feature/utils/processors/
   entityscript as a unit for the manager tier; Entryway's `582bddd26` authored all six files at
   once). Visuals go in the EntityScript only.
7. **Driver integration + routed end-to-end AutoTest** (`01300d524` — StoreDriver spawns the
   manager, a routed wage-debit test proves the whole path). Migrate existing gym/test callers in
   a `test:` commit (`1b631f8a0`). Driver-side wiring patterns: `ck-game-driver-architecture`.
8. **Generated footprints as separate `chore(...)` commits** — test wrapper actors, handle
   registry, autotest-map external actors (`ae18a830b`, repeated per milestone). Keeping codegen
   out of hand-written diffs keeps review sane.
9. **Debug tooling** — debugger page, DevViz (`214a3c02b`).
10. **Placement Content LAST** — the placement Blueprint + spawn-params asset + level placements
    (`cdf71b36a` for Employee; `345903a10` for Entryway: `Entryway_BB_BP.uasset` +
    `EntitySpawnParams_Entryway_BB_BP.uasset` + gym placements, nothing else). Content lands
    last because everything before it is testable headless with zero assets.
    *(EntitySpawnParams placement assets are BusterBlock-only in the corpus —
    `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md A4]`; interim
    stance: EntityScript spawn params + CkProvider, which this flow matches.)*

Then **budget for the post-ship fix wave** — in the corpus it lands overwhelmingly in two
categories: discovery/composition timing (§4) and multiplayer-client divergence
(`ck-game-replication-patterns`).

---

## 4. THE LOUDEST WARNING — discovery/composition timing

**This is the #1 failure mode of models and engineers working on CkFoundation games**
(maintainer ruling, 2026-07-03). Read this section twice. Every rule below is backed by a real
production incident, verified from the fix commit.

The physics of the problem: **entities compose asynchronously and late.**
EntitySpawnParams-placed features compose their ECS entity async from a placer actor's
BeginPlay; driver subordinates spawn after the driver constructs; replicated entities finish
even later on clients; and `utils_entity_tag::Add` is itself a **deferred** operation. Any code
that scans for entities once at construct time is betting that everything it needs already
exists and is fully built. That bet loses.

### The incidents (all verified via `git show`, 2026-07-03)

- **`66ac804db` — one-shot construct-time scan missed late-composing entities.** The
  StoreDriver's DoConstruct tag scan raced placed features whose entity composes async from the
  placer's BeginPlay. A framework-side speedup *exposed* the latent race: managers placed in the
  level were silently never bound — orders deducted money but no delivery truck ever spawned.
  Fix: re-run the binders on a deferred rescan with per-binder dedupe.
- **`1ca589b0d` — discovery tag stamped before composition finished.** A gondola stamped its
  discovery tag at the TOP of DoConstruct but only became a valid RetailGondola after its async
  shelf children finished. Discovery satisfied on the tag's mere presence, cast
  `As_RetailGondola` in the same breath, and **silently dropped** the half-built entity. The
  driver went Ready with 0 gondolas; NPCs roamed instead of shopping. Fix: stamp the tag at
  finalize (LAST), gate on expected minimums, and ensure on every tag→feature cast so recurrence
  fails loudly.
- **`287ee6601` — unscoped registry-wide discovery claimed foreign entities.** The door scan was
  registry-wide, so on a multi-building map the driver bound EVERY door in the level — 94% of
  "customer entered" events came from other buildings, and NPCs walked to the wrong building's
  doors. Fix: prune discovered entities to the store footprint before stamping Ready.

### The rules

1. **Stamp the discovery tag only when the entity is bindable.** If composition is fully
   synchronous inside `utils_<feature>::Add`, stamping inside `Add` is safe (Entryway does,
   `Script/ECS/Entryway/BB_Entryway_Utils.as:24-28`, with the discovery contract written in a
   comment right there). If the EntityScript defers construction (`ConstructionFlow::Continue`)
   or waits on async children, stamp the tag in the finalize step — the tag is a **promise that
   the feature contract holds**, not a birth announcement.
2. **Never read-once at init. Poll or bind.** Consumers discover via a persistent
   `CkEntityTagQuery` (delta-gated — it re-fires with the full match set every evaluate pass, so
   dedupe on `Get_Added()` or against already-bound state), or via a readiness promise from the
   owner. Driver-side mechanics (delta-gating, minimums contracts, readiness stamping) are owned
   by `ck-game-driver-architecture`; client-side readiness (rep-notify bind order,
   consume-after-bind) by `ck-game-replication-patterns`.
3. **Ensure on every tag→feature cast in discovery code.** A tag without the feature fragment is
   a composition-order bug — fail loudly (`ck::EnsureIfNot`), never skip silently
   (`1ca589b0d` made this mandatory after the silent-drop incident).
4. **Scope your scans.** Registry-wide tag discovery grabs every instance in the world,
   including other buildings'/arenas'/players' copies. Scope by explicit reference, radius, or
   per-scope tags before acting on matches (`287ee6601`).
5. **Assume your test spawns race too.** An AutoTest that composes a feature and immediately
   asserts on tag-discovered state hits the same physics — settle a tick first
   (`de3099e1c`: "Composition read TAG_BbCheckoutCounter in the same tick it was added, but
   utils_entity_tag::Add is deferred").

The positive pattern, in one sentence: **producers stamp the tag last; consumers bind
signal-driven or delta-gated queries and gate behavior on an explicit readiness state — nobody
scans once and trusts the result.**

---

## 5. Deferred-request discipline — mutations land next tick

Root doctrine non-negotiable #5: *utility functions enqueue; processors mutate.* The full
contract (why requests are deferred, pump passes, what "next tick" precisely means) is
`ckecs-architecture-contract` §3 — cite it, don't re-derive it. What the feature author must
internalize:

- `Request_*` (yours and the framework's — attribute mutations, inventory ops, entity-tag adds,
  entity destruction) **does not take effect in the calling scope.** Reading the value back on
  the same tick returns the pre-request state.
- **Corpus incident** `de3099e1c` (verified): a compose test read an entity tag in the same tick
  it was added and asserted zero matches; fix was deferring the read past a settle frame. The
  same commit fixed a signal-vs-request ordering bug in an interaction channel — three of three
  failures in that commit trace to same-tick assumptions.
- Consume results through the feature's **signals** (the processor broadcasts after mutating) or,
  in tests, settle a tick before asserting (`ck-game-testing-discipline` owns the settle
  helpers).
- In your own request processor: **clear the request fragment BEFORE broadcasting**, so a
  re-entrant `Request_*` from a signal handler survives instead of being wiped
  (`Script/ECS/Trashcan/BB_Trashcan_Processor_Requests.as:28-29`, verified).

---

## 6. Definition of done

A feature is done when ALL of these hold — not when the code compiles:

1. **Three AutoTests green headless**: unit (pure logic, if any), compose (`Add` invariants),
   and end-to-end (routed through the driver/owner). Run them headless and read the real result
   artifact — invocation and evidence standards: `ck-game-testing-discipline`.
2. **AS compile log clean.** Never claim done without reading the fresh editor log — grep it for
   `Angelscript: Error` / warnings naming your files. A green-looking editor that kept old
   script code ("Hot reload failed ... Keeping all old script code") is running your PREVIOUS
   code. (Rule and log locations: `Plugins/CkFoundation/Script/CLAUDE.md` §22 and your project's
   CLAUDE.md verification rules.)
3. **`DynamicHandleTypes.json` regenerated and committed** with the feature commit (§3 step 3).
4. **Signals unbound on teardown.** Every cross-entity `BindTo_On*` has a `DoEndPlay` /
   teardown `UnbindFrom_On*`, and owned ephemeral children are destroyed — the corpus's
   fix-wave category 4 is entirely stale bindings and leaked children
   (`ck-game-entity-composition-patterns` owns the lifetime rules).
5. **Visuals verified in-editor**: anything only observable in PIE (DevViz, meshes, widgets,
   placement orientation) is `[EDITOR-VERIFY]` — state the exact manual steps (which map, which
   placement, what to look at) rather than claiming it works.
6. **If you added ANY C++ API** (rare — see ruling #1): it must work and be verified in C++,
   Blueprint, AND AngelScript. Root doctrine non-negotiable #4: "works in C++" is one third of
   done.
7. **Generated footprints committed** as their own `chore(...)` commits (§3 step 8) — missing
   wrapper regen makes tests silently vanish from the runner.

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Worked example — a complete minimal feature | `references/worked-example.md` |

## 8. When a "feature" doesn't fit this recipe

Not everything is a fragment quartet. Route by what you're actually building (corpus-grounded,
usage census 2026-07-03):

| You're building | Route | Corpus grounding |
|---|---|---|
| A UI panel/HUD element | AS `UUserWidget`-family subclass in `Script/UI/` + a WBP visual shell; no feature dir | 135 such classes in BusterBlock; logic in AS, visuals in the WBP |
| A one-shot VFX/SFX/gameplay cue | `UCk_GenericCue_EntityScript` in `Script/Cues/` | 37 cue subclasses |
| A behavior node (SM state/task, GOAP action) | `UCk_SmState_/SmTask_/GoapAction_EntityScript` under the owning feature's dir — entities, but not quartet features | 326 SM-node + 32 GOAP subclasses |
| Pure functions (math, formatting, parsing) | Plain namespace library, no fragments, no handle | Employee's shift evaluator (`0ddfada98`) before it grew fragments |
| A world-singleton service that owns/coordinates others | That's a **driver** — `ck-game-driver-architecture` | StoreDriver + 11 subordinates |
| A new framework-level capability (needs C++ fragments) | Framework change: `ck-macros-and-codegen`, gated by `ck-game-framework-boundary` | BusterBlock defines zero — strong prior you don't need it |

If it has designer-tunable state on world entities, other systems reacting to it, or a
request/mutation surface — it's a quartet feature; use the recipe.

## Common mistakes

- **Visuals or asset refs in the feature Params** → tests now need assets; headless composition
  breaks. Visuals belong to the EntityScript (§2; stated in-source at
  `Script/ECS/Trashcan/BB_Trashcan_Feature.as:26-29`).
- **Discovery tag stamped at the top of a deferred construction** → consumers bind half-built
  entities and silently drop them (`1ca589b0d`). Tag last.
- **Construct-time one-shot scan for other entities** → misses everything that composes later
  (`66ac804db`). Poll/bind, delta-gated.
- **Reading back state the same tick as a `Request_*`** → stale read (`de3099e1c`). Consume via
  signal or settle a tick.
- **Treating first-boot "'FCk_Handle_X' is not a data type" as your bug** → expected self-heal
  transient; gate on the post-regen clean reload (`ck-angelscript-interop`).
- **Forgetting the mixin call form** — a utils function whose first param is the handle binds as
  a handle *member*; the static `UCk_Utils_X::` form may not resolve (`ck-game-angelscript-gameplay`).
- **Spawning a `Replicates` EntityScript under a non-replicating owner in tests** → `[REP_DEBUG]`
  flood / rep ensures; test may pass while silently broken (`ck-game-replication-patterns`).
- **Broadcasting before clearing the request fragment** → re-entrant requests wiped
  (`BB_Trashcan_Processor_Requests.as:28-29`).
- **Editing `Script/Generated/` by hand or blanket-deleting it** → hot-reload sweep / phantom
  namespaces (`Plugins/CkFoundation/Script/CLAUDE.md` §22).

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock superproject HEAD `52a75e13d` (detached, tracks dev) and
Phase-0 discovery reports (feature-lifecycle, usage-census, entity-composition, venus-recon).
Independently verified by the author: all cited commits via `git show --stat` (`582bddd26`,
`345903a10`, `287ee6601`, `66ac804db`, `1ca589b0d`, `de3099e1c`, `4713da549`, and the Employee
chain `5df283d37`…`cdf71b36a`); full reads of `Script/ECS/Entryway/{Feature,Utils,EntityScript,
Processor_Setup}.as`, `Script/ECS/Trashcan/{Feature,Utils,Processor_Requests}.as`, the Entryway
AutoTest, `Plugins/CkFoundation/Script/CLAUDE.md`, and the root doctrine Non-negotiables.
Generic-vs-BB-specific labels come from the Venus recon (second consumer, CkFoundation pinned
2026-03-25).

Volatile claims and how to re-verify (Git Bash, from the consuming project root — note the
repo-root `.ignore` typically hides `Script/` from ripgrep, hence `--no-ignore`):

```bash
# The reference features still exist and keep the taught shape:
rg --no-ignore --files Script/ECS/Entryway Script/ECS/Trashcan
# The arc commits still resolve:
git show --stat 582bddd26 1ca589b0d 66ac804db 287ee6601 de3099e1c
# AS-first still holds (expect ~0 game C++ CK-macro usage):
rg -c 'CK_REGISTER_PROCESSOR|FProcessor_' Source/ || echo "no game C++ processors"
# Processor base class still current (expect UCk_Processor_Script_Base_UE, not Actor ticks):
rg --no-ignore -o ': UCk_Processor_Script_Base_UE' Script -g '*.as' | wc -l
```

If a re-verify fails, fix THIS file and note the change; the framework skills cited here
(`ck-angelscript-interop`, `ckecs-architecture-contract`, `ck-macros-and-codegen`) own their own
facts.
