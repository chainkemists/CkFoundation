---
name: ck-game-debugging-playbook
description: Use when a gameplay feature built ON CkFoundation misbehaves at runtime — "my feature
  doesn't fire", a signal callback never runs, a Request_* seems ignored, an entity/handle is
  unexpectedly invalid, discovery/binding misses entities that spawned late, an interactable focuses
  but never starts the interaction, a bound script is invoked after its entity died, "works in PIE
  but not packaged" (behavior divergence, not a crash), or you need the debugger tooling
  (ck.EcsDebugger, ck.DebugOverlay, ensure watermark, stat CkProcessors) to inspect live entities.
  For BUILDING games ON CkFoundation. Not for build/UHT/linker/AS-compile errors or packaged
  crashes (ck-debugging-playbook), perf claims (ck-performance-and-analysis), or writing tests to
  pin a bug (ck-game-testing-discipline); for modifying the framework itself, see
  ck-debugging-playbook + ckecs-architecture-contract.
---

## Overview

Symptom-first triage for the layer ABOVE build failures: the game compiles, the editor boots, and
your feature *silently does nothing*. The framework's `ck-debugging-playbook` owns everything below
(linker/UHT/AS-compile walls, packaged 0xC0000005 crashes, DLL locks) — this skill owns "the code
runs but the behavior is wrong", plus consumer-side usage of the CkGameplayDebugger tooling.

Method: find your symptom in the triage table, run the **discriminating experiment** before
touching code. Nearly every symptom here has 2–3 plausible causes; the observed base rate across
the corpus (BusterBlock, ~7000 commits; incident taxonomy verified 2026-07-03) says start with
**timing** — the entity/tag/binding you relied on did not exist yet at the moment you looked. That
is the single most common failure mode, and the one agents most often skip past.

Jargon, once: **Entity/Handle** — `FCk_Handle`, the typed reference to an ECS entity.
**Fragment** — an ECS component on an entity. **Request** — a deferred mutation (`Request_*`
functions queue; a processor consumes later — root `Plugins/CkFoundation/CLAUDE.md` non-negotiable
#5). **Signal** — the framework's bindable event (`BindTo_OnX` / `UnbindFrom_OnX`).
**EntityScript** — the AngelScript class that composes an entity (the placeable unit).
**Composition** — a feature's `utils_<feature>::Add(...)` stamping fragments/tags/children.
**Ensure** — `CK_ENSURE_IF_NOT`, the house validation macro (dialog in editor, log-only or silent
elsewhere — see §7).

## When NOT to use this skill

| You actually have | Load instead |
|---|---|
| Build/linker/UHT error, AS compile wall, editor won't boot | `ck-debugging-playbook` (framework) |
| Packaged client *crashes* (0xC0000005, GC pool) | `ck-debugging-playbook` §5 |
| "Is this slow?" / profiling / pump-limit warnings | `ck-performance-and-analysis` |
| Writing an AutoTest/Gauntlet test to pin the bug you found | `ck-game-testing-discipline` |
| "Has this failure been seen before?" — incident history | `ck-failure-archaeology` |
| Designing replication topology (not debugging a rep symptom) | `ck-game-replication-patterns` |
| AS language/idiom questions, hot-reload loop | `ck-game-angelscript-gameplay` |
| Understanding WHY the contract is shaped this way | `ckecs-architecture-contract` |

## The triage table — "my feature doesn't fire"

Ordered by observed frequency in the corpus. Start at row 1 even if a later row "feels" right.

| # | Symptom | Most likely cause (ranked) | Discriminating experiment | § |
|---|---|---|---|---|
| 1 | Discovery/binder/scan finds nothing, or misses SOME instances; works when you place things earlier | Target composed after your construct-time scan; tag stamped before composition finished; binder bound after the only broadcast | Log the entity handle + timestamp at compose AND at consume; diff the order. Inspect fragments live via `ck.EcsDebugger` (§6) | §1 |
| 2 | `Request_*` called, nothing happens | You read the result same-tick (deferred contract); target handle invalid at consume time; no processor consumes it | Read one tick later (`WaitOneFrame` in tests / a one-shot timer in game code); check `ck::IsValid(Handle)` at the call site; `stat CkProcessors` shows the consuming processor | §2 |
| 3 | Signal callback never runs (or runs after the entity died) | Wrong binding policy (bound after the broadcast, no replay); producer never fired; unbind leak invoking a dead script; destroy-mid-interaction | Log at broadcast site and at bind site with tick/frame numbers; check the bind's `ECk_Signal_BindingPolicy` | §3 |
| 4 | Handle invalid where "it must be valid"; `ck::ToActor` ensure fires | Stale handle (entity destroyed/pending-kill); entity never had an OwningActor | Walk the validity ladder; use `TryGet_EntityOwningActor` for pawn-less entities | §4 |
| 5 | Interactable focuses (highlight/prompt shows) but interaction never starts | Interaction channel mismatch between source and target; interactable lives on a child probe-node, not the owner | Enumerate the target's channels; compare against the source's channel | §5 |
| 6 | Works on server/host, wrong or blank on clients; value "arrives" pre-filled and callback never fires | Rep-notify bound after the one-shot initial dispatch; replicated spawn under a non-replicating owner; client wrote a server-authoritative value | See routing in §6 → `ck-game-replication-patterns` | §6 |
| 7 | AS change "took" but old behavior persists; types vanish; f-string throws at PIE start | Hot-reload didn't apply; stale `DynamicHandleTypes.json`; runtime-only AS errors | Route: `ck-game-angelscript-gameplay` + `ck-angelscript-interop` §2 | §6 |
| 8 | Behavior differs between PIE and packaged (no crash) | Ensure silence, cook stripping, define gate | §7, then `ck-debugging-playbook` §6 | §7 |
| 9 | Crash/ensure storm on PIE stop, level switch, or entity destroy | Teardown order: bound signals or queued requests outliving the entity | §8 | §8 |

---

## §1. Composition & discovery timing — check this FIRST

**The #1 failure mode.** Entities on this framework compose asynchronously and in stages: an
EntityScript's `DoConstruct` may return `Continue` and finish frames later; child entities spawn
deferred; discovery tags are (correctly) stamped LAST; replicated entities exist on clients before
their values arrive. Any code that scans, binds, or reads "at construct time" silently misses
everything that isn't there *yet* — and everything that arrives *later*.

The full prevention rules (stamp the discovery tag last, delta-gate tag-query results, defer-rescan
for late spawns) are owned by `ck-game-feature-recipe` and `ck-game-driver-architecture`. This
section is the *diagnosis* side.

**Discriminating experiments, in order:**

1. **Log both ends with frame numbers.** At the compose site (end of `utils_<feature>::Add` or the
   EntityScript's construction-finished point) and at the consume site (your scan/bind), log the
   handle and `GFrameCounter`-equivalent context:

   ```angelscript
   // compose side (feature Utils, after the discovery tag is stamped)
   ck::feature::Log(f"[<Prefix>Feature] composed {InHandle.ToString()}");
   // consume side (driver/binder)
   ck::feature::Log(f"[<Prefix>Driver] binding {Found.ToString()}");
   ```

   If the consume line prints before the compose line for the missing instance — timing confirmed.
   (Raw handles don't format in f-strings; always `.ToString()` — `ck-angelscript-interop` §2.)

2. **Inspect the entity live at the failing moment.** `[EDITOR-VERIFY]` In PIE, open the console
   (`` ` ``) and run `ck.EcsDebugger` to toggle the CK ECS Debugger tab (also under Tools → Debug).
   Find the entity, read its fragment list: is the feature's marker fragment there? The discovery
   tag? The State fragment's child handles? A half-composed entity (marker present, tag absent) is
   the "tag stamped after composition" race made visible. Console surface reference:
   `ck-gameplaydebugger-extension` Runbook B "Verify".

3. **Ask "who re-scans?"** A one-shot construct-time scan is only correct if nothing relevant can
   spawn later. In any real level something always does. Corpus examples (BusterBlock, all
   verified 2026-07-03): `66ac804db` "defer-rescan tag discovery so late-composing entities still
   bind"; `1ca589b0d` "reliably discover async-composed gondolas"; `287ee6601` scoping discovery so
   a *second* matching entity elsewhere in the world isn't claimed by the wrong owner. If your scan
   has no late-spawn path, that is the bug regardless of what today's repro looks like.

4. **Bound before the broadcast?** A signal with a future-only binding policy connected one frame
   after the producer's only fire hears nothing, forever (see §3). For replicated values the
   analogue is `d6f9785a4`: on clients the initial replicated values were applied by the one-shot
   rep-notify dispatch *before* the consumer bound — values sat there pre-filled, the callback
   never fired, panels stayed blank. The fix pattern: after binding, do one explicit reconcile read
   of the current state. Never rely on "the first broadcast will initialize me."

**Anti-pattern flag:** if your fix is "add a 0.5 s timer and scan again", you are re-implementing —
badly — the readiness contract the feature should expose (`Promise_OnConstructed`, a
`Promise_OnReady`-style acquire, or delta-gated tag-query updates). Timers hide the race; they
don't close it. (Maintainer-ranked worst-debt category: band-aid bootstraps.)

## §2. Request never processed

The contract (root CLAUDE.md non-negotiable #5; mechanics in `ckecs-architecture-contract` §3
"Requests — the deferred-mutation contract"): `Request_*` writes into a Requests fragment; a
processor consumes it on a later pass of the pump. Consequences:

1. **Never read the mutated state in the same tick you issued the request.** In game code, react
   in the feature's completion signal or `OnValueChanged` payload instead of re-reading. Corpus
   example (BusterBlock, `de3099e1c`): a test read a just-added entity tag the same tick —
   `utils_entity_tag::Add` is deferred like everything else; the assert had to move past a
   one-frame wait.
2. **Check the target handle is valid when the processor runs, not when you called.** A request
   queued on an entity that dies the same frame is never consumed — the destruction pipeline drops
   it (`ck-lifecycle-teardown-campaign` "Mechanics primer": requests queued on a dying entity are
   never consumed). If your request races a destroy, you have a §8 problem.
3. **Confirm the consuming processor exists and ticks.** `[EDITOR-VERIFY]` In PIE: console →
   `stat CkProcessors` — find the feature's processor row (script processors appear by class
   name). No row = it never registered (wrong `_MarkedDirtyBy` tag, or the processor class failed
   to load — check the AS compile verdict, §6 route). Row present with 0-entity work while your
   request is pending = the dirty tag/fragment pairing is wrong. Stat-group reference:
   `ck-performance-and-analysis` §1.2; pump/dirty internals: `ckecs-domain-reference` §2.4
   "MarkedDirtyBy — what marks, what clears, what pumps".
4. **Destruction is a multi-tick pipeline.** `Request_DestroyEntity` does not invalidate the
   handle that frame; post-destroy asserts/queries must poll until the handle is actually invalid.
   Corpus example (BusterBlock, `585fd6035`): destruction-timing asserts fixed by polling, not by
   a fixed delay. Pipeline detail: `ckecs-domain-reference` §3.4 "Destroy flow — deferred, staged,
   leaf-first".

## §3. Signal never fired — or fired into a corpse

Three distinct failure shapes. Identify which one you have before changing anything.

**3a. Bound too late, wrong replay policy.** Binding policies, verbatim from root
`Plugins/CkFoundation/CLAUDE.md` (Signals block — a past doc typo'd these and taught non-compiling
code): `ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame` (replay same-frame payload),
`FireIfPayloadInFlight` (replay last payload from any frame), `IgnorePayloadInFlight` (future fires
only). PostFire: `DoNothing` | `Unbind`. If the producer broadcast once during composition and you
bound with `IgnorePayloadInFlight` a frame later, you hear nothing and no error is raised. Check
the bind's policy argument *before* suspecting the producer — this enum is a fossilized bug class
(`ck-failure-archaeology` §9). Full semantics incl. "only the last payload replays" and
"Unbind + replay never connects": `ckecs-architecture-contract` §5.

**Known framework trap:** CkTimer's generated `BindTo_*` overloads drop the `InPostFireBehavior`
argument — asking for `PostFireBehavior::Unbind` on Timer signals is silently not honored
(framework DECISIONS §36, `Plugins/CkFoundation/.claude/reports/DECISIONS.md`). If a timer callback
keeps firing after you "unbound via PostFire", this is why; unbind explicitly.

**3b. Producer genuinely never fired.** Discriminate with a log at the broadcast site (or the
feature's existing log function — C++ features have per-module `ck::<feature>::Log/Warning/...`
free functions via `CK_DEFINE_LOG_FUNCTIONS`, and they are callable from AS: corpus example
(BusterBlock) `ck::ai::Log(...)` in NPC code). If the producer is gated on its own state machine or
occupancy/trigger logic, check §1 first — "producer never fired" is usually "producer's OWN
discovery/timing failed", one level down.

**3c. Bound-and-leaked: the callback runs on a dead script.** The canonical consumer incident
(BusterBlock, `05d2dc527`, verified): a driver bound a world-singleton's `OnTimeChanged/OnDayChanged`
at construct and never unbound; when the driver was destroyed mid-session the singleton kept a
delegate into the now-EndedPlay script; the next hour-boundary tick invoked the dead script and
tripped `ck::IsValid(_AssociatedEntity)`. Symptom signature: an ensure/`IsValid` trip in a callback,
on an entity you destroyed earlier, triggered by an unrelated periodic event.

Rule: **every cross-entity `BindTo_*` in an EntityScript needs a matching `UnbindFrom_*` in
`DoEndPlay`** (bind-to-self is torn down with the entity; bind-to-*another* entity is not). Also
re-arm bindings after save/load restore — snapshot restore does not restore signal bindings
(corpus: BusterBlock `a31e46f13`, `742867535`).

**3d. Destroy-mid-interaction landmine.** Destroying an entity that is currently the target of an
interaction leaks the interaction and the source never hears Failed — a live, known framework
defect. Use the feature's Cancel verb to interrupt, then destroy. Details and status:
`ck-lifecycle-teardown-campaign` (Mechanics primer + evidence table).

## §4. Handle invalid

Walk the validity ladder — `ckecs-domain-reference` §2.2 "FCk_Handle — what makes it valid or
invalid" (3-step ladder, incl. the pending-kill window where a handle is default-invalid to
`ck::IsValid` but the entity still exists for in-flight iteration). Common consumer cases:

- **Stale cached handle**: you cached a handle in a member/fragment; the entity was destroyed;
  entity slot reuse makes the handle fail validation. Guard every use of a cached cross-entity
  handle with `ck::IsValid`, and reset the member after you destroy what it points to.
- **`ck::ToActor` on an actorless entity**: `ck::ToActor` is the *checked* conversion — it ensures
  when the entity has no OwningActor. Most gameplay entities on this framework are pawn-less pure
  entities; use `UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor` (AS: the `TryGet_` form) and
  handle absence. Bridge forms table: `ckecs-domain-reference` §3.3.
- **Read-after-destroy in the same frame**: see §2 point 4.

## §5. Wrong interaction channel / probe mismatch — the silent Focused-stuck

Interaction is channelized: an InteractSource engages an InteractTarget only when both speak the
same channel. A mismatch is **silent**: the focus/highlight path still works, so the object *looks*
interactive, but `Get_InteractTarget(<Channel>)` on the source's channel returns invalid and the
interaction state machine sits in Focused forever. No ensure, no log.

Corpus incident (BusterBlock, `de3099e1c`, verified): a refactor moved a station's engage target
from Primary to Tertiary; a helper and a test interactor stayed on Primary — everything focused,
nothing ever started a transaction, and the failure surfaced only as a test that "never started".

**Discriminating experiment:** at the failure moment, enumerate the target side — which channels
does the interactable actually expose? (`ck.EcsDebugger` on the interactable's entity; or log
`Get_InteractTarget(<each channel>)` validity from the source.) Compare against the channel the
source engages with. One invalid-on-your-channel result is the whole diagnosis.

**Topology trap:** a composed feature's interactable lives on a **child probe-node entity**, not on
the feature's root — use the feature's `Get_Interactable()` accessor; `As_Interactable()` on the
owner handle fails unless you already hold the probe-node handle. When a refactor changes a
channel, sweep ALL engagers (gym helpers, tests, AI interactors) in the same change — the compiler
will not find them for you.

## §6. Routed symptoms — know when to leave this skill

**Replication-shaped** (right on server, wrong/blank/stale on clients; `[REP_DEBUG]` log flood;
`Get_Replication` ensure at spawn): route to `ck-game-replication-patterns`. Headline rules so you
recognize the shape: replicated EntityScripts must spawn under a replication-capable lifetime owner
(ActorRelay channel — not `ck::TransientEntity()`); on clients, `OnConstructed` means *composed*,
NOT *values applied* — read initial values only after `Promise_OnReplicationComplete`
(`ckecs-architecture-contract` §7); clients never write server-authoritative state (route the write
through a server-executed path). The bind-after-one-shot-dispatch trap is §1 point 4.

**AS-silently-broken** (behavior matches OLD code; a class/type vanished; "is not a data type"
walls after a rebase; f-string throw at PIE start only): route to `ck-game-angelscript-gameplay`
for the consumer loop and `ck-angelscript-interop` §2 (silent-breakage catalog) for the mechanics.
The two-boot discriminator matters here: the first editor boot after handle/codegen changes is
*expected* to self-heal red; a second boot still red is a real error (`ck-debugging-playbook` §4.1).
A stale `Script/Generated/DynamicHandleTypes.json` after a rebase produces mass "not a data type"
cascades — repair the registry, don't chase the individual errors.

## §7. PIE vs packaged — consumer edition (divergence, not crash)

The deep half (define gates, cook stripping mechanics, config diffs, the 6-axis checklist) is
`ck-debugging-playbook` §6. The consumer-side facts that repeatedly bite game teams:

1. **Ensures are silent in Test/Shipping and log-only under `-unattended`** — your guard "fired"
   and nobody saw it. Before claiming "the ensure never triggers in packaged", grep the packaged
   log for it. Matrix: `ck-debugging-playbook` §6.5.
2. **Test/dev script content must not stage into Shipping.** Corpus incident (BusterBlock,
   `e0de34899`, verified): test + gym AngelScript staged into a packaged Shipping build; their base
   classes (CkTests) are disabled in Shipping, so 431 "unknown super type" errors aborted AS
   preprocessing and **the client never booted**. Fix pattern: keep dev-only AS under a directory
   skipped when `bUseEditorScripts=false` (BB uses `Script/Dev/`, later an editor-only test
   plugin), and NeverCook test maps. If your packaged build dies during AS preprocessing with a
   super-type wall, audit what got staged before debugging any individual error.
3. **Development-Editor ≠ Development-game.** Exactly four CK defines differ — behavior can
   legitimately diverge between PIE and a packaged Development client with no bug in your code.
   Reference: `ck-macros-and-codegen` §2.4 (define matrix).
4. **Engine GC verifiers vs AS-synthesized CDOs.** Corpus incident (BusterBlock, `126ab3ac6`,
   verified): AS generates UClasses after the disregard-for-GC boundary closes; their CDOs
   reference cooked unrooted assets, so `s.VerifyUnreachableObjects` / `s.VerifyObjectLoadFlags` /
   `gc.VerifyAssumptions` fatally crash/hang a packaged **Development** client while editor and
   Shipping are unaffected. If your packaged-Development client dies in a GC verify with AS CDOs in
   the callstack, this is a known class — those verifies are disabled via `[ConsoleVariables]` in
   `DefaultEngine.ini`, not by code changes. (Actual GC *crashes* → `ck-debugging-playbook` §5,
   incl. `Ck.Diag.VerifyGCAssumptions`.)

"Works in PIE" checklist before shipping a feature claim: exercised in a packaged (or at minimum
`-game`) run? Grepped that run's log for your feature's ensures/warnings? Confirmed no dev-only
content or script your feature depends on gets stripped?

## §8. Teardown-order crashes — symptoms and first moves

Symptoms: ensure storm or crash on PIE stop / level switch / mid-session entity destroy; callbacks
firing on EndedPlay scripts (§3c); interactions that never finish after their target died (§3d);
requests that vanish (§2 point 2).

First moves, in order:
1. Identify **what outlived what**: the callstack's entity vs the entity you destroyed. The
   destroy pipeline is staged and leaf-first (`ckecs-domain-reference` §3.4); world teardown on PIE
   stop is NOT the same path as `Request_DestroyEntity` (`ck-lifecycle-teardown-campaign` Mechanics
   primer) — a bug only on PIE stop points at world teardown ordering, not your destroy logic.
2. Audit the dying script's `DoEndPlay`: does every cross-entity `BindTo_*` have an
   `UnbindFrom_*`? Are memberships released (wait-lines, occupancy, reservations) so other systems
   don't hold dead-entity slots? Are owned standalone children explicitly destroyed?
3. If the crash involves an in-flight interaction, stop — that's the known framework defect;
   route to `ck-lifecycle-teardown-campaign` rather than patching around it locally.

## §9. The debugger toolbox — discriminating experiments, consumer usage

You are a *user* of these tools; authoring new overlays/inspectors is
`ck-gameplaydebugger-extension` (its Runbook A/B "Verify" sections are the authoritative
command tables). All console steps are `[EDITOR-VERIFY]` — run in PIE, backtick console.

| Tool | Command surface | What question it answers |
|---|---|---|
| CK ECS Debugger (Slate tab) | `ck.EcsDebugger` (toggle; `1`/`0` to force), or Tools → Debug menu | "What fragments does this entity have RIGHT NOW?" — the §1 half-composed check, the §5 channel check, cached-handle postmortems |
| Debug overlay (in-game, works in packaged Development) | `ck.DebugOverlay 1`, `.Next`/`.Prev` cycle focus, `.Lock`, `.Layout.Next`, `.UnpinAll`, `.Help`, `.World next\|<idx>` | "Show me this entity's live state while I play" — watch a value/state flip (or not flip) at the failing moment |
| Ensure watermark counters | CkWatermark on-screen panel rows `Ensures` / unique-ensures, backed by `UCk_Ensure_Subsystem_UE::Get_EnsureCount()`; rows configured in Watermark project settings (`UCk_Utils_Watermark_ProjectSettings_UE`) | "Did ANY ensure fire this session?" — nonzero in a run where dialogs are suppressed (§7.1) means go grep the log |
| Processor stats | `stat CkProcessors` (also `stat CkSignals`, `stat CkScript`) | "Is the consuming processor registered and doing work?" (§2.3). Interpretation: `ck-performance-and-analysis` §1.2 / §3.1 |
| Per-feature log functions | C++: `ck::<feature>::Verbose/Log/Warning(...)` via `CK_DEFINE_LOG_FUNCTIONS`; callable from AS | Turn a feature's existing verbosity up before adding new prints |
| Breadcrumb constants | `namespace constants_<feature> { const FString k_Breadcrumb_X = "[<Feature>] X"; }` — production emits it, tests/watchers reference the same const | Stable log assertions: a reworded log line can't silently break a watcher. Corpus example (BusterBlock): `constants_store_manager::k_Breadcrumb_OpenPanel`, emitted in the HFSM and watched by Gauntlet tests |
| Per-feature debug-draw toggles | A `F<Prefix>_<Feature>_DebugSettings` struct of `ECk_EnableDisable Draw*` fields (+`HasAnyEnabled()`) embedded in the feature's Params, consumed by a debug-draw section at the end of the feature's processor | Per-instance, designer-toggleable visualization of exactly the state you're triaging. Provenance: Venus corpus pattern (`Vns_Targeting_Feature.as` DebugSettings struct, verified 2026-07-03); [SINGLE-EXEMPLAR] as a *uniform* convention — BusterBlock uses ad-hoc DevViz files instead. Both are legitimate; the struct form is the more discoverable shape to copy |

Cheap, high-yield experiment shapes:
- **Two-point logging with handles** (§1.1) — one line at produce, one at consume, diff the order.
- **Freeze the confound**: if a day/time cycle or RNG feeds the repro, pin it (BB features expose
  `Request_Debug_*` setters like minutes-per-tick=0 precisely for this).
- **Bypass the physical layer**: features that react to overlap/probes should expose
  `Request_Debug_ForceEnter/ForceExit`-style requests — drive the logic directly to split "physics
  never delivered the event" from "the logic ignored it".

## §10. Evidence discipline

- **Reproduce before you claim cause.** One observation ranks hypotheses; it doesn't pick one.
  The corpus flake studies (§11) each required multiple runs before the real cause separated from
  the coincidental one.
- **The stale-green trap.** A green run that predates your latest edit — or ran against stale
  binaries/old AS — proves nothing. Rebuild/recompile, rerun, then claim. Canonical telling:
  `ck-debugging-playbook` "Common mistakes → Stale-green"; the gating discipline is
  `ck-change-control` / `ck-methodology`.
- **Read the actual verdict, not a proxy.** A wrapper script's exit banner, a "completed"
  notification, or a grep narrowed to your own file is not the result. Open the fresh log; find
  the harness's own verdict line and the error/warning summary. For AS: the reload verdict lands
  seconds after save — check it before running anything (a test can't fail "for your reason" if
  your class never compiled).
- **Isolate own-change vs pre-existing.** Before debugging a red state, stash your change and
  re-run once. Inherited breakage debugged as if you caused it is the most expensive wrong turn
  in this playbook.

## §11. Worked examples — three root-caused flakes (the method in action)

From the BusterBlock Gauntlet corpus (`Source/BusterBlock/Tests/Gauntlet/README.md`,
`PlayerOperatesCheckout` row; verified 2026-07-03). Corpus examples — the *method* generalizes,
the specifics are BB's:

1. **MoveDirection drift.** Intermittent: the pawn engaged a station, then "randomly" lost the
   engage trace. Ranked causes: trace flake? station bug? Two-point observation showed the nav
   layer kept pushing a movement input every tick after arrival — the pawn slowly drifted off the
   use-spot. Fix: zero movement each tick while engaged. Lesson: the flake lived in an *adjacent
   system still running*, not in the failing system — ask "what else writes this state every tick?"
2. **Nondeterministic construction order.** A sequence of spawned sub-entities was indexed by
   position in a values array; construction order was nondeterministic, so 2 of 3 runs picked the
   wrong one. Fix: identify each host by a **binding fragment** stamped on it
   (`FBb_Fragment_CheckoutSettle_Binding` on the host root), never by index. Lesson: any code
   indexing into "the order things constructed" is a latent flake — key by identity fragment/tag.
3. **Alignment-gated clicks.** Injected clicks during a camera pan intermittently landed on a
   *neighboring* probe. Fix: gate the click until aim is within 4° of the target. Lesson: when an
   input-shaped action "sometimes hits the wrong thing", instrument WHERE it landed before
   suspecting the handler.

Common thread: each flake was closed by **observing the intermediate state** (position each tick,
which host got picked, where the trace landed) — not by retrying until green or padding delays.

## Common mistakes

1. **Jumping to row 3–6 when row 1 is the cause.** Timing first, always. If placing the target
   earlier / spawning it before the consumer "fixes" it, you've *confirmed* a §1 bug — now fix the
   contract (readiness promise / delta-gated rescan), not the placement.
2. **Fixing a race with a timer/delay.** Hides it in PIE, resurfaces packaged or under load. §1
   anti-pattern flag.
3. **Reading a request's effect same-tick** — then concluding the feature is broken. §2.
4. **Suspecting the producer before checking the bind's policy argument.** §3a — one enum, minutes
   to check, the most common signal bug.
5. **Binding cross-entity signals without an EndPlay unbind** — works until the first mid-session
   destroy, then §3c. Grep your EntityScripts: every cross-entity `BindTo_` should pair with an
   `UnbindFrom_` or a stated reason it can't leak.
6. **Trusting "it focuses" as "interaction works".** Focus and engage are different channels. §5.
7. **Debugging a client-side rep symptom as local logic.** Blank client value + callback never
   fired = bind-after-dispatch (§1.4/§6), not a broken feature.
8. **Claiming "no ensure fired" from a Test/Shipping/`-unattended` run** without grepping the log.
   §7.1.
9. **Trusting a green run that predates the edit, or a wrapper's exit banner.** §10.
10. **Adding prints when the feature already has a log function and the debugger can show the
    fragment live.** Turn up the existing surface first — it's already load-bearing and stays
    useful after you're done.

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock superproject detached HEAD `52a75e13d` (dev tip),
CkFoundation framework skills dated 2026-07-02. Venus corpus at `D:/Repos/Venus`
(`feature/state-machine`, CkFoundation pinned 2026-03-25). Framework citations are by skill name +
section heading (stable); commit exhibits are BusterBlock superproject SHAs.

Re-verification (Git Bash, cwd = BusterBlock superproject root; NOTE: repo-root `.ignore` hides
`Script/`, plugin `Script/`, `docs/`, `Content/` from the agent Grep/Glob tools — use
`rg --no-ignore` there, always re-check a zero-match):

- Commit exhibits still real:
  `git show --stat 05d2dc527 de3099e1c e0de34899 126ab3ac6 d6f9785a4 66ac804db 1ca589b0d 287ee6601 585fd6035 3688dc72b a31e46f13 742867535`
- Binding-policy enumerators verbatim: root `Plugins/CkFoundation/CLAUDE.md` Signals block, or
  `rg -n 'enum class ECk_Signal_BindingPolicy' -A 10 Plugins/CkFoundation/Source/CkEcs`
- Debugger console surface: `ck-gameplaydebugger-extension` SKILL.md tables;
  `rg --no-ignore -n 'ck\.DebugOverlay|ck\.EcsDebugger' Plugins/CkGameplayDebugger/Source`
- Ensure watermark: `rg --no-ignore -n 'Get_EnsureCount' Plugins/CkFoundation/Source/CkWatermark`
  (widgets `CkWatermarkStat_EnsureCount_Widget` / `_EnsureCountUnique_`); enable/rows are
  project-settings driven — `[EDITOR-VERIFY]` in Project Settings → Watermark
- Gauntlet flake rows: `grep -n -i 'MoveDirection\|nondeterministic\|alignment' Source/BusterBlock/Tests/Gauntlet/README.md`
- Venus DebugSettings exemplar:
  `sed -n '125,165p' D:/Repos/Venus/Script/ECS/Targeting/Vns_Targeting_Feature.as`
- Breadcrumb-constant exemplar:
  `rg --no-ignore -n 'k_Breadcrumb' Script/ECS/StoreManager/BB_StoreManager_Feature.as`
- CkTimer PostFire drop: framework `Plugins/CkFoundation/.claude/reports/DECISIONS.md` §36
- Destroy-mid-interaction defect status: `ck-lifecycle-teardown-campaign` (a live campaign — if it
  has concluded, soften §3d/§8.3 to its outcome)

Volatile facts: the frequency ordering of the triage table reflects the 2026-07 BusterBlock
incident corpus; re-rank if a future consumer's incident mix differs. The `Script/Dev/` staging
convention (§7.2) was BusterBlock's fix shape at `e0de34899` and later evolved (tests moved into an
editor-only plugin) — the invariant to teach is "dev-only AS must not stage into Shipping", not the
directory name.
