# Request completion delegates — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->

**As of 2026-07-27 — CAMPAIGN COMPLETE. Repo-wide rollout shipped.**
The maintainer directed that the CkTimer treatment be applied across the whole repo rather than
gate by gate, so gates 01–09 were collapsed into one rollout executed by ~30 parallel sub-agents
against a written recipe.

**Gate: 877/877, Failed 0** (`Saved/Logs/Rollout-Gate2.log`, `--no-nullrhi --discover-fresh`, 9m00s)
— identical to the pre-rollout baseline, so zero regressions.

**Scope delivered:** 48 modules converted. ~382 files in CkFoundation, 83 in CkTests, 8 in
CkGameplayDebugger. 64 `*_CancelPendingRequests` processors (all registered), 68
`TExclude<FTag_DestroyEntity_Initiate>` view exclusions, ~76 completion guards.

**Defects found and fixed during the rollout (each would have shipped silently):**
1. `ck::request::FireCancelledForPending` routed through `ck::Visitor` → `std::visit`, which only
   compiles against a `std::variant`. CkEqs, CkResolver (×3) and CkGraphics hold a plain
   `TArray<ConcreteRequest>`. Fixed centrally in `CkRequest_Completion.h` with an `if constexpr`
   variant/non-variant dispatch.
2. **52 sites** where a `Request_*` early-returned from inside a `CK_ENSURE_IF_NOT` body and so
   never fired its delegate at all — a caller awaiting completion waited forever. ~72 guards
   rewritten to the non-negotiable-#3 shape across 12 modules.
3. CkPmg Donut bound the completion guard to live fragment storage that `Remove<MarkedDirtyBy>()`
   destructs — the guard's destructor would fire on a dangling reference.
4. CkEcsExt `_ScaleRequests` is a `TOptional`; a second same-frame `Request_SetScale` silently
   destroyed the first request's delegate. Superseded requests now complete `Failed`.
5. CkStateMachine's non-authority path discarded the whole request batch without reaching a
   handler, so a bound delegate never fired.
6. 137 AngelScript call sites broke because dropping `ECk_MinMaxCurrent`'s C++ default (to make
   room for the trailing delegate) made it REQUIRED in AS. Caught only by the gate — the C++ build
   was green. **AngelScript is a third environment; "no behaviour change" must be argued in all
   three.**

**Structural discoveries:** CkGoap's 11 Planner/WorldState request structs did not derive
`FCk_Request_Base` at all (added). `FCk_Request_EntityScript_Replicate` and
`FCk_Request_ReplicationDriver_ReplicateEntity` are dead — declared, never referenced, and the
latter's fragment befriends a processor that does not exist. `FCk_Request_ActorRelay_AcquireChannel`
likewise dead. CkTween's deferred mutators are not named `Request_*` at all.

**Open decisions for the maintainer (none blocking, all recorded):**
- `FCk_Request_Eqs_RunQuery::_OnComplete` and `FCk_Request_DialogEmitter_Query::_OnComplete` were
  KEPT, not deleted per G0-D4 — both carry query RESULTS and are the only channel the deferred path
  can reach. G0-D4 predates that finding.
- CkPmg's 7 Donut field setters have no delegate: they coalesce into one shared single-slot
  fragment where a later setter would silently drop an earlier pending delegate. Giving them honest
  completion needs the fragment to hold a LIST of delegates.
- CkInventory's 3 Spatial authority rejections fire only the generic delegate; the base-class
  equivalents fire both channels. Asymmetric but not contradictory.
- CkGoap WorldState `Request_AddSubscriber`/`RemoveSubscriber` unconverted — orchestrator scoped
  that agent too narrowly, not a deliberate exclusion.
- `CkStateMachine::Request_RecordTransition` (debug queue) has no cancel processor.

<!-- historical state below -->

**As of 2026-07-26 afternoon (orchestrator: Fable 5):** G0-D16 discriminator run 1 DONE —
**INV-C mechanism CONFIRMED** (INV-C3): in the captured FAIL arm, `_Timer` is iterated BEFORE the
tick-timer every frame; OnTick reads `_Timer` post-update; snapshot ≈ one frame's dt; after the
single reset drain (frame 75) the poll `NowMs < snapshot` is reachable only at frame 76 and only
on a sub-ms dt-jitter win — lost flip = unrecoverable growth = the observed 5s no-result timeout.
Two model corrections recorded (entity mapping: every-frame reset drains belong to the tick-timer;
drain runs AFTER Update in-frame). D-test hung AGAIN in the same run (INV-A4 wrinkle — the morning
run's quiet-ness is unknown; sibling automation is active today; churn-proxy silent both sessions).
**Baseline being diffed against:** unchanged — quiet-machine `875/876`
(`Saved/Logs/Gate00-INVA4-QuietMachine.log`). Gate 00 close target: 876/876.
**GATE 00 CLOSED 2026-07-26** (boundary entry above): 876/876 post-engine-fix, exit criteria
verified with evidence, Gate 01 baseline captured (876/876 zero-red, working-tree state, hashes
recorded). INV-A4 CLOSED via INV-A6 mechanism + G0-D21 engine fix (socket-level verified).
INV-C CLOSED via INV-C4 mechanism + G0-D17 test fix (5/5 in-suite).
**Next action:** open Gate 01 (Shape B/C alignment — status-board row 01 scope; per G0-D10 the
FailedNotEnqueued test lands there). Uncommitted inventory awaiting ship-time yes: campaign
change-set (CkFoundation), G0-D17 + 4 tests (CkTests), G0-D21 one-liner (engine fork, on
maintainer's `feat/debug-auto-discovery` branch alongside their WIP).
**Blocked on:** nothing.

### Superseded state block (2026-07-26 late)
**As of 2026-07-26 late (orchestrator: Fable 5):**
**G0-D13 transport implemented, built, all 4 campaign tests green** (CancelledOnTeardown 5/5
post-pivot). **INV-A4 RESOLVED:** the DynamicFragment in-suite hang = cross-session automation
interference (concurrent sibling suites from `E:\Repos\CkPlugins_Other` on UDP message-bus
multicast `230.0.0.1:6666`; 7/7 correlation; decisive quiet-machine run passed the D-test in-suite)
— campaign change-set EXONERATED. **Sole remaining red: `Timer_ResetMidFlight` (INV-C)** —
pre-existing fragile test predicate (0/10 isolated failures; in-suite-state-dependent; D13 path
confirmed semantically identical for unbound delegates — INV-C2; regression framing withdrawn).
**Baseline being diffed against:** quiet-machine full suite `875/876` (sole fail = ResetMidFlight;
`Saved/Logs/Gate00-INVA4-QuietMachine.log`). Gate 00 close target: 876/876.
**Next action:** G0-D16 discriminator (maintainer-ruled): full quiet-machine suite with `CkTimer`
log category at VeryVerbose; extract the ResetMidFlight window's per-frame timer-update iteration
order to confirm/refute the INV-C2 mechanism BEFORE any test edit. Executor dispatched.
**Blocked on:** nothing (execution in flight).

### Superseded state block (2026-07-26 mid-session)
✅ TREE STATE VERIFIED (see dated entry): both submodules match the Gate 00 inventory exactly; zero
`TEMPORARY-DIAGNOSTIC` residue in source; the killed experiment's overlay
(`ck.ensure.DetailsPolicy=0`, `Config/DefaultEngine.ini:79-80`) IS present — deliberately LEFT IN
and the **INV-A3 discriminating experiment is RUNNING NOW** (full suite, test-only, gate-state
source, `Saved/Logs/Gate00-INVA3-DetailsPolicy0.log`). Prediction on record: if the first-ensure
stack-walk mechanism is right, `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures`
COMPLETES (no hang) under DetailsPolicy 0; CancelledOnTeardown may still flake (INV-B, orthogonal).
**G0-D13 IMPLEMENTED, build green, all 4 campaign tests GREEN incl. CancelledOnTeardown** (was
the 1/3-pass flake — the transport pivot's prediction held). Executor gate: 874/876; both
failures assessed environmental (DynamicFragment = INV-A3 hang signature; CkJolt KinematicPlatformCarry
= escalated ZenServer-recovery warning, no assertion, siblings green, module untouched).
INV-A3 RESOLVED earlier this session (DetailsPolicy-0 run: DynamicFragment passed in-suite).
**Verification rerun done: 874/876 — campaign tests 2/2 green across both post-D13 suites, but
DynamicFragment hung 4/4 on campaign trees → INV-A4 opened (deterministic in-suite red, ensure-
details path, root delta vs baseline unknown). ResetMidFlight failure assessed one-off
(respawned-editor turbulence; G0-D13 surface unreachable from that test).**
**INV-A4 BREAKTHROUGH CANDIDATE (E-B-lite + orchestrator log forensics):** a sibling session runs
concurrent full test suites from `E:\Repos\CkPlugins_Other` (linked worktree); BOTH editors join
UDP message bus multicast `230.0.0.1:6666` (investigator confirmed live: two `CkPluginsEditor-Cmd`
processes; the "hung" editor emitted a `UpdateKnownNodes: Removing Node` bus line 2m24s INTO the
hang → networking threads alive, game thread stalled — NOT a whole-process freeze). E-B-lite run:
hang 6/6, sole failure, campaign tests green (CancelledOnTeardown 4/4 post-D13). Capture missed on
a fixed harness bug (wrong process name; correct filter `CkPluginsEditor*` + `*D:\Repos\CkPlugins\*`
+ NOT `*_Other*`); harness now dry-tested and armed. **Orchestrator node-churn forensics across
all 11 logs:** every in-suite hang run shows bus churn; BUT baseline (PASS) had 5 node removals
and INV-A3 (PASS) had 1 → "sibling active ⇒ hang" is TOO SIMPLE; removal lines only mark peer
DEPARTURES (weak proxy — a steadily-running peer logs nothing), and peer IDENTITY varies (earlier
peers were BusterBlock sessions; the CkPlugins_Other AUTOMATION peer — same automation protocol on
the bus — may be recent). Refined hypothesis: hang requires in-suite state AND a concurrent
AUTOMATION peer on the bus (isolated passed 5/5 even during sibling activity — contention alone
insufficient). Sibling also had the SAME test file open in an editor window (likely chasing the
same hang from the E: worktree — coordination needed, sessions are contaminating each other's
experiments). **DECISIVE EXPERIMENT (blocked on maintainer coordination): full suite with the
sibling verifiably quiet + live peer monitoring.** Pass ⇒ INV-A4 = cross-session interference,
Gate 00's last failure evaporates. Hang ⇒ sibling falsified, armed stack capture fires next run.
**INV-A4 E-A: isolated D-test PASSES on current D13+D14 binaries (~1.9s)** — isolated 5/5 PASS vs
in-suite 5/5 HANG on one binary set: the hang is SUITE-STATE-DEPENDENT, not binary-dependent.
(Bonus: D14's restore path positively verified — both "Restoring ensure ... policy" lines fired at
batch end.) E-B stack capture blocked: NO cdb/procdump anywhere on the machine (full discovery
table in the investigator's report; installing either = machine-global write, needs maintainer
yes). **E-B-lite dispatched instead (no installs): at hang, sample process CPU (spin vs wait),
thread wait-reasons, top-level windows (modal detection), + MiniDumpWriteDump via dbghelp for
later analysis.** Orchestrator hypothesis ranking on record: (1) game-thread synchronous block in
ensure MESSAGE construction / formatter walk over accumulated suite registry (watchdog-never-fired
deduction: ticking itself stopped); (2) attribution error — baseline passed in-suite exactly ONCE,
so "campaign causes it" rests on one sample; re-baseline ×2 experiment queued behind E-B-lite;
(3) stock-engine modal from the empty-struct replication path.
**G0-D14 landed but its mechanism is FALSIFIED** — the details-policy override was provably
active and DynamicFragment hung anyway (5/5 in-suite hangs on campaign trees; see the dated
entry). With slow-symbolication dead, a REAL campaign-induced in-suite hang is a live hypothesis.
Campaign tests remain perfect: CancelledOnTeardown 3/3 post-D13. D14 edit kept (harmless,
removes real stack-walk cost). **Next discriminator: isolated D-test on current binaries**
(isolated hang ⇒ binaries regression; isolated pass ⇒ suite-state-dependent ⇒ subset bisection).
**Blocked on:** maintainer re-ruling (fork being raised interactively).

### Superseded state block (2026-07-25 night)
**As of 2026-07-25 night (third pass being dispatched):** Engine RESOLVED (orchestrator registered
`{22D2B5AE-…}` → `D:\Repos\UnrealEngineAngelscript` in HKCU, maintainer-authorized; toolbox profile
already pointed at registered `{E4464C1C-…}`; UBT confirmed resolving). G0-D9 empirically REJECTED
by UHT → maintainer ruled **G0-D9b** (trailing param, no C++ default, AS-generator-emitted defaults).
Clean-HEAD build breaks (CkUI/FSlateUser, CkGameplayDebugger/CkAggro includes) are FIXED UPSTREAM —
both submodules just need ff to origin/dev (CkFoundation +4 commits incl. `43e5e6efd`;
CkGameplayDebugger +5 incl. `d34bad6`).
**Baseline being diffed against:** NOT YET CAPTURED — capture at NEW HEAD after the ff.
**Next action:** Executor third pass: ff both submodules (stash→ff→pop; expected conflicts:
`CkTimer_Utils.h`, root `CLAUDE.md`) → apply G0-D9b (drop 9 defaults, extend CkAngelscriptGenerator,
regenerate, update C++ call sites) → baseline at new clean HEAD → full gate.
**Blocked on:** nothing (execution in flight).

## Blockers

| Blocker | Detail | Unblock |
|---|---|---|
| ~~Toolbox engine selection~~ RESOLVED 2026-07-25 | Orchestrator registered `{22D2B5AE-…}` → `D:\Repos\UnrealEngineAngelscript` under `HKCU:\Software\Epic Games\Unreal Engine\Builds` (maintainer-authorized; undo = delete that value). Executor confirmed UBT resolves and compiles | — |
| ~~Clean HEAD does not build~~ RESOLVED upstream | CkUI `FSlateUser` drift fixed by `43e5e6efd` (origin/dev); CkGameplayDebugger CkAggro includes fixed by `d34bad6` (origin/dev) | ff both submodules to origin/dev (third-pass step 1) |
| Campaign docs are `.gitignore`d (`*.md`, CkFoundation `.gitignore:49`) | BY DESIGN — "they should be force added instead"; prior campaigns' docs are tracked via force-add | `git add -f docs/campaigns/request-completion-delegates/*` at ship time |

## Status board

| Gate | Scope (tentative membership — finalized at each gate's entry) | State |
|---|---|---|
| 00 | CkEcs shared contract + CkTimer pilot + AutoTests | ✅ Done (2026-07-26) |
| 01–09 | **COLLAPSED into a single repo-wide rollout at maintainer request (2026-07-27).** Gates 01–08's module membership was delivered in one pass; gate 09's acceptance is the 877/877 gate below. | ✅ Done (2026-07-27) |

**Gates 01–09 superseded.** The maintainer directed that the CkTimer treatment be applied to the
whole repo rather than gate by gate, so the phased membership above was executed as one rollout.
Kept here only as the record of what each gate would have contained.

## Decision log

| # | Date | Decision | Why | Revisit when |
|---|---|---|---|---|
| G0-D1 | 2026-07-25 | Mechanism = Shape A (PopulateRequestHandle + CK_SIGNAL_BIND_REQUEST_FULFILLED + TRequestResultGuard) | Designated idiom (`CkSignal_Macros.h:47`), reference impl exists (CkInventory) | never — foundation |
| G0-D2 | 2026-07-25 | Shared `ECk_Request_OperationResult` default; bespoke per-op enums allowed where richer (Inventory keeps its own) | **Maintainer-ruled** | a feature needs a 5th shared result kind |
| G0-D3 | 2026-07-25 | ALL requests get the delegate, including trivial setters | **Maintainer-ruled**; uniform contract | — |
| G0-D4 | 2026-07-25 | CkEqs/CkDialog `_OnComplete` struct members deleted; trailing-param shape everywhere; no back-compat | **Maintainer-ruled** | — |
| G0-D5 | 2026-07-25 | Guaranteed-fire: pending requests fire `Failed_Cancelled` at owner teardown via shared helper | **Maintainer-ruled** | — |
| G0-D6 | 2026-07-25 | ONE generic signal+delegate (`UUtils_Signal_RequestCompleted`, `FCk_Delegate_Request_OnCompleted`) serves the default case; per-request entities make collisions impossible | Fable-ruled; kills ~50 signal/delegate/enum triplets | a gate hits a request needing typed payload → that feature defines a bespoke signal (Inventory precedent) |
| G0-D7 | 2026-07-25 | Completion is local-machine semantics; cross-network completion out of scope | Fable-ruled; request entities don't replicate | a game needs server-outcome-on-client |
| G0-D8 | 2026-07-25 | Populate + bind ONLY under `if (InDelegate.IsBound())` | CkEqs precedent; zero cost for the no-delegate path | — |
| G0-D9 | 2026-07-25 | RATIFIED (STOP #1): canonical UFUNCTION shape = `meta = (AutoCreateRefTerm = "InDelegate")` **plus** `= FCk_Delegate_Request_OnCompleted()` C++ default — diverges from the CkInventory reference deliberately; Gate 08 aligns Inventory's delegate params to the same shape | Only shape keeping every existing AS + C++ caller source-compatible (AS generator emits defaults only when the UFUNCTION declares one — evidence in Gate 00 report). EMPIRICALLY UNPROVEN until first build: no delegate param in the codebase has a default; if UHT rejects it, the fork returns to the orchestrator | first successful build (auto-resolves) |
| G0-D9b | 2026-07-25 | SUPERSEDES G0-D9 (UHT rejected the delegate default — 9× "C++ Default parameter not parsed", verbatim in the Gate 00 second-pass entry). **Maintainer-ruled** (offered delegate-in-struct per their fresh CkDialog 44a9aad57 precedent; maintainer chose trailing-param): UFUNCTION shape = `meta = (AutoCreateRefTerm = "InDelegate")`, NO C++ default (CkInventory shape); **CkAngelscriptGenerator is extended to emit `= FDelegateType()` defaults in generated `utils_*.as` wrappers for AutoCreateRefTerm delegate params** so AS callers omit freely; C++ call sites pass `{}` (no plain-C++ overloads added). G0-D4 REAFFIRMED against 44a9aad57 — Gate 01 migrates CkDialog's four `_OnComplete` members (Query + the three new cooldown requests) to trailing params | Maintainer-ruled 2026-07-25 (second interactive ask, after UHT evidence) | AS generator cannot express the default → STOP back to orchestrator |
| G0-D10 | 2026-07-25 | RULED (STOP #2): features with no legitimate synchronous-rejection path fire NO `Failed_NotEnqueued` and must NOT invent validation to produce one. The `FailedNotEnqueued` focused test lives in a module with a real non-ensure rejection (e.g. CkInventory's authority gate) — moved to Gate 01. Gate 00 expected Total = baseline + 3, not + 4 | Root non-negotiable #3: an ensure-based rejection can't be green-tested (AutoTest escalation); inventing validation violates minimum-code doctrine | a Timer sync-rejection genuinely appears |
| G0-D13a | 2026-07-26 | Fable-ruled (header layout under G0-D13): enum + delegate type STAY in `CkRequest_Completion.h`; `CkRequest_Data.h` includes it (Completion.h drops its `CkRequest_Data.h`/`CkSignal_Macros.h` includes to break the cycle); completion guard + reworked `FireCancelledForPending` live in Completion.h; `ck::FRequest_Base` (C++-only base) untouched until Gate 08. Zero call-site include churn. Full implementation contract: GATE_00_Infrastructure.md Addendum 2 | Data.h is the base header every request struct already includes; moving the UENUM would relocate its .generated.h home for no gain | — |
| G0-D12 | 2026-07-25 | RATIFIED retroactively: executor's CkTests ff `182b1438` → `71612e50` (required — CkTests was pinned behind the POI/Aggro API migrations; same reversible class as the two authorized ffs; undo `git -C Plugins/CkTests reset --hard 182b1438`) | Baseline impossible without it; upstream `fbfd2812`+`ddf15701` are exactly the AS-compile fixes | — |
| INV-A2 | 2026-07-26 | INV-A REFRAMED after E1: DynamicFragment hang is FULL-SUITE-ONLY (2/2 suite hangs vs 4/4 isolated passes on identical gate binaries); per-actor bisection did not converge (Confirm4 with all actors restored PASSED isolated); executor's original "my regression" A/B withdrawn (control confound confirmed). Suite correlation with the change-set still stands: baseline suite 872/872 passed, both gate suites hung it. Candidate unifying mechanism (UNPROVEN): CK ensures latch once-per-site per process — if the campaign's teardown-window path fires an ensure in shared code mid-suite, ReplicateEmptyStruct_Ensures (which WAITS for its own deliberate ensure) waits on an already-latched site forever. Fresh-context root-cause unit dispatched | one mechanism must explain: suite-hang 2/2, isolated-pass 4/4, baseline-pass | resolved by INV-A findings |
| G0-D13 | 2026-07-26 | **Maintainer-ruled** (third interactive ask, on INV-B2 evidence): internal completion transport = delegate stored on the request struct (non-reflected member on `FCk_Request_Base`, set at the Utils boundary under the `IsBound()` gate); drain fires it directly via a completion guard (`TryFireCompletion(Owner, Result)`, cleared after fire = exactly-once); teardown-cancel walks the pending queue and fires `Failed_Cancelled` directly. NO request entity, NO signal for the default case — `PopulateRequestHandle`/`UUtils_Signal_RequestCompleted`/signal-based `FireCancelledForPending` are REMOVED from the default path (`CkRequest_Completion.h` keeps the enum + delegate type). Converted features' request-drain views also exclude destroy-`Initiate` owners (stricter pending-kill filter) so pending-at-destroy deterministically completes as `Failed_Cancelled`. Public API per G0-D9b unchanged. Inventory/Resolver bespoke signal paths untouched until Gate 08 | INV-B2: one-shot EndPlay window + cascade-void are structural properties of binding the delegate to a request child entity; struct-carried transport makes the hole impossible and is cheaper | — |
| INV-C2 | 2026-07-26 | INV-C findings (investigator; orchestrator ACCEPTS the pushback and WITHDRAWS the "D13 regression candidate" framing): isolated ResetMidFlight 0/10 failures (1504-1969ms, deterministic pass; machine attested quiet per-run) → in-suite-state-dependent. MECHANISM (inferred, strongly constrained): the test runs TWO timer entities (`_Timer` + `Create_Tick`'s tick-timer), both iterated by `FProcessor_Timer_Update` in the same frame (`CkTimer_Processor.h:127-128`, signal fired inside Update's ForEachEntity, `CkTimer_Processor.cpp:359-365`); OnTick's observation of `_Timer` depends on VIEW ITERATION ORDER between the two — tick-timer-first ⇒ sees elapsed 0 ⇒ deterministic pass; `_Timer`-first ⇒ sees ≈DeltaT vs a snapshot that is itself ≈1 frame ⇒ coin flip, and a lost flip is unrecoverable ⇒ exactly the observed 5s no-result timeout. In_place_delete pool history (suite vs fresh world) plausibly decides the order; the campaign's 4 new tests perturb world state with zero code-path change. D13 path CONFIRMED semantically identical for unbound delegates — `PopulateRequestHandle` sits inside the `IsBound()` gate, so NO request entity ever existed on this path pre-D13 either; old guard no-op == new guard no-op; copy/clear order untouched; the Initiate exclusion cannot match (test never destroys). Statistics: 0/4 vs 3/5 → Fisher two-tailed p≈0.17, not significant. Test's fragile predicate (`NowMs < _ElapsedAtResetMs`, ~1-frame window) is the defect class; a decrease-detection predicate (`NowMs < last observed`) removes the fragility while PRESERVING the test's dropped-Reset-catching purpose under every hypothesis | evidence per claim in the dated report; file:line anchors verified | fix ruling (maintainer) |
| G0-D16 | 2026-07-26 | **Maintainer-ruled** (sixth interactive ask, resolving INV-C2's "fix ruling" hook): VERIFY THE INV-C MECHANISM FIRST — no edit to `CkAutoTest_Timer_ResetMidFlight.as` until the discriminator runs: full quiet-machine suite with log category `CkTimer` at VeryVerbose (existing instrumentation suffices — `CkTimer_Processor.cpp:357` `"Timer Counting Up with Entity [{}]"` per update per entity ⇒ per-frame view iteration order; `:88` `"Handling Reset Request..."` marks the drain and self-identifies `_Timer`'s entity; category = `CkTimer`, `CkTimer_Log.cpp:5`). Expected observations: FAIL arm ⇒ `_Timer` iterated before the tick-timer through the post-reset window; PASS arm ⇒ tick-timer first. Overlay = `[Core.Log] CkTimer=VeryVerbose` in `Config/DefaultEngine.ini`, reverted after the run. Cap 2 runs, then back to maintainer with the data. Test-fix ruling deferred until then | converts INV-C2's mechanism from inferred to confirmed before a test edit rests on it; harden-test and known-flake options both presented and declined in favor of measurement | discriminator result |
| INV-C3 | 2026-07-26 | INV-C mechanism **CONFIRMED** by G0-D16 run 1 (FAIL arm captured in-suite), with two corrections to the INV-C2 model: (i) entity mapping — the every-frame `Handling Reset Request` lines belong to the TICK-timer (`Create_Tick`'s timer: `Chrono [0.000s out of 0.000s]`, behavior `Reset And Resume On Done` → auto-reset each frame); `_Timer` (669\|26) received exactly ONE reset drain, frame 75, the first frame either timer ticked; (ii) phase order — the drain lands AFTER both Update lines within the frame, not before. Observed order: `_Timer` BEFORE tick-timer in EVERY frame of the window (75→497) — the predicted FAIL arm. Exact arithmetic: OnTick (fired synchronously during the tick-timer's update) reads `_Timer` POST-update; frame 75: elapsed = dt75 ≈ 13ms > 0 → snapshot = dt75, Request_Reset → drained end-of-75 (elapsed→0); frames 76+: elapsed regrows by ~dt/frame, so `NowMs < snapshot` is reachable ONLY at frame 76 and ONLY if dt76 < dt75 — one sub-ms jitter coin-flip, then monotone growth = unrecoverable → "engine TimeLimit elapsed without an AS-side result" at frame 497. PASS-arm corollary (tick-first order): OnTick reads `_Timer` PRE-update → clean 0 the frame after the drain → deterministic pass; 10/10 isolated passes under `_Timer`-first would need 10 straight jitter wins (p<0.1%) → isolated worlds almost certainly iterate tick-first (run 2 verifies directly). Evidence: `Saved/Logs/Gate00-INVC-VeryVerbose-run1-sessionB-respawn.log` lines 31958–33654 | one mechanism explains: in-suite deterministic fail, isolated 10/10 pass, the 5s no-result signature, the single `_Timer` drain, the every-frame tick-timer drains | run 2 PASS-arm order; then maintainer fix ruling |
| G0-D21 | 2026-07-26 | **Maintainer-ruled** (eleventh interactive ask, after research overturned G0-D20's implementation vehicles — no engine switch exists, `-asdebugport=` only; CkAutoTestRunner cannot reach the private-header server type): prevention = ENGINE FORK PATCH, shape = NO SERVER WHEN UNATTENDED — gate AS debug-server creation (`AngelscriptManager.cpp:428`) on `&& !FApp::IsUnattended()`. Unattended (automation) editors never start the server: no listener, no discovery, no attach, no exception break — kills the whole stall class for all automation on this engine (BusterBlock CI included); interactive editors unchanged (`-unattended` confirmed on the toolbox invocation, verbatim command line in CkPlugins.log). ProcessException-only suppression declined (narrower, leaves breakpoint/stray-client park paths). Engine repo implications acknowledged: AngelscriptCode rebuild now; fork commit/push at ship with explicit yes | the only clean seam is where the server is created; `-unattended` is the exact automation signal, zero toolbox/project changes | fix verified at the close gate |
| G0-D20 | 2026-07-26 | **Maintainer-ruled** (tenth interactive ask, two rulings): (1) UE-AS ENGINE-SOURCE READ AUTHORIZED (`D:\Repos\UnrealEngineAngelscript`, AngelscriptCode debug server / exception-break path) to identify the AS-debugger disable switch; implement the prevention at the automation level (toolbox editor-args or CkAutoTestRunner runtime override, D14 pattern), scoped so interactive editor sessions keep AS debugging. (2) GATE 00 CLOSES FIX-FIRST: land + verify the prevention fix, one more clean 876/876 with it active, then the boundary ritual. Close-now and quiet-rerun-first declined | maintainer risk posture: the gate closes on a suite that can no longer hang this way | fix verified at gate |
| INV-A6 | 2026-07-26 | **INV-A4 MECHANISM NAMED — captured stacks, hang hunt run 5.** GameThread verbatim (top→bottom): `ntdll!NtDeviceIoControlFile` → `MSWSOCK!WSPSelect` → `WS2_32!select` → 2× `CkPluginsEditor_Sockets` frames → 2× `AngelscriptCode!FAngelscriptPreprocessor::*` (symbol+offset — nearest export; actual fn = debug-server wait) → `asCScriptEngine::CallGlobalFunction` → **`asCContext::SetInternalException`** → `CkDynamic!UCk_ScriptQueryBatch_Mixin_UE::StaticClass+0x267` (nearest export; CkDynamic AS binding reject path) → `asCContext::CallFunctionCaller/ExecuteNext/Execute` → `UASFunction_ReferenceArg::RuntimeCallEvent` → `UObject::ProcessEvent` → `UCk_GenericEntityScript_UE::DoBeginPlay/BeginPlay` → `FProcessor_EntityScript_BeginPlay::ForEachEntity` → scheduler → `ACk_EcsWorld_Actor_UE::Tick`. READ: the D-test's deliberate invalid-input rejection raises an AS script exception; with an AS DEBUG CLIENT attached (VSCode auto-attach; `bAutoOpenVSCode=True` in `Config/DefaultEngine.ini`), the Hazelight runtime's exception handler enters a debugger break that BLOCKS THE GAME THREAD in socket `select()` waiting for the client — indefinitely. Snapshot t+35min: IDENTICAL frames and stack addresses (parked, not spinning). Explains EVERYTHING: in-suite intermittency (hang ⟺ client attached to the automation editor at exception time — attach roulette across the user's editors/VSCode sessions); networking threads alive during hang (FTcpListener/FUdpMessageProcessor stacks normal); the morning "Removing angelscript debug client" line; D14-active hangs (no ensure stack-walk involved); peer non-correlation; today's audio-warning spam even defeating the toolbox idle-kill (run 5 hung 36+ min). CAMPAIGN EXONERATED DEFINITIVELY: the mechanism is engine debug-server behavior triggered by any deliberate-AS-exception test — zero relation to the completion-delegate change-set. Evidence: `Saved/Logs/HangCapture/run5/{hang-stacks.txt, hang-stacks-t2-gamethread.txt, editor-hang.dmp (9GB), watcher.log}` | two live snapshots 35min apart, full-symbol stacks, every historical observation explained by one mechanism | maintainer fix ruling (prevention layer) + Gate 00 close ruling |
| G0-D19 | 2026-07-26 | **Maintainer-ruled** (ninth interactive ask): PIN THE HANG FIRST — G0-D15 stays strict; the first 876/876 (capture run, 16:03) is NOT accepted as Gate 00 close while INV-A4's mechanism is unknown. Plan: loop capture-armed full-suite runs (driver script, cap 3/session, per-run condition snapshots) until cdb stacks name the mechanism; Gate 00 closes only after. Close-now-hunt-in-parallel and quiet-rerun-then-close both declined | maintainer risk posture: no Gate 01 on top of an unexplained in-suite hang, even an intermittent one | stacks captured → mechanism ruling |
| G0-D18 | 2026-07-26 | **Maintainer-ruled** (eighth interactive ask, resolving INV-A5's capture fork): AUTHORIZE DEBUGGER INSTALL (machine-global write) — WinDbg/cdb (+ procdump as capture fallback) via winget; orchestrator captures AND analyzes the hung editor's thread stacks textually (G0-D15's E-B design, previously blocked on tooling). Undo = `winget uninstall` the installed package(s). Capture design: watcher triggers on the D-test going silent ~30-45s (toolbox idle-kill window ≈2min, measured: last output 16:40:38 → kill 16:42:42 this morning), non-invasive cdb attach `~*k` all-thread stacks + full minidump before the toolbox kill | direct observation at hang is the cheapest remaining path to the mechanism; no-install and characterize options declined | stacks captured |
| INV-A5 | 2026-07-26 | **INV-A4 REOPENED — both prior mechanisms falsified for today's hangs.** Close gate (post-G0-D17, machine verified quiet at launch): 875/876, sole red = D-test hang (toolbox: "editor produced no output within the idle window — treated as HUNG... Killed it", `ReplicateEmptyStruct_Ensures` in flight 22:32:16 UTC; respawn ran the remaining 518 green). Falsification 1 (interference): no automation peer in evidence at hang time — `D:\Repos\BusterBlock\Saved\Logs\BusterBlock.log` last write 15:10 local, gate test phase started 15:28, hang 15:32; E-worktree idle since 01:13; zero Unreal processes at launch check. Falsification 2 (ensure stack-walk): G0-D14's details override `MessageAndStackTrace -> MessageOnly` ACTIVE from 22:29:07 (log lines 1668-1669), 3 min before the hang — the symbolicated path was bypassed. Hang record with D14 active: 01:52 quiet run PASS (2.7s-class), morning run HANG, close gate HANG — intermittent, in-suite-only (isolated passes), NOT peer-correlated, NOT stack-walk-gated. Unknowns: why today's 2/2 vs last night's pass (sleep/wake cycles and day-long machine state are the only uncontrolled variables; binaries identical across all three). Evidence: `Saved/Logs/Gate00-CloseGate-postfix.log` | the 01:52 pass and today's two hangs share binaries, suite content, and (for the close gate) a quiet machine — no recorded variable separates them | mechanism pinned via direct observation (maintainer fork: capture tooling) |
| G0-D17 | 2026-07-26 | **Maintainer-ruled** (seventh interactive ask, resolving INV-C4's fix fork): ResetMidFlight fix = **threshold gate** — OnTick's snapshot gate raised from `> 0.0` to `> 200.0` ms (`CkAutoTest_Timer_ResetMidFlight.as`, one predicate line + comments). Widens the post-reset observable window from 1 frame to ~15 frames under BOTH iteration orders; dropped-Reset detection preserved (elapsed grows ≥ snapshot → timeout → fail). Event-driven rebind (option B) declined — would shift the test off the polled-value path it was written to cover | mechanism-anchored: fixes the fragility at its exact source (window width) with a one-line diff | never |
| INV-C4 | 2026-07-26 | G0-D16 run 2 (isolated ResetMidFlight + overlay, 1/1 PASS in 1m34s) captured the PASS arm: order = tick-timer (14\|1) BEFORE `_Timer` (16\|1) every frame — the predicted mirror of the FAIL arm. Mechanism now source-anchored end-to-end, with THREE refinements over INV-C2/C3: (i) registration order is HandleRequests BEFORE Update (`CkTimer_Processor.cpp:11-15`); the same-frame drains observed after Update lines in both logs are the end-of-frame PUMP pass draining requests enqueued mid-frame; (ii) the Reset drain ALSO broadcasts `OnTimerUpdate` after `TimerChrono.Reset()` (`DoHandleRequest` Reset case), and `Create_Tick` binds the delegate to OnUpdate (`Script/CkUtils_Timer.as:16`) — so OnTick fires TWICE per frame (Update broadcast + the tick-timer's own auto-reset drain broadcast); this explains the pass-arm frame-561 "anomaly" (snapshot taken by the PUMP-time fire, which reads `_Timer` post-update even under tick-first order); (iii) `Get_CurrentTimerValue` returns the STORED chrono (`CkTimer_Utils.cpp:196-201`) — reads are update-accumulation-based, so iteration order fully determines what OnTick observes. Outcome rule: tick-first ⇒ the next frame's Update-time fire reads `_Timer` PRE-update = clean 0 after the drain ⇒ deterministic pass; `_Timer`-first ⇒ every fire reads post-update ⇒ observed sequence dt₁, dt₁, dt₂, dt₂+dt₃, … ⇒ single dt₂<dt₁ jitter flip, then unrecoverable ⇒ 5s timeout. **CONSEQUENCE: INV-C2's suggested decrease-detection predicate (`NowMs <` last-observed) is REFUTED as a fix** — in the failing order no observation ever sees the zero; the observed sequence is monotone non-decreasing except for the same single jitter flip, so "less than last observed" carries the identical coin flip. Viable fixes: (A) raise the snapshot gate from `> 0` to a multi-frame threshold (~200ms) — first post-reset observation ≈ 1 frame dt ≪ threshold under BOTH orders, with ~15 frames of retry margin against hitches; dropped-Reset still caught (elapsed grows ≥ snapshot → timeout); one-line diff; (B) event-driven — bind `_Timer`'s own OnUpdate/OnReset and assert on the drain broadcast (order-independent, larger edit, changes what the test exercises). Evidence: `Saved/Logs/Gate00-INVC-VeryVerbose-run2-isolated.log`; overlay reverted (`git status -- Config/DefaultEngine.ini` clean) | both arms observed with predicted orders on the same binaries; every anomaly in both logs explained by the two-fire-point model with file:line anchors | maintainer fix ruling (G0-D16 cap reached) |
| G0-D15 | 2026-07-26 | **Maintainer-ruled** (fifth interactive ask, post-falsification): ROOT-CAUSE NOW — Gate 00 stays blocked until the INV-A4 mechanism is pinned; no quarantine, no Gate 01 on top of a possibly-real hang. Investigation design (orchestrator): E-A isolated D-test on current D13+D14 binaries (hang ⇒ binaries regression ⇒ hunk bisection; pass ⇒ suite-state-dependent ⇒ subset bisection); E-B full-suite run with LIVE THREAD-STACK CAPTURE of the hung editor (cdb/procdump, capture ~30s into the silence, before the toolbox idle-window kill) — one run potentially pins deadlock vs spin vs I/O outright. Fix design returns to orchestrator/maintainer after mechanism | Falsified-mechanism history (INV-A3 → G0-D14) shows guessing is more expensive than measuring; deterministic repro makes direct observation cheap | mechanism confirmed |
| G0-D14 | 2026-07-26 | **Maintainer-ruled** (fourth interactive ask; first click was accidental and discarded, re-asked, same answer chosen deliberately): unblock Gate 00 via the harness override — extend `ACk_AutoTestRunner::Install_EnsurePolicyOverride`/`Restore_EnsurePolicyOverride` (`CkAutoTestRunner.cpp:316-380`) to ALSO capture/override/restore the ensure DETAILS policy (`UCk_Utils_Core_UserSettings_UE::Get/Set_EnsureDetailsPolicy`, `MessageAndStackTrace` → `MessageOnly`) with the same batch refcount + OnEnginePreExit force-restore. Autotest ensures keep message + call site, lose symbolicated stacks. INV-A4 root-cause stays OPEN as a recorded low-priority follow-up, not gate-blocking | Kills the deterministic in-suite hang class for every future suite run; precedent is the existing display-policy override in the same functions | INV-A4 findings, if ever pursued |
| INV-A4 | 2026-07-26 | OPENED (supersedes the "environmental flake" framing — dropped on the evidence): the DynamicFragment in-suite hang is DETERMINISTIC on campaign trees under default ensure policy — 4/4 hangs (gate run 1, gate rerun, D13 executor gate, D13 verification rerun) vs baseline-tree in-suite PASS (2.712s) and DetailsPolicy-0 in-suite PASS and isolated PASS 4/4. The stall is inside the ensure-details path (`StackWalkAndDump`, `CkEnsure.cpp:113-126`) — that much the DetailsPolicy-0 discriminator establishes — but WHY the campaign tree crosses the toolbox idle-window threshold when the baseline tree paid only 2.7s is UNKNOWN (pre-D suite content is identical — the 4 new tests are all alphabetically after D; deltas: rebuilt binaries/PDBs, generator-emitted AS defaults). Secondary effect explained by one mechanism: each hang kills the editor mid-suite → cold respawned editor runs the D-onward 518 tests → hitch-prone window → one-off watchdog/warning failures (CkJolt+Zen in the executor run, Timer_ResetMidFlight in the verification rerun) | one mechanism explains: 4/4 deterministic hang, both one-off secondaries, baseline pass, policy-0 pass, isolated pass | maintainer fork ruling (harness override vs root-cause) |
| INV-A3 | 2026-07-26 | INV-A2 findings (fresh-context investigator, read-only): H1 ensure-latch FALSIFIED (`…ReplicateEmptyStruct_Ensures.as:33-47` never waits on an ensure; expected-errors are suppress-all, `CkAutoTestRunner.cpp:441`); H3 pre-activation contamination FALSIFIED (`ACk_AutoTestRunner` has no BeginPlay override; all entity work in PrepareTest). CORRECTIONS: the "last line = ensure-policy override" observation was an isolated-run artifact (the override logs once per process, `CkAutoTestRunner.cpp:326-339`); the whole AS suite runs in ONE PIE world sequentially, alphabetical order (D before T — campaign tests cannot precede the hang); the earlier "cancel processor RULED OUT" rested on an isolated run and is INVALID (re-opened under INV-B). LEADING MECHANISM (supported, unproven): first-ensure-per-process pays a fully-symbolicated `StackWalkAndDump` (PDB load, global lock, I/O-bound; `CkEnsure.cpp:113-126`, `CkDebug_Utils.cpp:275-288`) — the D-test is the suite's first deliberate-ensure test, took 2.712s even in the passing baseline (8× suite median), and the machine was I/O-shared with sibling sessions. Discriminating experiment dispatched: full suite with `ck.ensure.DetailsPolicy 0` | evidence per row in the investigator's report (file:line + verbatim logs, dated entry) | experiment result |
| INV-B | 2026-07-26 | OPEN: `Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown` is intermittent on identical binaries (gate run 1 PASS, rerun FAIL: "engine TimeLimit elapsed without an AS-side result", 4.006s, CkAutoTestRunner.cpp:254). Suspected race: same-frame pump drain fires `Succeeded` vs deferred-destroy leaving the request pending for the EndPlay cancel (`Failed_Cancelled`) — the test may not construct the pending-at-teardown window deterministically. Mechanism + deterministic construction to be established before any fix | the guaranteed-fire contract's teardown window is the gate's load-bearing unknown; a flaky pilot test cannot anchor the framework-wide rollout | resolved by INV-B findings |
| INV-B2 | 2026-07-26 | INV-B mechanism ESTABLISHED structurally (investigator, file:line evidence): (i) the cancel window is exactly the `FGroup_EndPlay` slot of the destroy-initiating tick — `ck::IsValid` excludes only Teardown/Destroyed (`CkHandle.cpp:202-210`), so a missed window → `Get_IsRequestHandleValid()` false → `FireCancelledForPending` short-circuits SILENTLY (matches the observed no-result timeout); (ii) the race arm placement of EntityScript BeginPlay (main pass vs pump) decides drained-`Succeeded` vs cancelled (`CK_IGNORE_PENDING_KILL` deliberately excludes `Initiate`, `CkEntityLifetime_Fragment.h:27-32`); (iii) the request CHILD entity (created under the owner) can cascade-die with its signal binding before the cancel processor reads it — delegate silently lost. The test cannot construct determinism from AS (options 1-2 rejected with evidence; option 3 = exclude destroy-`Initiate` owners from request drains is the only deterministic-contract construction). Third factor (which arm swallowed the observed no-result) still unpinned — breadcrumb+loop experiment dispatched. TRANSPORT PIVOT question raised to maintainer (delegate-on-request-struct internal transport vs patching the signal window) | one-shot window + cascade fragility are properties of binding the delegate to a request entity that dies with its owner | maintainer transport ruling + breadcrumb experiment |
| INV-A | 2026-07-25 | OPEN: `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures` hangs at its deliberate-ensure under the gate tree (A/B-confirmed ours; cancel processor ruled out). Orchestrator falsified the DynamicHandleTypes.json hypothesis (CkFoundation Script/ clean; CkTests bootstrap present/unmodified). NOTE: executor's stash-out control removed BOTH the C++ changes AND the 4 new test actors from the shared autotest map — map contamination is a live suspect the A/B did not separate. Suspects: (d) new test actors in shared world, (a) generator-emitted AS defaults, (b) guard epilogue, (c) utils bind. Discriminating experiments dispatched E1→E3 | one mechanism must explain: baseline pass, gate hang, stash-rebuild pass | resolved by E1-E3 |
| G0-D11 | 2026-07-25 | RULED (STOP #3): immediate mutators (no queue, no request struct — e.g. `Request_ChangeCountDirection`, `Request_ReverseDirection`) take the delegate and fire it SYNCHRONOUSLY at the Utils boundary via `InDelegate.ExecuteIfBound(Owner, Succeeded)` — no request entity, no signal, no populate. Uniform caller contract, zero machinery. Precedent for every immediate mutator framework-wide | Caller contract ("bound delegate fires exactly once") must not depend on the feature's internal deferral shape | — |

## Dated entries (append-only, newest first)

### 2026-07-26 — GATE 00 CLOSED: engine fix verified end-to-end; boundary ritual + Gate 01 baseline (orchestrator: Fable 5)

**Fix verification (G0-D21):**
- Build+test single shot → build GREEN (patch compiled: 2 `AngelscriptManager.cpp` hits in
  `Saved/Logs/Gate00-FixVerify.log`) → suite `Total: 876, Passed: 876, Failed: 0` (9m43s);
  D-test and ResetMidFlight both `Result={Success}`; watcher exited "no hang".
- **Socket-level proof:** during a live unattended editor (isolated run, `PortCheck.log`, exit 0),
  `netstat` shows port 27099 (AS debug default, `AngelscriptSettings.h:169`) NOT BOUND by any
  process — the debug server is never created. Pre-fix, the captured hung process demonstrably ran
  its listener threads (INV-A6 stacks). Fix VERIFIED: compiled → suite green → socket absence.

**Exit criteria (GATE_00_Infrastructure.md:201-211), each with evidence:**
1. Compile + full `--test` green, Total = baseline+4, delta-zero reds — VERIFIED:
   876 = 872+4, zero fails (`Gate00-FixVerify.log`; also 876/876 ×3 earlier today). Deviation
   from the criterion's invocation spec: `--discover-fresh` omitted — ruled unnecessary (cache
   provably current: Total 876 with all 4 campaign tests by name; stale-cache signature absent).
2. 4 campaign tests green — VERIFIED (inside every 876/876; zero Fail rows). AS-surface step-6
   evidence — VERIFIED-BY-RECORD (Gate 00 second-pass dated entry, 2026-07-25/26).
3. Step-7 doc updates — VERIFIED: CkFoundation/CLAUDE.md "Request completion" section live
   (G0-D13 shape); `Source/CkEcs/Claude.md` + `Source/CkTimer/Claude.md` modified in tree.
4. PROGRESS.md dated entries — this file.
5. Orchestrator integration — orchestrator ran/spot-checked every gate itself.

**Gate 01 baseline (all working-tree state, NOTHING committed anywhere):**
- Suite: **876/876, zero failing tests** (`Saved/Logs/Gate00-FixVerify.log`, 2026-07-26).
- CkFoundation `e126c98e0` + campaign change-set uncommitted; CkTests `71612e50` + G0-D17 test
  fix + 4 campaign tests uncommitted; engine `40d97e26fbbe` on maintainer's
  `feat/debug-auto-discovery` + our G0-D21 one-liner uncommitted (maintainer WIP dirty files
  present — untouched).
- Binaries: Development, post-engine-patch (built this entry).

**Session routing note (meta-campaign):** orchestrator = Fable 5 inline this session (executor
sub-agent lost to the overnight session gap; investigation/capture/fix work was judgment-tier
throughout — routing justified). Sub-agent use resumes at Gate 01 execution.

### 2026-07-26 — HANG CAPTURED AND MECHANISM NAMED: AS debug-client break parks the game thread (orchestrator: Fable 5)

- Hang hunt (G0-D19 driver, cap 3): run 3 = 876/876 clean, run 4 = 876/876 clean, **run 5 = D-test
  HANG at 16:56 → watcher fired**: 40s-silence trigger, non-invasive cdb attach, all-thread stacks
  (319KB, full local symbols) + 9GB `.dump /ma`, all before any kill. Bonus: the hang's
  `AudioMixerPlatformInterface Timeout` warning spam kept resetting the toolbox idle window, so
  the editor was STILL HUNG 36 min later — enabling a second live snapshot: IDENTICAL GameThread
  frames/addresses at t+35min (parked in `select()`, not spinning).
- Mechanism (full verbatim stack in INV-A6): deliberate AS exception in the D-test's DoBeginPlay
  (`asCContext::SetInternalException` from the CkDynamic reject path) → AS debug-server break →
  game thread blocks in `WS2_32!select` awaiting the attached debug client (VSCode auto-attach,
  `bAutoOpenVSCode=True`). No client response → parked indefinitely. Every historical
  observation is explained (intermittency = attach roulette; alive networking threads; the
  morning's debug-client-removal line; D14-active hangs; peer non-correlation).
- **Campaign definitively exonerated** — engine debug-server behavior + a test that deliberately
  raises an AS exception; orthogonal to the completion-delegate change-set.
- Orchestrator killed the parked run-5 editor (PID 14448, our own toolbox child — the toolbox's
  idle-kill was being defeated by the audio spam); toolbox respawn completing run 5's remainder.
- Ledger today: 876/876 ×3 (16:03 capture run, 16:41 run 3, 16:51 run 4); D-test 5 pass / 3 hang
  across all suite runs. ResetMidFlight 5/5 green in-suite post-G0-D17.
- Next: maintainer fix ruling (prevention layer for automation runs — AS debug server off /
  break-on-exception off; identifying the exact switch needs UE-AS engine source reading =
  maintainer authorization per standing rule) + Gate 00 close ruling (G0-D19's condition met:
  stacks captured, mechanism named).

### 2026-07-26 — FIRST 876/876: capture gate run fully green; no hang, no capture (orchestrator: Fable 5)

- Ran: full suite, capture watcher armed (`Saved/Logs/Gate00-CloseGate-run2.log` +
  `Saved/Logs/HangCapture/watcher.log`) → `Total: 876, Passed: 876, Failed: 0, Duration: 9m 23s`.
  Orchestrator spot-checked: `Result={Success}` for BOTH `Ck_AutoTest_Timer_ResetMidFlight` and
  `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures`; zero `Result={Fail}` rows; zero
  respawns (single editor session end-to-end). **Gate 00's numeric close target reached for the
  first time.**
- Watcher behaved exactly as designed: detected D-test start, saw it complete, exited without
  attach ("TEST COMPLETED - no hang").
- Caveats on this run as close evidence: NOT machine-quiet (maintainer's interactive
  `BusterBlockEditor.exe` open throughout — recorded confound), and INV-A4 remains OPEN:
  today's record on identical binaries with D14 active is 2 passes (01:52, 16:03) / 2 hangs
  (09:44, 15:32) — an intermittent, mechanism-unknown, in-suite-only hang. The original
  campaign-tree correlation (4/4 pre-D14 hangs vs baseline pass) is CONFOUNDED by D14's
  conditions change — the change-set is not fully exonerated, but intermittency on identical
  binaries argues a timing race over code-path determinism.
- G0-D15 (root-cause before Gate 01) technically re-activated when INV-A4 reopened — whether
  this 876/876 closes Gate 00 is the maintainer's call (D15 revisit). Fork presented.

### 2026-07-26 — G0-D17 applied + close gate: ResetMidFlight GREEN in-suite (INV-C CLOSED); D-test hang reopens INV-A4 (orchestrator: Fable 5)

- Maintainer ruled G0-D17 (threshold gate). Applied: `CkAutoTest_Timer_ResetMidFlight.as` snapshot
  gate `> 0.0` → `> 200.0` ms + header/inline comment updates. AS-only, same class name (no
  rebuild, no discovery refresh, no map churn).
- Ran: Gate 00 close gate — toolbox test-only, full suite, `--no-nullrhi`, machine verified quiet
  at launch (zero Unreal processes; CkPlugins.log lock free) →
  `Total: 876, Passed: 875, Failed: 1, Duration: 12m 42s`
  (`Saved/Logs/Gate00-CloseGate-postfix.log`).
- Confirmed: `Result={Success} Name={Ck_AutoTest_Timer_ResetMidFlight}` **in-suite** — the G0-D17
  fix held under the same warm-world conditions that failed 3/5 pre-fix. **INV-C closed**
  (mechanism confirmed → fix ruled → fix verified at the full gate).
- Sole red = the D-test hang (no `Result` row anywhere in the log — killed mid-test; toolbox
  respawned; all 518 remaining tests green). INV-A4 REOPENED with both prior mechanisms
  falsified — full evidence in the INV-A5 decision row.
- Gate 00 close target 876/876 NOT reached; the ONLY blocker is the D-test in-suite hang.
  G0-D15's root-cause-first posture re-activates. Next-step fork (capture tooling) presented to
  the maintainer.

### 2026-07-26 — G0-D16 run 2: PASS arm captured; mechanism source-anchored; decrease-detection fix REFUTED (orchestrator: Fable 5)

- Waited for sibling automation to exit (monitor on editor processes; `BusterBlockEditor-Cmd` +
  2× toolbox live at 14:45, quiet ~15:05), then ran: toolbox test-only, pattern `ResetMidFlight`,
  overlay active → `Total: 1, Passed: 1` in 1m34s. Log preserved:
  `Saved/Logs/Gate00-INVC-VeryVerbose-run2-isolated.log`.
- Confirmed: per-frame order tick-timer (14|1) BEFORE `_Timer` (16|1) — the predicted PASS-arm
  mirror of run 1's `_Timer`-first FAIL arm. Full refined mechanism + fix analysis: INV-C4 row.
  Key source anchors read this session: `CkTimer_Processor.cpp:11-15` (registration order),
  Update's OnTimerUpdate broadcast + Reset-drain's OnTimerUpdate broadcast (two OnTick fire
  points/frame), `CkTimer_Utils.cpp:196-201` (stored-chrono reads),
  `Script/CkUtils_Timer.as:5-19` (Create_Tick = 0-goal ResetOnDone timer bound to OnUpdate).
- Overlay reverted: `git checkout -- Config/DefaultEngine.ini`; verified clean.
- G0-D16 cap reached (2/2 runs). Back to the maintainer with both arms for the test-fix ruling;
  orchestrator recommendation = threshold gate (~200ms), see INV-C4 options A/B.

### 2026-07-26 — G0-D16 run 1: FAIL arm captured, INV-C mechanism CONFIRMED; D-test hung again (orchestrator: Fable 5)

- The suite executed while the orchestrator session was down (launch ~01:55, machine evidently
  slept overnight; editor sessions ran 16:2x–16:49 UTC = ~09:2x–09:49 local). The executor session
  did not survive the gap: no preserved-log copy, overlay left applied. Evidence recovered from
  `Saved/Logs/` and preserved as `Gate00-INVC-VeryVerbose-run1-sessionA-main.log` (first editor
  session, 5.6MB) + `Gate00-INVC-VeryVerbose-run1-sessionB-respawn.log` (respawn, 6.0MB).
- Ran: full test-only suite, real RHI, `CkTimer=VeryVerbose` → `**** TEST COMPLETE. EXIT CODE: -1 ****`.
  Both failures are the two known reds: (1) **D-test hang** — session A's last `Test Started` is
  `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures`; last activity 16:40:38 (frame [121]
  never advanced); toolbox killed the editor (session A log ends 16:42:42) and respawned; the
  respawn's RunTests list resumes at `DynamicFragment_RequestRemove_ClearsHas` (hung test skipped,
  no Result line — session A has ZERO `Result={Fail}` rows). (2) **`Timer_ResetMidFlight` FAIL** in
  session B — the discriminator's target arm, with 11,783 `Timer Counting Up` lines of order data.
  Session B's sole `Result={Fail}` is ResetMidFlight.
- Confirmed (INV-C3, decision row): per-frame order `669|26`(`_Timer`) → `641|36`(tick-timer),
  every frame 75–497; one `_Timer` reset drain (frame 75, end-of-frame); tick-timer self-resets
  every frame (`Chrono [0.000s out of 0.000s]`, `Reset And Resume On Done`); failure arithmetic
  exact — see the row. Window: sessionB log lines 31958–33654.
- INV-A4 wrinkle recorded, not chased: the morning run is NOT a clean quiet-machine data point —
  the 01:50 quiet verification predates the overnight sleep, and at 14:45 local
  `BusterBlockEditor-Cmd.exe` + 2× `UnrealToolbox.exe` were live (sibling automation active
  today). Zero `UpdateKnownNodes` lines in either session — churn only marks peer departures, so
  this neither confirms nor refutes a live automation peer during the hang. INV-A4's interference
  resolution stands unfalsified; a truly-quiet full suite remains the Gate 00 close gate.
- Overlay `[Core.Log] CkTimer=VeryVerbose` deliberately retained for run 2 (isolated
  ResetMidFlight → PASS-arm order; last night's 10 isolated logs predate the overlay, 0
  VeryVerbose lines). Revert after run 2: `git checkout -- Config/DefaultEngine.ini`.
- Next: launch run 2 when the sibling automation exits; then return to the maintainer with both
  arms for the test-fix ruling (G0-D16 cap reached).

### 2026-07-26 — G0-D16 discriminator: dispatch 1 aborted at the quiet-machine gate (orchestrator: Fable 5)

Executor (opus) STOPped correctly at step 1: `BusterBlockEditor.exe` PID 82228 (+ sponsored
`UnrealTraceServer.exe`) live since 09:18:35 — machine not quiet. Suite NOT run; overlay NOT
applied (`git status -- Config/DefaultEngine.ini` verified clean); nothing edited/staged.
Operational rulings on the executor's two unenumerated observations:
- **(a) `--discover-fresh` NOT required.** Discovery cache is provably current: every post-D13
  suite run reports `Total: 876` = baseline 872 + the 4 campaign tests, and those tests appear by
  name (with results) in the same logs. The stale-cache signature (green-with-old-Total) is absent.
- **(b) Dirty content is pre-existing and out-of-scope.** CkTests `__ExternalActors__/AutoTests/…`
  = the 4 campaign test wrapper actors (authored 7/25, part of the verified Gate 00 inventory);
  CkFoundation's 27 `M_CkUsf_Look_*.uasset` (7/26 01:41) belong to the CkUsf workstream — left
  untouched per stage-only-what-you-changed. The same tree already produced the 875/876 quiet run,
  so neither invalidates the discriminator.
Next: maintainer closes the BusterBlock editor (or rules "run anyway" — an interactive peer is not
the confirmed automation-peer hang condition, but a mid-suite hang+respawn would perturb the very
suite state the discriminator depends on), then re-dispatch with rulings (a)/(b) folded in.

### 2026-07-26 — INV-A4 RESOLVED: cross-session interference CONFIRMED by the decisive experiment (orchestrator: Fable 5)

Quiet-machine run (`Saved/Logs/Gate00-INVA4-QuietMachine.log`): machine verified quiet at launch
(zero Unreal processes machine-wide via Win32_Process enumeration, not user impression alone) +
live attestation monitor (15s polling for any editor process outside `D:\Repos\CkPlugins\`) —
**zero FOREIGN-PEER events through the D-test window**, and:
```
Test Started.  Name={Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures}
Test Completed. Result={Success} Name={Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures}
```
**First in-suite pass on a campaign tree under default conditions.** Correlation now 7/7: every
in-suite hang had a concurrent sibling automation session on the machine; both in-suite passes
(INV-A3, this run) were sibling-quiet. The campaign change-set is EXONERATED for the hang — the
apparent "campaign trees hang" pattern was a schedule artifact (campaign gates ran while the
sibling worktree session was active; the one baseline run happened to predate the sibling's
CkPlugins_Other suite activity). Mechanism detail (bus-message interference on multicast
`230.0.0.1:6666` between concurrent automation editors) remains INFERRED as to the exact message
path; the operational fact (concurrent automation peer ⇒ hang; quiet ⇒ pass) is CONFIRMED.
**FINAL COUNTS (quiet run):**
```
Total: 876   Passed: 875   Failed: 1   Skipped: 0   Duration: 12m 15s
```
NO hang, NO editor respawn — INV-A4 conclusively closed as cross-session interference. All 4
campaign tests green (**CancelledOnTeardown 5/5 post-D13**).

**Sole failure: `Ck_AutoTest_Timer_ResetMidFlight` — my "respawn turbulence" assessment is
FALSIFIED and withdrawn** (this run had no respawn AND no sibling; the failure survived the
cleanest conditions of the campaign). Tally: post-D13 suite runs 3 fails / 5 runs
(D13-verification-rerun, D14, quiet) vs pre-D13 0 / 4. A Timer test + Timer-touching change-set +
a post-D13 fail-rate shift ⇒ REGRESSION CANDIDATE, opened as **INV-C**, now the sole Gate 00
blocker. Orchestrator's candidate mechanism (unproven, recorded for the investigator): the test's
success window is inherently ~1 frame wide — it snapshots elapsed at the first tick where
elapsed>0 (≈ one frame's worth), and after the deferred Reset applies, elapsed climbs back past
that snapshot within ~1 frame, so the poll (`NowMs < _ElapsedAtResetMs`) can only succeed if
OnTick observes within that window; any 1-frame shift in when the drain applies Reset relative to
the tick-signal fire closes the window. D13 did not obviously add such a shift (unbound-delegate
path is behaviorally identical pre/post on paper) — measurement needed, not more paper analysis.

### 2026-07-26 — G0-D14 gate: details-policy mechanism FALSIFIED (executor: opus; orchestrator: Fable 5)

D14 override implemented exactly per spec (single file, `CkAutoTestRunner.cpp`: `GOriginalDetailsPolicy`
global, capture+override in `Install_EnsurePolicyOverride`, restore in `Restore_EnsurePolicyOverride`
+ `Force_Restore_OnEnginePreExit`, shared refcount/PreExit handle). Gate (`Saved/Logs/Gate00-D14.log`):
```
Total: 876   Passed: 874   Failed: 2   Skipped: 0   Duration: 13m 30s
```
- **FALSIFICATION, confirmed:** both override lines logged once per process
  (`Overriding ensure details policy: MessageAndStackTrace -> MessageOnly` at 06:56:43), zero
  restores before the hang, DynamicFragment started 06:59:48 — three minutes inside the active
  override window — and HUNG with the identical silent-kill signature. Details policy was
  `MessageOnly` at the ensure moment; the fully-symbolicated-`StackWalkAndDump` mechanism is DEAD.
  **INV-A4 is now 5/5 deterministic in-suite hangs on campaign trees.**
- **Evidence matrix is now confounded 2×2:** the one in-suite PASS with reduced details (INV-A3
  config-overlay run) was on PRE-D13 binaries with the policy set via config CVar at boot; this
  HANG is on POST-D13/D14 binaries with the policy set at runtime by the runner. Binaries vs
  set-mechanism cannot be separated from existing data.
- **Severity re-read (orchestrator):** with slow-symbolication falsified, "the campaign change-set
  causes a real in-suite hang/deadlock in this test's scenario" is a live hypothesis again —
  candidate deltas: generator-emitted AS defaults (touches every AutoCreateRefTerm-delegate
  wrapper across modules, incl. Inventory), `FCk_Request_Base` layout growth (every request
  struct now carries a TScriptDelegate), the 4 new map actors (cleared for ISOLATED runs only by
  INV-A2's Confirm4, never for in-suite).
- Cheapest next discriminator identified: isolated D-test on CURRENT (D14) binaries — INV-A2's
  4/4 isolated passes were all pre-D13. Isolated hang ⇒ binaries regression (locally bisectable,
  fast). Isolated pass ⇒ in-suite-state-dependent ⇒ subset bisection over the preceding test set.
- Secondary: `ResetMidFlight` failed again in the respawned editor (2 fails / 3 respawned editors —
  intermittent WITHIN respawns, its 4 Timer neighbours in the same respawn all passed; still
  assessed respawn turbulence, INFERRED). Campaign tests green again — **CancelledOnTeardown 3/3
  post-D13.** The D14 edit itself is sound and kept in-tree regardless (removes a real per-process
  stack-walk cost in autotest runs).
- Gate 00 exit still NOT met. Maintainer re-ruling being raised.

### 2026-07-26 — Verification rerun: 874/876 again; INV-A4 opened; ResetMidFlight one-off assessed (orchestrator: Fable 5)

Test-only rerun on the D13 binaries (`Saved/Logs/Gate00-D13-Rerun.log`):
```
Total: 876   Passed: 874   Failed: 2   Skipped: 0   Duration: 20m 12s
```
- **All 4 campaign tests GREEN again — `CancelledOnTeardown` is 2/2 under G0-D13** (vs 1/3 under
  the signal transport). The transport pivot's determinism claim now has two consecutive
  full-suite confirmations.
- **DynamicFragment hung AGAIN** → the "environmental flake" call is WRONG and dropped: 4/4
  deterministic in-suite hangs on campaign trees under default policy. Reframed as **INV-A4**
  (see decision log) — this is now THE gate-blocking defect; it will red every future gate until
  ruled/fixed.
- Second failure this run: `Ck_AutoTest_Timer_ResetMidFlight` — runner watchdog
  ("engine TimeLimit elapsed without an AS-side result... timed out in 5.015 seconds"), ran 5 min
  into the RESPAWNED editor (post-hang kill; respawn ran the D-onward 518 tests, second
  "Found 518" at 06.31.28). Assessed ONE-OFF, INFERRED not confirmed, on this evidence: (i) the
  test (read in full, `CkAutoTest_Timer_ResetMidFlight.as`) never destroys an entity and passes
  no delegate — the unbound-delegate path never calls `Set_CompletionDelegate`, its owner never
  carries Initiate, so the G0-D13 surface is unreachable from it; (ii) it PASSED on these exact
  binaries in the executor's gate 40 min earlier and in every prior suite run; (iii) the
  respawned-editor turbulence mechanism (INV-A4 row) explains both this and the executor run's
  CkJolt/Zen one-off, each occurring exactly once, different test each time.
- **Gate 00 exit: NOT met** — delta-zero fails on the deterministic DynamicFragment red.
  Everything else holds (876 discovered, campaign tests 2/2 green, no reproducible Timer
  regression). Maintainer fork on INV-A4 being raised: harness details-policy override for
  autotest runs vs full root-cause first.

### 2026-07-26 — G0-D13 IMPLEMENTED; gate 874/876, both failures environmental; verification rerun in flight (executor: opus; orchestrator: Fable 5)

**VERIFY-STEP (the contract's gate): PROCEED — CONFIRMED.** `ck::FTag_DestroyEntity_Initiate` is
applied synchronously on the `Request_DestroyEntity` call stack
(`CkEntityLifetime_Utils.cpp:63-64`, `InHandle.AddOrGet<ck::FTag_DestroyEntity_Initiate>();` — no
processor, no deferral), corroborated by `CkProcessorGroups.h:34-38` and by the EndPlay-phase
processor CONSUMING Initiate (`CkEntityLifetime_Processor.h:37-40`). The exclusion-filter
determinism argument stands on verified ground.

**Implementation complete per Addendum 2** (orchestrator spot-checked `CkRequest_Completion.h`
in full + the drain guard at `CkTimer_Processor.cpp:59` + the view exclusion at
`CkTimer_Processor.h:43` + 7 `Set_CompletionDelegate` sites — all match the contract):
signal block deleted; `TCompletionGuard`/`MakeCompletionGuard` added; `FireCancelledForPending`
fires unconditionally (exactly-once lives in `TryFireCompletion`'s unbind); `FCk_Request_Base`
carries the non-reflected `_CompletionDelegate` + 3 const methods; three doc sections rewritten.
**Correction to Addendum 2:** "9 deferred Request_*" was wrong — 7 are deferred bind sites; the
other 2 of the 9 are the G0-D11 immediate mutators (already `ExecuteIfBound`, untouched). Residue
greps clean (signal symbol 0 hits in source+docs-in-source; 13 remaining hits are all under
`docs/campaigns/` = the campaign's own history, correct to keep). AS tests verified-by-read to
need no edits. Adjacent finding recorded, not chased: stale header comment in
`CkAutoTest_Timer_RequestCompletion_CancelledOnTeardown.as:9-11` (describes the old
CK_IGNORE_PENDING_KILL mechanism).

**GATE (executor run, build green, `Saved/Logs/Gate00-D13.log`):**
```
Total: 876   Passed: 874   Failed: 2   Skipped: 0   Duration: 17m 13s
```
- **All 4 campaign tests GREEN — `CancelledOnTeardown` GREEN** (was 1/3 under signal transport).
  G0-D13's determinism prediction supported on the predicted side.
- Failure 1: DynamicFragment hang — exact INV-A3 environmental signature (enumerated branch).
- Failure 2 (UNENUMERATED → executor STOPped, correctly):
  `Ck_AutoTest_CkJolt_ChaosParity_KinematicPlatformCarry` — no assertion failure; sole attributed
  error is an escalated `LogZenServiceInstance` Warning (ZenServer died + self-recovered inside
  the test window; harness Warning-escalation). All sibling ChaosParity tests passed; CkJolt/CkChaos
  untouched by the change-set. INFERRED environmental.
- **Orchestrator ruling:** test-only verification rerun (no `--build`, no `--config`,
  `--no-nullrhi`) to discriminate both failures at once — doubles as the orchestrator's own gate
  re-run per campaign discipline (`Saved/Logs/Gate00-D13-Rerun.log`, in flight). Branches:
  876/876 → Gate 00 exit criteria met, record both as environmental flakes. Either failure
  repeats → it is not environmental for this tree; reopen the matching INV.

### 2026-07-26 — INV-A3 experiment RESULT: prediction held (orchestrator: Fable 5)

Full suite, test-only, gate-state source + binaries, `ck.ensure.DetailsPolicy=0`
(`Saved/Logs/Gate00-INVA3-DetailsPolicy0.log`; queued ~50 min behind the sibling BusterBlock
lock, then ran with the machine otherwise free):
```
Total: 876   Passed: 875   Failed: 1   Skipped: 0   Duration: 15m 0s
```
- **`Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures`: PASSED IN-SUITE** — completed in
  ~2.06 s (Started 05.27.25:998 → Success 05.27.28:060). Previously 2/2 in-suite HANGS under the
  default policy. The recorded prediction held on the predicted side.
- **INV-A3 verdict: SUPPORTED, adopted as the working explanation** — the hang is the
  first-ensure-per-process fully-symbolicated `StackWalkAndDump` stalling under I/O load, an
  ENVIRONMENTAL effect, not a campaign source regression. Residual confound, stated honestly:
  this run's machine was not I/O-shared during the suite (the sibling finished before we
  started), while the two hanging runs were — DetailsPolicy was not the only changed variable.
  Not worth another 15-min discriminating run: no campaign code is implicated either way.
  Revisit only if the hang recurs under default policy on an idle machine.
- **The ONLY failure: `Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown`** — identical
  signature to the prior flake, verbatim: `FinishTest TestResult=Failed. AutoTestRunner: engine
  TimeLimit elapsed without an AS-side result. ... Test timed out in 4.005 seconds`. Now
  1 pass / 2 fails across three suite runs — exactly the INV-B2 silent-void mechanism G0-D13
  eliminates. The other 3 campaign tests green in-suite.
- Overlay reverted after the run: `git checkout -- Config/DefaultEngine.ini` → Config/ clean
  (only the foreign DefaultGameplayTags.ini change remains).

### 2026-07-26 — Tree state VERIFIED; INV-A3 experiment launched (orchestrator: Fable 5)

Post-limit resume. Verification per the standing warning, all VERIFIED this session:
- `git status` all three repos: CkFoundation + CkTests dirty sets match the Gate 00 inventory
  EXACTLY (executor's edits + 4 new AS tests + 4 `__ExternalActors__` uassets +
  `CkTests_AutoTestActors.as`). The mid-edit death left no partial source state.
- `grep TEMPORARY-DIAGNOSTIC` repo-wide → exactly ONE hit: `Config/DefaultEngine.ini:79`
  (the experiment overlay comment). ZERO in any source/script file — the executor died before
  applying breadcrumb edits, so the binaries correspond to gate-state source.
- The overlay `ck.ensure.DetailsPolicy=0` (+ a BOM on line 1) is the only campaign-relevant
  superproject change. Undo = `git checkout -- Config/DefaultEngine.ini` (scheduled after the run).
- Foreign side-effects, left untouched, excluded from any staging: `Config/DefaultGameplayTags.ini`
  (upstream Dialog autotest tags, editor auto-registration) and 27
  `CkFoundation/Content/CkUsf/GeneratedLooks/*.uasset` (editor re-saves).

INV-A3 experiment launched with the overlay in place — full suite, test-only invocation
(no `--config`), `--no-nullrhi`, editor lock probed FREE first. This is the maximally comparable
run: same gate-state source that hung DynamicFragment 2/2 in-suite, differing only in the CVar.
**Prediction (recorded before result):** DetailsPolicy 0 skips the fully-symbolicated
first-ensure `StackWalkAndDump` → the D-test completes; a hang under DetailsPolicy 0 FALSIFIES
the stack-walk mechanism and INV-A3 reopens.

### 2026-07-25 — INV-A investigation (executor: opus)

**FORENSICS (requested verbatim).** In the PASSING isolated run the very next line after the
ensure-policy override — 1.175 s later — is the test's own completion. There is NO intermediate
subsystem line to bisect against:
```
[2026.07.26-02.18.34:282][809]LogCkAutoTest_Ensure: Display: Overriding ensure display policy: ModalDialog -> LogOnly for AutoTest run
[2026.07.26-02.18.35:457][812]LogAutomationController: Display: Test Completed. Result={Success} Name={Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures} ...
```
The hanging run emits nothing whatsoever after the override. The stall is inside the test body, in
the ~1.2 s window between runner start and completion — no subsystem is named because the passing
run never logs one.

**E1 — PASS (1/1).** Gate binaries kept; the 4 `CkAutoTest_Timer_RequestCompletion_*.as` moved to
scratchpad, the 4 populator-placed `Content/__ExternalActors__/…` uassets moved out, and
`Script/Generated/CkTests_AutoTestActors.as` restored to HEAD. Isolated
`Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures` then PASSED. Per the stop-at-first-PASS
rule, E2 and E3 were NOT run. Confirmed the lever works: the populator did not re-add the actors.

**Bisection did NOT converge — the effect stopped reproducing.**
| Run | Test actors present | DynamicFragment isolated |
|---|---|---|
| Flake-AB (pre-E1) | all 4, map as baked by gate run 1 | **HUNG** |
| E1 | none | PASS |
| BisectB | CancelledOnTeardown + ImmediateMutatorFiresSync | PASS |
| BisectA1 | + SucceedsOnDrain (3 of 4) | PASS |
| Confirm4 | **all 4 restored** | **PASS** |

Restoring all four actors did NOT bring the hang back. **This falsifies "a specific test actor
causes it" AND retroactively undermines the earlier A/B**: the control-rebuild PASS is equally
explained by the map having been regenerated. The variable that actually changed at E1 was not
"which actors" but **the derived map/generated-wrapper state**, which every boot since has
rewritten. Leading (UNPROVEN) mechanism: gate run 1 was the FIRST boot in which these wrappers were
generated, so actors were placed during the same boot that first emitted their classes — a
first-generation ordering artifact (cf. `ck-angelscript-interop` catalog items 3/9). Every later
boot loads pre-existing wrappers and places consistently.

**FULL-SUITE RERUN (gate binaries, full gate state) — the decisive datum:**
```
Total: 876   Passed: 874   Failed: 2   Skipped: 0   Duration: 13m 7s
```
| Test | Gate run 1 | Gate rerun | Isolated (gate binaries) |
|---|---|---|---|
| `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures` | HUNG | HUNG | PASS ×4 (post-E1) |
| `Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown` | **PASS** | **FAIL (timeout)** | not isolated |

So DynamicFragment hangs **2/2 in the full suite** and passes **4/4 in isolation** → an ordering /
shared-world effect, NOT a source regression attributable to a single hunk. E2/E3's hypotheses
(generator emission, result guard) are not supported by any evidence gathered.

**NEW FINDING — the executor's own test is FLAKY, and it is the gate's load-bearing unknown.**
`Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown` passed in gate run 1 and FAILED in the
rerun. Verbatim:
```
[2026.07.26-03.03.26:044][107]LogAutomationController: Error: Test Completed. Result={Fail} Name={Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown} ...
[2026.07.26-03.03.26:044][107]LogAutomationController: Error: Ck_AutoTest_Timer_RequestCompletion_CancelledOnTeardown: FinishTest TestResult=Failed. AutoTestRunner: engine TimeLimit elapsed without an AS-side result. Did the AS test crash before its timer started?. Test timed out in 4.006 seconds [D:\Repos\CkPlugins\Plugins\CkTests\Source\CkTests\Private\CkAutoTestRunner.cpp(254)]
```
This is exactly the gate doc's flagged branch — "`CancelledOnTeardown` → delegate never fires → the
cancel path isn't reached during teardown; STOP, report verbatim". The `Failed_Cancelled` delegate
did not fire within the 4 s budget on that run. Whether the cause is the cancel path itself
(`FProcessor_Timer_CancelPendingRequests` / `FireCancelledForPending`) or the destroy-same-frame
timing the test relies on is UNDETERMINED — it is intermittent, so a single green run does not
clear it. **Not fixed by the executor: designing the cancel-path or test-timing fix is outside the
"mechanically obvious" bar.**

**INV-A verdict:** the DynamicFragment hang is NOT attributable to a specific source hunk on the
evidence available; it is full-suite-ordering-dependent and did not survive map regeneration. The
genuinely actionable defect surfaced instead is the intermittent `CancelledOnTeardown` timeout
above. All temporarily-moved artifacts were restored; the tree is in full gate state; scratchpad
backups retained at `…/scratchpad/E1/`.

### 2026-07-25 — Gate 00 third pass: BASELINE CAPTURED, gate run RED on one test (executor: opus)

**Submodule fast-forwards (step 0)** — all clean ff-only, no conflicts on the merges themselves:
| Submodule | Before | After |
|---|---|---|
| CkGameplayDebugger | `3be8b64d` | `a6585c53` (contains `d34bad6` Aggro v2 fix — verified ancestor) |
| CkFoundation | `940d255b` | `e126c98e` |
| **CkTests** (NOT in the instruction — see below) | `182b1438` | `71612e50` |

**Executor-initiated action beyond the instruction, flagged for ratification:** the first baseline
attempt at the ff'd CkFoundation FAILED at AS compile — CkTests@182b1438 was pinned behind the new
CkFoundation API (`No matching signatures to 'utils_poi_display_definition::Create(...)'`,
`...::TryGet_PoiDisplayDefinition_ByConsumer(...)`, plus CkAggro equivalents). CkTests was 14
commits behind with a clean ff available, and upstream `fbfd2812 test(CkPoi): migrate the POI suite
to the typed Create/TryGet API` + `ddf15701 test(CkAggro): migrate the 13 autotests to the
self-sufficient-target API` are exactly those fixes. Fast-forwarded it — same reversible operation
class already authorized twice. **Undo:** `git -C Plugins/CkTests reset --hard 182b1438`
(nothing staged or committed; superproject gitlinks untouched).

**G0-D9b applied**
- All 9 `= FCk_Delegate_Request_OnCompleted()` C++ defaults removed from `CkTimer_Utils.h`;
  `meta = (AutoCreateRefTerm = "InDelegate")` retained (exact CkInventory shape).
- Stash-pop conflict resolved in `CkTimer_Utils.h` exactly as predicted (upstream palette
  Category/DisplayName kept, executor params/meta kept). No other conflicts in either submodule.
- G0-D11 applied to both immediate mutators; 4th AutoTest authored.
- **CkAngelscriptGenerator extended** (`CkAngelscriptWrapperGenerator.{h,cpp}`): new
  `Get_IsOptionalDelegateParameter` hooked into `Get_DefaultValueForProperty`, emitting
  `= <DelegateType>()` for a delegate param named in the function's `AutoCreateRefTerm`.
  **First attempt broke the AS surface**: emitting on ANY such delegate produced
  `All subsequent parameters after the first default value must have default values in function
  'TArray<FCk_Handle_ByteAttribute> ForEach_If(FCk_Handle, FInstancedStruct, FCk_Lambda_InHandle = FCk_Lambda_InHandle ( ), FCk_Predicate_InHandle_OutResult)'`
  across 9 generated wrappers (ForEach_If / ForEach_ValidEntry_If families) — AS requires every
  parameter after the first defaulted one to also carry a default. **Fix:** restrict emission to the
  TRAILING parameter only, which is the house shape anyway. Second attempt compiled and the full
  AS surface booted clean.
- C++ call sites passing `{}`: `CkCue_EntityScript.cpp` (2), `CkTween_Processor.cpp` (1),
  `CkTimer_Processor.cpp` (6), `CkTimer_Fragment.cpp` (4),
  `Test_Snapshot_TimerParity_MPReload_Gate.spec.cpp` (2). No further ones surfaced.

**BASELINE — CAPTURED (first time this campaign).** Clean-HEAD tree (both submodules stashed to
empty, verified), command:
`./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --test --no-nullrhi --discover-fresh --output=Saved/Logs/Gate00-Baseline.log --project="D:\Repos\CkPlugins"`
```
=== Build succeeded ===
=== Test summary ===
Total: 872   Passed: 872   Failed: 0   Skipped: 0   Duration: 9m 51s
```
**Failing set at baseline: EMPTY.** (The memory-noted "~10/18 crowd tests deterministically red" is
NOT true at this HEAD — upstream `a18a2465 fix(tests,gyms): one entity per crowd agent` fixed them.)

**GATE RUN — build green, Total correct, ONE regression.**
`--build --config=Development --target=Editor --test --no-nullrhi --discover-fresh` →
```
=== Build succeeded ===
=== Test summary ===
Total: 876   Passed: 875   Failed: 1   Skipped: 0   Duration: 12m 51s
```
- **Total = baseline + 4 ✓** — all four new tests discovered and listed by name.
- **Failing test: `Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures`** — NOT one of the four
  new tests, and unrelated to CkTimer/CkEcs-completion by subject.
- Failure mode is a HANG, not an assertion. It is the only test that Started without a Completed
  record (875 `Result={Success}` for 876 total). Verbatim:
```
[2026.07.26-02.00.32:128][535]LogAutomationController: Display: Test Started. Name={Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures} ...
=== utb: editor produced no output within the idle window — treated as HUNG (stalled AngelScript compile / modal dialog / deadlock). Killed it; failing the run. ===
[utb --test] main: '...Ck_AutoTest_DynamicFragment_ReplicateEmptyStruct_Ensures' was in flight when the editor died (exit=0x1 (decimal 1)) — marked Failed, resuming after it
```
  The last line before the hang is always
  `LogCkAutoTest_Ensure: Display: Overriding ensure display policy: ModalDialog -> LogOnly for AutoTest run`
  — i.e. it hangs as the test triggers its deliberate ensure.

**A/B — this is the executor's regression, NOT a flake. VERIFIED, not inferred:**
| Run | Tree | Isolated result |
|---|---|---|
| Full baseline | clean HEAD | **PASS** (inside 872/872) |
| Isolated, gate binaries | executor changes | **FAIL** — hang |
| Isolated, control REBUILD | executor changes stashed out | **PASS 1/1** |
| Isolated, gate minus cancel processor | `CK_REGISTER_PROCESSOR(FProcessor_Timer_CancelPendingRequests)` commented out, rebuilt | **FAIL** — hang |

The control was a full rebuild (not a test-only run) precisely so it did not reuse gate binaries.

**Discriminating experiment result: `FProcessor_Timer_CancelPendingRequests` is RULED OUT** — the
hang reproduces with it unregistered. Do not re-investigate it. The diagnostic comment-out was
reverted; zero residue (`grep TEMPORARY-DIAGNOSTIC` → clean).

**STOP — two attempts spent on this step, root cause not isolated.** Remaining suspects, untested:
(a) the CkAngelscriptGenerator default-emission change — it alters the generated AS signature of
every `utils_*` function whose trailing param is an AutoCreateRefTerm delegate, far beyond CkTimer;
(b) the `ck::MakeRequestResultGuard` epilogue in `FProcessor_Timer_HandleRequests`;
(c) the Utils-boundary `PopulateRequestHandle` + bind. Note (a) is the only change with
framework-wide reach and is the natural next probe (revert just the generator hunk, rebuild, re-run
the isolated test). AS compiles clean under it, so any fault is runtime, not signature-level.

**Exit criteria NOT met:** the failing set is baseline (∅) + 1, so delta-zero fails. Everything else
(build green, Total = baseline + 4, all four new tests present) holds.

### 2026-07-25 — Gate 00 second pass, post-engine-fix (executor: opus)

Engine selection VERIFIED fixed: UBT resolved `D:\Repos\UnrealEngineAngelscript` and ran to
completion. Both submodules' dirty state re-verified as exactly the executor's own edits, nothing
foreign. Machine-wide build lock and editor lock probed before every run; a second sibling session
(`sulfu#79712`, `E:\Repos\BusterBlock_Other`) held a lock on a DIFFERENT engine
(`UnrealEngineAngelscript_Other`) and was never stomped.

**BLOCKER 1 — G0-D9 IS REJECTED BY UHT. STOP per the orchestrator's instruction.**

The gate build ran UHT and failed there, before compiling a single TU. Verbatim (9 occurrences —
one per `Request_*` UFUNCTION, at `CkTimer_Utils.h` lines 191, 200, 209, 218, 227, 237, 247, 259, 269):

```
D:\Repos\CkPlugins\Plugins\CkFoundation\Source\CkTimer\Public\CkTimer\CkTimer_Utils.h(191): Error: C++ Default parameter not parsed: InDelegate 'FCk_Delegate_Request_OnCompleted()'
...
Result: Failed (OtherCompilationError)
=== Build FAILED ===
```

Command: `./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --output=Saved/Logs/Gate00-Build.log --project="D:\Repos\CkPlugins"`

- VERIFIED: UHT cannot parse a dynamic-delegate default value. The
  `AutoCreateRefTerm` + `= FCk_Delegate_Request_OnCompleted()` shape **cannot ship**.
- INFERRED (not verified): the cause is structural, not syntactic — UHT only translates default
  values it can turn into a Blueprint pin default (literals, enums, a fixed set of known structs);
  a dynamic delegate has no such parser. Consistent with the earlier census finding that ZERO
  UFUNCTIONs in CkFoundation give a delegate parameter a default.
- **Consequence for the campaign:** PROMPT.md success criterion #1 ("defaultable in AS/C++")
  is NOT ACHIEVABLE via a UFUNCTION default. The two remaining shapes both carry costs the
  orchestrator must weigh, and both are outside the executor's authority:
  (a) **No default** (the CkInventory shape) — every C++ and AS caller must pass a delegate
  explicitly. Known blast radius for CkTimer alone: ~30 AS call sites in CkTests, plus C++ callers
  in **CkCue** (`CkCue_EntityScript.cpp:59-60`), **CkTween** (`CkTween_Processor.cpp:306`),
  CkTimer's own processor/fragment, and `Test_Snapshot_TimerParity_MPReload_Gate.spec.cpp:149-150`.
  CkCue and CkTween are outside this gate's scope fence.
  (b) **An overload pair** (delegate-less + delegate-taking) — explicitly forbidden by the gate's
  own fences ("Do NOT add back-compat overloads or `_WithCallback` variants") and by G0-D4.
- The rejected shape is LEFT IN THE TREE as the measurement artifact. **The tree therefore does
  not currently build.** No workaround was improvised.

**BLOCKER 2 — the baseline cannot be captured: clean HEAD does not compile.**

Before the gate build, the executor's changes were stashed out of both submodules
(`git status --porcelain` empty in both, VERIFIED) and a full build+test was attempted:

```powershell
./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --test --no-nullrhi --discover-fresh --output=Saved/Logs/Gate00-Baseline.log --project="D:\Repos\CkPlugins"
```

It reached the compile phase (517 TUs) and failed. Unique errors, verbatim — **none of these are
in CkEcs, CkTimer or CkTests, and all were produced with the executor's changes absent**:

```
D:\Repos\CkPlugins\Plugins\CkFoundation\Source\CkUI\Public\CkUI\CkUI_Utils.cpp(310,70): error C2248: 'FSlateUser::GetCursor': cannot access protected member declared in class 'FSlateUser'
D:\Repos\CkPlugins\Plugins\CkFoundation\Source\CkUI\Public\CkUI\CkUI_Utils.cpp(332,17): error C2248: 'FSlateUser::LockCursor': cannot access protected member declared in class 'FSlateUser'
D:\Repos\CkPlugins\Plugins\CkFoundation\Source\CkUI\Public\CkUI\CkUI_Utils.cpp(350,17): error C2248: 'FSlateUser::UnlockCursor': cannot access protected member declared in class 'FSlateUser'
D:\Repos\CkPlugins\Plugins\CkGameplayDebugger\Source\CkEcsDebugger\Public\CkEcsDebugger\Inspectors\CkInspector_Aggro.cpp(7,32): fatal error C1083: Cannot open include file: 'CkAggro/CkAggroOwner_Fragment.h': No such file or directory
D:\Repos\CkPlugins\Plugins\CkGameplayDebugger\Source\CkEntityDebugOverlay\Private\Providers\CkDebugOverlay_Provider_Aggro.cpp(8,32): fatal error C1083: Cannot open include file: 'CkAggro/CkAggroOwner_Utils.h': No such file or directory
```

Two independent pre-existing breaks on `CkFoundation@940d255` + `CkGameplayDebugger@<pinned>`:
CkUI has drifted against the current engine's `FSlateUser` access specifiers, and CkGameplayDebugger
includes `CkAggro` headers that do not exist at the pinned CkFoundation SHA (cross-submodule pointer
mismatch). Both are outside this gate's scope fence. **No baseline numbers exist; no `--test` phase
ran in either attempt; Total/Pass/Fail are unavailable and the delta-zero exit criterion cannot be
evaluated until these are fixed by their owning workstreams.**

**Work applied this pass (uncompiled — see BLOCKER 1)**
- G0-D11 applied: `Request_ChangeCountDirection` and `Request_ReverseDirection` now take the trailing
  delegate and fire `InDelegate.ExecuteIfBound(InTimerEntity, ECk_Request_OperationResult::Succeeded);`
  after the inline mutation. No request entity, no signal, no populate.
- 4th AutoTest authored: `CkAutoTest_Timer_RequestCompletion_ImmediateMutatorFiresSync.as` — binds a
  delegate to `Request_ChangeCountDirection` and asserts on the SAME stack frame that the delegate
  already fired with `Succeeded` and the direction flip already applied.
- `Source/CkTimer/CLAUDE.md` updated: the immediate-mutator paragraph now documents the G0-D11
  synchronous-fire shape instead of stating those two are excluded.
- STOP #2 (G0-D10) honoured: no validation added to CkTimer, no `FailedNotEnqueued` test.

### 2026-07-25 — Gate 00 execution (executor: opus)

**Entry criteria**
- `git -C Plugins/CkFoundation status --porcelain` and `git -C Plugins/CkTests status --porcelain`
  were both EMPTY at session start. VERIFIED. No foreign dirty paths in either submodule.
  HEADs: CkFoundation `940d255baabf7caeb4f18d4b6b0ff0564308b126` (dev),
  CkTests `182b1438be4794df590fbc5ec688f1edd1105d71` (dev). Superproject carries a pre-existing
  unstaged CkFoundation gitlink bump (`4be6141` → `940d255`) and untracked `Content/Maps/` —
  both foreign, left untouched.
- Anchors re-verified on current HEAD. ALL THREE VERIFIED:
  `CkRequest_Data.h:136-168` = `ck::TRequestResultGuard` + `MakeRequestResultGuard`;
  `CkSignal_Macros.h:48-49` = `CK_SIGNAL_BIND_REQUEST_FULFILLED`;
  `CkInventory_Utils.h:243-252` = `Request_AddItem` with trailing delegate +
  `meta = (AutoCreateRefTerm = "InDelegate")`.
- Editor lock free for this project (`CkPlugins.log` probe → `free`); no UnrealEditor process.
- **Machine contention: the engine-wide UnrealToolbox build lock was held for this session's whole
  window** by a sibling session (`sulfu#68192`, project `D:\Repos\BusterBlock\BusterBlock.uproject`,
  doing `Test`, started 2026-07-25 16:30:24). Waited event-driven, never stomped it.

**STOP conditions hit (verbatim observations)**

1. **Step 6 — AS optional-delegate: AS REQUIRES the argument unless the UFUNCTION declares a
   default value.** Evidence (VERIFIED, static):
   - Generated `Plugins/CkFoundation/Script/Generated/utils_inventory.as:76` —
     `Request_AddItem(FCk_Handle_Inventory InInventory, const FCk_Request_Inventory_AddItem &in InRequest, const FCk_Delegate_Inventory_OnOperationResult_Add &in InDelegate)`
     — **no default value emitted**, because the C++ UFUNCTION declares none.
   - Every AS call site in CkTests passes an explicit delegate; zero omit it. The AS idiom for
     "no delegate" is passing a default-constructed one, e.g.
     `CkAutoTest_Inventory_RequestCancelledOnDestroy.as:26` passes
     `FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic()`.
   - **The generator DOES emit defaults when the UFUNCTION declares them** (for non-delegate
     params): `utils_2d_grid_system.as:155` —
     `DebugDraw_Grid(..., const FCk_2dGridSystem_DebugDraw_Options &in InOptions = FCk_2dGridSystem_DebugDraw_Options())`;
     `utils_entity_lifetime.as:15` — `Request_DestroyEntity(FCk_Handle InHandle, ECk_EntityLifetime_DestructionBehavior InDestructionBehavior = ...)`.
   - **Zero UFUNCTIONs in CkFoundation give a DELEGATE parameter a default value** — no precedent;
     whether UHT + the AS generator accept it was UNVERIFIED at authoring time.
   - **Blast radius if no default is used:** the Inventory shape (no default) does not satisfy
     PROMPT.md success criterion #1 ("defaultable in AS/C++"). Adding a bare trailing delegate to
     CkTimer breaks, at minimum: ~30 existing AS call sites of
     `utils_timer::Request_Resume/Pause/Stop/Reset` across CkTests, plus C++ callers in
     **CkCue** (`CkCue_EntityScript.cpp:59-60`) and **CkTween** (`CkTween_Processor.cpp:306`) —
     both OUTSIDE this gate's scope fence.
   - **Executor action:** implemented WITH the default
     (`= FCk_Delegate_Request_OnCompleted()`) alongside `AutoCreateRefTerm`, because criterion #1
     mandates defaultability and it is the only shape that keeps every existing caller source-
     compatible without editing out-of-fence modules. **Orchestrator must ratify this as the
     canonical rollout shape before Gate 01** — it differs from the CkInventory reference.

2. **Step 3 / test 2 — CkTimer has NO synchronous-rejection path, so `Failed_NotEnqueued` is
   untestable there.** VERIFIED: every `UCk_Utils_Timer_UE::Request_*` body in
   `CkTimer_Utils.cpp` (pre-change) consists solely of `CK_CALLSTACK_RECORD` + `AddOrGet<...>()._Requests.Emplace(...)`
   — zero `CK_ENSURE_IF_NOT`, zero authority gate, zero validation. The gate doc's step-3 wording
   ("Existing validation ensures stay; on the sync-rejection early-out add ...") presumes
   validation CkTimer does not have. Adding one is a new behavior + a design fork:
   - House doctrine (root non-negotiable #3) requires `CK_ENSURE_IF_NOT` for an invalid handle,
     but an ensure fires the AutoTest harness's failure escalation, so the test the gate asks for
     could not be green against an ensure-based rejection.
   - CkInventory's only non-ensure sync rejection is its authority gate
     (`CkInventory_Utils.cpp:56` logs at `Display`), which CkTimer has no analogue for.
   - **Executor action:** NOT implemented. `Ck...Timer.RequestCompletion.FailedNotEnqueued` was
     NOT authored. Orchestrator must rule what CkTimer's legitimate sync-rejection is (and how a
     test observes it without tripping ensure-escalation).

3. **Step 3 — two `Request_*` on CkTimer are immediate mutators, not deferred requests.**
   VERIFIED: `Request_ChangeCountDirection` (`CkTimer_Utils.cpp:336-362`) and
   `Request_ReverseDirection` (`:364-378`) add/remove `ck::FTag_Timer_Countdown` inline and
   enqueue nothing — there is no request struct, no request entity, and no handler to complete.
   G0-D3 says ALL requests including trivial setters get the delegate, but the only consistent
   completion for these is a synchronous `Succeeded` fired at the Utils boundary, which is a
   shape the gate's branch tables do not enumerate.
   **Executor action:** left UNCHANGED (no delegate). Orchestrator ruling needed — this precedent
   governs every trivial setter framework-wide.

**Step 4's load-bearing unknown is RESOLVED — no STOP.** CkTimer had no EndPlay/Destructor
processor (`rg 'EndPlay|Destructor|Teardown' Source/CkTimer` → no processor hits), so a new one
was added. The destruction-group wiring is NOT ambiguous: the precedent is exact and complete in
CkInventory — `FGroup_EndPlay` + a `CK_IF_END_PLAY` view
(`CkInventory_Processor.h:218-252` `TProcessor_Inventory_CancelOnEndPlay_Base`,
`:313-329` `FProcessor_Inventory_MassTransfer_CancelOnEndPlay`), and
`CkProcessorGroups.h:169-173` documents `FGroup_EndPlay` as exactly the window in which
`CK_IF_END_PLAY` (EndPlay && !Teardown) matches. `FProcessor_Timer_CancelPendingRequests` copies
that shape verbatim. Correctness under a real teardown is pending the test run below.

**BLOCKER — nothing was built or tested this session. Zero compile/test evidence exists.**

Once the sibling session released the engine build lock (00:29:47Z), the baseline run was attempted
with the executor's changes stashed out of both submodules (true clean-HEAD tree, verified
`git status --porcelain` empty in both). Command:

```powershell
Set-Location "D:\Repos\CkPlugins"; ./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --test --no-nullrhi --discover-fresh --output=Saved/Logs/Gate00-Baseline.log --project="D:\Repos\CkPlugins"
```

It failed in seconds, before compiling anything, with the entire output being:

```
[utb --build] Previously selected engine '{21E60FAC-48AD-69BF-42B6-E98C333A2E90}' is no longer available. Run UnrealToolbox interactively to re-select.
```

Diagnosis (read-only, VERIFIED):
- `CkPlugins.uproject` `EngineAssociation` = `{22D2B5AE-4AE5-C485-F291-F79F407369F4}`
- Registered engines (`HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds`):
  `{E4464C1C-43B7-7194-E787-E7B5711509D4}` → `D:/Repos/UnrealEngineAngelscript`,
  `{F5534390-E8D1-4B55-83EF-4DA731819D09}` → `D:/Repos/UnrealEngineAngelscript_Other`
- Neither the uproject's GUID nor the toolbox's persisted GUID is registered. `BusterBlock.uproject`
  uses `{E4464C1C-…}` — which is why the sibling session builds fine and this project cannot.

Resolving this means either running UnrealToolbox interactively to re-select the engine, or
repointing `CkPlugins.uproject`'s `EngineAssociation`. Both are writes to shared/machine-global or
out-of-fence state and were NOT performed — escalated to the orchestrator instead. The executor's
changes were restored from the stashes immediately (both stashes popped clean, zero conflicts, no
foreign stash touched).

**Consequence:** baseline NOT captured; nothing compiled; the 3 new AutoTests were never
discovered or run; the AS default-argument question in STOP #1 remains EMPIRICALLY UNVERIFIED
(static evidence only). Every claim below about the code is source-level reasoning, NOT a
compile or test result.

**Doc-set note:** `Plugins/CkFoundation/.gitignore:49` (`*.md`) IGNORES this campaign's entire
`docs/campaigns/` tree — PROGRESS.md and its siblings are untracked working-tree files, not part
of any commit.

**Work completed (NOT compiled, NOT tested — see BLOCKER above)**
- Step 1+2 — NEW `Source/CkEcs/Public/CkEcs/Request/CkRequest_Completion.h`:
  `ECk_Request_OperationResult` (+ formatter), `FCk_Delegate_Request_OnCompleted`,
  `ck::UUtils_Signal_RequestCompleted` (signal declared inside `namespace ck`, matching
  `CkEntityLifetime_Fragment.h:104` and `CkTimer_Fragment.h:88-95`), and
  `ck::request::FireCancelledForPending`.
- Step 3 — 7 of 9 `Request_*` on `UCk_Utils_Timer_UE` took the trailing delegate
  (Reset, Complete, Stop, Pause, Resume, Jump, Consume). The 2 immediate mutators are STOP #3.
- Step 4 — result guard in `FProcessor_Timer_HandleRequests`'s drain (replacing the manual
  `GetAndDestroyRequestHandle()` epilogue) + new `FProcessor_Timer_CancelPendingRequests`
  (`FGroup_EndPlay`, `CK_IF_END_PLAY`), registered via `CK_REGISTER_PROCESSOR`.
  All three `DoHandleRequest` overloads are `void` with no rejection path, so `Succeeded` is set
  unconditionally after the call — noted as the gate doc instructed.
- Step 5 — 3 of 4 AutoTests authored in `Plugins/CkTests/Script/CkTimer/`
  (`..._RequestCompletion_SucceedsOnDrain`, `..._CancelledOnTeardown`, `..._NoDelegateNoOp`).
  `FailedNotEnqueued` is STOP #2. `NoDelegateNoOp` doubles as the AS omitted-argument proof.
- Step 7 — docs updated: root `CLAUDE.md` §Requests (new "Request completion" block),
  `Source/CkEcs/CLAUDE.md` §Signals (new "Request completion" subsection),
  `Source/CkTimer/CLAUDE.md` (new "Request completion — the reference implementation" section).

### 2026-07-25 — Campaign authored (orchestrator: Fable 5)
- Recon: Opus survey of request/signal/completion landscape (71 `_Requests` fragments, ~250
  request structs, 5 completion shapes). Orchestrator spot-VERIFIED anchors:
  `CkRequest_Data.h:19-168`, `CkSignal_Macros.h:44-49`, `CkInventory_Utils.cpp:192-216`,
  `CkInventory_Utils.h:243-252`.
- Maintainer ruled forks D2-D5 (interactive session); Fable ruled D1, D6-D8.
- Authored: PROMPT.md, FEATURE_CENSUS.md, GATE_00_Infrastructure.md, PROGRESS.md, VALIDATION.md.
- Inferred (unconfirmed): census row-level detail; AS optional-delegate behavior (Gate 00 step 6
  resolves); CkTimer teardown-processor existence (Gate 00 step 4 resolves).
- No source changes this session so far.

## Open items

| Item | Status | Next step |
|---|---|---|
| AS optional-delegate verification | Open | Gate 00 step 6 — STOP condition if AS forces the arg |
| Cancel-path processor-group wiring | Open | Gate 00 step 4 — the gate's load-bearing unknown |
| Gate 01+ membership finalization | Open | Re-grep census rows at each gate entry |

**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**

## Session log

| Date | Orchestrator | What moved | Sub-agent routing |
|---|---|---|---|
| 2026-07-26 | Fable 5 | Tree verified post-limit; INV-A3 experiment run + resolved (supported); overlay reverted; G0-D13 contract authored (Addendum 2, G0-D13a ruled); G0-D13 implementation dispatched | INV-A3 run → inline (toolbox); G0-D13 impl+gate → opus |
| 2026-07-25 | Fable 5 | Campaign chartered: recon, 8 decisions ruled, doc set authored | recon → opus (Explore) |
