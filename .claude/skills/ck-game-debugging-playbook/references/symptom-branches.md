# Symptom branches §2-§9

Reference for `ck-game-debugging-playbook`: request never processed, signal into a corpse, invalid handle, channel/probe mismatch, routed symptoms, PIE vs packaged, teardown-order crashes, the debugger toolbox.

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

