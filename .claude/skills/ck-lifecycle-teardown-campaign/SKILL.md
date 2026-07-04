---
name: ck-lifecycle-teardown-campaign
description: Use when running the entity-teardown / signal-unbind lifecycle campaign — the CkInteraction "This processor doesn't get called, can cause issues if teardown is mid interaction!!!" TODOs, OnInteractionFinished never firing after Request_DestroyEntity mid-interaction, leaked interaction entities, stale FTag_TeamListener after UnbindFrom_OnTeamAssigned*, bind-vs-unbind coverage audits, *_EndPlay processor gaps. Not for general bug hunts (ck-debugging-playbook) or test-harness mechanics (ck-tests-authoring-and-running).
---

# ck-lifecycle-teardown-campaign

Single-purpose, decision-gated campaign to retire the entity-teardown / signal-unbind lifecycle
debt cluster in CkFoundation, anchored on a live code-confessed defect in CkInteraction. Selected
from Phase-0 evidence (DECISIONS.md item 19); the maintainer named no "hardest problem", and this
cluster won on robustness-first grounds. Run the phases in order; each has a falsifiable exit gate.

Repo layout assumed: superproject `d:\Repos\BusterBlock`, this plugin at `Plugins/CkFoundation`.
All shell commands are **Git Bash**, cwd `d:\Repos\BusterBlock` unless stated. Style for any code
you write: root `Plugins/CkFoundation/CLAUDE.md` (do not restate it — read it).

## When NOT to use this skill

| Situation | Use instead |
|---|---|
| A random teardown/lifecycle bug outside this campaign | `ck-debugging-playbook` |
| How to author/run AS autotests, suite baselines, harness mechanics | `ck-tests-authoring-and-running` (CkTests) |
| ECS pipeline concepts beyond the primer below | `ckecs-architecture-contract` |
| Landing any code change (review class, gates, commit ritual) | `ck-change-control` |
| Global fragment-storage pointer-stability (`in_place_delete`) questions | SETTLED design — DECISIONS.md §45; still fenced context here, see fences below |

## Success criteria — falsifiable, never judged by eye

1. **(a) Inventory committed:** a complete enumerated inventory of bind-sites vs unbind-sites and
   teardown-processor coverage per feature module (Phase 0), committed as a report file in the
   campaign branch.
2. **(b) Red repro:** a CkTests AS autotest that REPRODUCES the mid-interaction teardown miss —
   red at HEAD (Phase 1).
3. **(c) Green fix, zero new ensures:** a fix promoted through `ck-change-control` that turns (b)
   green with zero new ensure-fires across the full CkTests suite, diffed against a baseline you
   recorded BEFORE touching anything (Phase 4).

## Evidence table — re-verified at HEAD 2026-07-02

Every row below was read from source on 2026-07-02. Re-verify before acting (commands in
Provenance). **Two of the mission-brief's grep patterns were wrong; corrected patterns are used
here** — the comment says "doesn't", not "don't", and the Team TODOs say "unbound", not "unbind".

| # | Anchor (file:line) | Quoted confession | What it implies |
|---|---|---|---|
| 1 | `Plugins/CkFoundation/Source/CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp:222` | `// TODO: This processor doesn't get called, can cause issues if teardown is mid interaction!!!` (in `FProcessor_InteractTarget_EndPlay::ForEachEntity`) | The author believed the EndPlay cleanup (release signal connections + fail in-flight interactions) never runs. Dated 2024-10-24 (commit `9320ccbe7`) — **predates** the processor scheduler's EndPlay window (2026-04-14, `6b57b2b9b`), so the literal claim may be stale while the downstream effect is not. |
| 2 | `.../InteractSource/CkInteractSource_Processor.cpp:177` | identical TODO, in `FProcessor_InteractSource_EndPlay::ForEachEntity` | Same confession, source side. Two sites, not one. |
| 3 | `Plugins/CkTests/Script/CkInteraction/CkAutoTest_Interaction_TimedInterruptedByCancel.as:13-16` | header comment: test was "simplified to use the CkInteraction-native Cancel path rather than destroying the target entity (**target destruction would leak the interaction**; Cancel is the supported interrupt verb)" | A prior test-authoring session independently identified the destroy-mid-interaction path as broken and deliberately routed around it. The suite has NO test covering the broken path — that is the Phase-1 gap to fill. |
| 4 | `Plugins/CkFoundation/Source/CkRelationship/Public/CkRelationship/Team/CkTeam_Utils.cpp:376, 402, 428` | `// TODO: figure out a bullet-proof way to remove the FTag_TeamListener if ALL the delegates have been unbound` (×3) | `FTag_TeamListener` is added on every `BindTo_OnTeamAssignedToAnyEntity*` (`DoAddTeamListener`, same file :432-441) and never removed. Two views iterate it forever: `CkTeam_Processor.cpp:64` and `:78`. Unbound listeners keep paying fan-out cost permanently. |
| 5 | `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h:44, 99` | `static constexpr auto in_place_delete = true;` — from commit `2c8319c1c` (2023-11): "we turn on the pointer stability guarantee for the fragments... A proper fix is upcoming" | **FENCED context, not a target.** Now settled design — fragment-storage pointer stability is global and deliberate (DECISIONS.md §45). Load-bearing for ~3 years against random signal disconnects. Do not touch. |
| 6 | `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Handle/CkHandle.cpp:790` | `// TODO: This is a temporary fix. We need to find a better way to handle the fact that sometimes the Handle does NOT have a replicated object` | Adjacent lifecycle-debt evidence (replication-owned). NOT in scope — do not "fix" en route. |
| 7 | `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.cpp:314` | `// TODO: This is a temporary fix. We need to find a better way to handle this` | Same — adjacent evidence only. |

If any anchor is GONE when you re-verify: check `git log -S "<quoted text>" -- <file>` in
`Plugins/CkFoundation` to find the removing commit, update DECISIONS.md item 19 accordingly, and
re-scope the campaign to the anchors that remain (if #1/#2 are fixed, the campaign narrows to
anchor #4, the TeamListener debt — Phase 3 option B still applies).

## Mechanics primer — just enough to reason about the phases

Read once; everything in Phases 1-3 depends on it. (Deeper treatment: `ckecs-architecture-contract`.)

**Destruction pipeline** (documented in
`Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h:32-41`, verified against the processors):
`UCk_Utils_EntityLifetime_UE::Request_DestroyEntity` (`CkEntityLifetime_Utils.cpp:28`) adds
`FTag_DestroyEntity_Initiate` **synchronously at call time and recursively to all lifetime
dependents** (child entities — :65-66). Then, per tick-group pipeline order:

| Tick | Group | What happens |
|---|---|---|
| N | `FGroup_EntityLifecycle` | `DestructionPhase_Endplay` adds `FTag_DestroyEntity_EndPlay` to every Initiate-tagged entity |
| N | `FGroup_EndPlay` | every feature `FProcessor_*_EndPlay` whose view includes `CK_IF_END_PLAY` (= EndPlay tag + NOT Teardown/Await/Finalize, `CkEntityLifetime_Fragment.h:43-47`) fires — **this is the only window teardown processors get** |
| N | `FGroup_Teardown` | `FTag_DestroyEntity_Teardown` added — window closes |
| N+1 | `FGroup_DestructionPipeline` (start of tick) | `FTag_DestroyEntity_Await` |
| N+2 | same | `FTag_DestroyEntity_Finalize`; entity actually destroyed |

**`CK_IGNORE_PENDING_KILL`** (`CkEntityLifetime_Fragment.h:37-41`) is the opposite filter: it
EXCLUDES entities in any phase EndPlay-or-later. Every ordinary `HandleRequests` processor carries
it — so **a request queued on an entity that is already in its EndPlay window is never consumed**.

**Signals**: a Ck signal is an ECS fragment (`TFragment_Signal`) living on the broadcasting entity;
binding returns a `ConnectionType` the listener may store. `ECk_Signal_PostFireBehavior::Unbind`
means one-shot — the binding auto-disconnects after the first fire
(`CkSignal_Utils.inl.h:105-113`); `DoNothing` means the connection persists until released manually.

**World teardown is NOT the pipeline**: `UCk_EcsWorld_Subsystem_UE::Deinitialize`
(`Source/CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.cpp:156-182`) resets the schedulers and
frees the registry with **no final destruction-pipeline pump** — entities alive at PIE-end/world
death never see their EndPlay processors. Keep the two teardown meanings separate in every claim
you write: *entity* teardown (pipeline, mid-game) vs *world* teardown (Deinitialize, end of play).

**The anchored defect, as read from HEAD** (this is the causal chain Phase 1 must test):

1. An interaction entity is spawned as a **lifetime child of the InteractTarget entity**
   (`UCk_Utils_Interaction_UE::Add` → `Request_SpawnEntity(InHandle=target, ...)`,
   `CkInteraction_Utils.cpp:22-38`).
2. Target and Source each raw-bind a processor member to the interaction's
   `OnInteractionFinished` signal with `PostFireBehavior::DoNothing` and stash the connection in
   `_InteractionFinishedSignals` (`CkInteractTarget_Processor.cpp:122-129`,
   `CkInteractSource_Processor.cpp:104-111`). These are the **only two** raw
   `::Bind<&FProcessor...>` sites in the plugin (count below).
3. Destroy the target mid-interaction → cascade marks the interaction entity too → both enter the
   same EndPlay window.
4. `FProcessor_InteractTarget_EndPlay` (if it runs — Phase 1 decides) queues
   `Request_EndInteraction(Failed)` on the interaction entity
   (`CkInteractTarget_Processor.cpp:224-228`).
5. That request is handled by `FProcessor_Interaction_HandleRequests` — whose view carries
   `CK_IGNORE_PENDING_KILL` (`CkInteraction_Processor.h:18`) — and the interaction entity is
   pending-kill. **The request is never consumed; `OnInteractionFinished` is never broadcast**
   (the broadcast lives only in the request handler, `CkInteraction_Processor.cpp:67`).
6. `FProcessor_Interaction_EndPlay` — the one processor guaranteed to see the dying interaction —
   has an **empty body** (`CkInteraction_Processor.cpp:72-80`).
7. Net effect: the source (which is NOT dying) never hears the interaction ended — gameplay
   waiting on completion hangs, and the source's `_InteractionFinishedSignals` keeps a stale entry
   until the source itself dies.

Steps 1-6 are confirmed by reading; step 7's observable behavior is inferred until the Phase-1
test runs. That test is the point of Phase 1.

## Phase 0 — baseline inventory

Deliverable: a committed report (suggested:
`Plugins/CkFoundation/.claude/reports/lifecycle-campaign-phase0.md`) containing the outputs below,
re-derived on YOUR date. My numbers, measured 2026-07-02, are the reference shape — if yours
differ wildly, investigate before proceeding.

Run from `d:\Repos\BusterBlock/Plugins/CkFoundation/Source` (Git Bash). `--no-ignore` is
load-bearing: the superproject `.ignore` blinds ripgrep-based tooling under several plugin dirs.

```bash
# 1. Macro-level bind vs unbind call sites (excludes the #define lines themselves)
rg --no-ignore -n "CK_SIGNAL_BIND\(" --glob "*.cpp" --glob "*.h" . | grep -v define | wc -l    # → 130
rg --no-ignore -n "CK_SIGNAL_UNBIND\(" --glob "*.cpp" --glob "*.h" . | grep -v define | wc -l  # → 126

# 2. BP/AS-facing surface: BindTo_* vs UnbindFrom_* definition sites (.cpp)
rg --no-ignore -n "^\s*(UCk_Utils_\w+::)?\s*BindTo_On\w+\(" --glob "*.cpp" . | wc -l       # → 169
rg --no-ignore -n "^\s*(UCk_Utils_\w+::)?\s*UnbindFrom_On\w+\(" --glob "*.cpp" . | wc -l   # → 149

# 3. Name-set diff: BindTo names with no same-named UnbindFrom (header surface)
rg --no-ignore -o "BindTo_(On\w+)" --glob "*.h" -N . | sed 's/.*BindTo_//' | sort -u > /tmp/b.txt
rg --no-ignore -o "UnbindFrom_(On\w+)" --glob "*.h" -N . | sed 's/.*UnbindFrom_//' | sort -u > /tmp/u.txt
comm -23 /tmp/b.txt /tmp/u.txt
# → 4 names (2026-07-02), classified:
#   OnGoal                      false positive — a comment wildcard "BindTo_OnGoal*" (CkCrowdAgent_Fragment.h:71)
#   OnTeamAssignedToAnyEntity   naming asymmetry — unbind EXISTS as UnbindFrom_OnTeamAssigned (CkTeam_Utils.h:388)
#   OnLoginEvent, OnLogoutEvent genuinely unpaired — CkGameSession_Subsystem (subsystem-lifetime binds)

# 4. Teardown-processor coverage: EndPlay registrations, and modules that bind but register none
rg --no-ignore -n "CK_REGISTER_PROCESSOR\(.*_EndPlay\)" . | wc -l   # → 23
comm -23 <(rg --no-ignore -l "CK_SIGNAL_BIND\(" --glob "*.cpp" --glob "*.h" . | grep -v Macros | tr '\\\\' '/' | cut -d/ -f2 | sort -u) \
         <(rg --no-ignore -l "CK_REGISTER_PROCESSOR\(.*_EndPlay\)" . | tr '\\\\' '/' | cut -d/ -f2 | sort -u)
# → 21 modules bind signals but register no *_EndPlay processor (2026-07-02):
#   CkAggro CkAnimation CkAttribute CkCrowd CkEcsExt CkEntityCollection CkEntityExtension
#   CkEntityTag CkEqs CkGrid CkMessaging CkNavigation CkRaySense CkRelationship CkRenderTarget
#   CkResolver CkShapes CkSpatialQuery CkSubstep CkTagSet CkTween

# 5. PostFireBehavior adoption
rg --no-ignore -n "PostFireBehavior::Unbind" . | wc -l      # → 21
rg --no-ignore -n "PostFireBehavior::DoNothing" . | wc -l   # → 163

# 6. The load-bearing shortlist: cross-entity connection storage + raw processor binds
rg --no-ignore -n "::Bind<&FProcessor" .
# → exactly 2 sites: CkInteractTarget_Processor.cpp:122, CkInteractSource_Processor.cpp:104
rg --no-ignore -l "ConnectionType\b" --glob "*Fragment*.h" .
# → 3 files: the two CkInteraction fragments + CkSignal_Fragment.h itself
```

**How to read this honestly (write this framing into your report):** the module list in #4 is a
LEAD list, not a defect list. A signal fragment dies with its broadcasting entity, disconnecting
its listeners; the dangerous direction is a listener *storing a connection to another entity's
signal* (#6 — exactly the two interaction sites) or a bind that adds a tag/fragment nothing
removes (anchor #4, TeamListener). Do not open 21 module fix-tickets off #4 alone.

## Phase 1 — reproduce (red test first)

Author ONE new AS autotest in the existing CkInteraction family. File:
`Plugins/CkTests/Script/CkInteraction/CkAutoTest_Interaction_DestroyTargetMidInteraction_SourceHearsFailed.as`.
Harness mechanics (base class contract, running the suite, reading verdicts): load
`ck-tests-authoring-and-running`. Copy the shape of
`CkAutoTest_Interaction_TimedInterruptedByCancel.as` (same directory) — the sketch below deviates
from it only where the scenario requires (target on its own entity; destroy instead of cancel;
listen on the source).

```angelscript
// Language=angelscript
// Pins the destroy-mid-interaction contract: destroying the InteractTarget's entity
// while a Timed interaction is in flight must still notify the surviving
// InteractSource with Failed. Red at HEAD per CkInteractTarget_Processor.cpp:222 TODO.
class UCk_AutoTest_Interaction_DestroyTargetMidInteraction_SourceHearsFailed : UCk_AutoTest_Base
{
    default _TimeoutSeconds = 4.0f;

    private FCk_Handle _MyEntity;
    private FCk_Handle _TargetOwner;
    private FCk_Handle_InteractSource _Source;
    private FCk_Handle_InteractTarget _Target;
    private bool _DestroyRequested = false;

    UFUNCTION(BlueprintOverride)
    void DoBeginPlay(FCk_Handle InHandle)
    {
        auto LocalHandle = InHandle;
        _MyEntity = LocalHandle;
        auto Channel = interaction_gym_helpers::DefaultChannel();

        auto SourceParams = FCk_Fragment_InteractSource_ParamsData();
        // Direct _Member write is the corpus-standard AS form here: this ParamsData ships no
        // CK_DEFINE_CONSTRUCTORS, so the BlueprintReadWrite surface is the only population path
        // (every committed CkInteraction gym/autotest does exactly this).
        SourceParams._InteractionChannel = Channel;
        _Source = utils_interact_source::Add(LocalHandle, SourceParams);

        // Target on its OWN entity — destroying it must not touch the source or the test entity
        _TargetOwner = utils_entity_lifetime::Request_CreateEntity(LocalHandle);
        auto TargetParams = FCk_Fragment_InteractTarget_ParamsData(Channel);
        TargetParams.Set_CompletionPolicy(ECk_Interaction_CompletionPolicy::Timed);
        TargetParams.Set_InteractionDuration(FCk_Time(0.5f));
        _Target = utils_interact_target::Add(_TargetOwner, TargetParams);

        utils_interact_source::BindTo_OnInteractionFinished(
            _Source,
            FCk_Delegate_InteractSource_OnInteractionFinished(this, n"OnSourceHeardFinished"));
        utils_interact_target::BindTo_OnNewInteraction(
            _Target,
            FCk_Delegate_InteractTarget_OnNewInteraction(this, n"OnNewInteraction"));

        auto Request = FCk_Try_InteractTarget_StartInteraction();
        Request.Set_InteractSource(_MyEntity);
        Request.Set_InteractInstigator(_MyEntity);
        utils_interact_target::Request_StartInteraction(_Target, Request);
    }

    UFUNCTION()
    private void OnNewInteraction(
        FCk_Handle_InteractTarget InTarget,
        FCk_Handle_Interaction InInteraction)
    {
        if (_DestroyRequested) { return; }
        System::SetTimer(this, n"DoDestroyTarget", 0.1f, bLooping = false);
    }

    UFUNCTION()
    private void DoDestroyTarget()
    {
        if (IsFinished()) { return; }
        _DestroyRequested = true;
        utils_entity_lifetime::Request_DestroyEntity(_TargetOwner);
    }

    UFUNCTION()
    private void OnSourceHeardFinished(
        FCk_Handle_InteractSource InSource,
        FCk_Handle_Interaction InInteraction,
        ECk_SucceededFailed InResult)
    {
        if (IsFinished()) { return; }

        Assert_True(_DestroyRequested,
            "Source must not hear Finished before the destroy (Timed 0.5s cannot complete by 0.1s)");
        Assert_True(InResult == ECk_SucceededFailed::Failed,
            f"Destroy-mid-interaction must finish as Failed (got {InResult})");
        FinishSuccess();
    }
}
```

Expected at HEAD: **timeout-red** — `OnSourceHeardFinished` never fires (mechanics-primer step 5).
The harness timeout (4s) IS the red signal; that is why `_TimeoutSeconds` stays small.

The same API surface, other environments (for manual repro or a gym probe):

- **C++:** `UCk_Utils_InteractSource_UE::BindTo_OnInteractionFinished(InSource, InDelegate, InBindingPolicy, InPostFireBehavior)` (`CkInteractSource_Utils.h:161-165`).
- **BP:** node `[Ck][InteractSource] Bind To OnInteractionFinished`, category `Ck|Utils|InteractSource`.
- **AS:** `utils_interact_source::BindTo_OnInteractionFinished(...)` as in the sketch (mixin form also works: `_Source.BindTo_OnInteractionFinished(...)`).

### Phase-1 exit gate — branch on what the test actually does

| Observed | Verdict | Next action |
|---|---|---|
| Test times out red (source never notified) | Defect confirmed live | → Phase 2 with the test as your oracle |
| Test PASSES (source hears Failed) | The 2024 TODOs are stale — the 2026-04 scheduler EndPlay window closed the gap | Delete the two TODO comments **as part of the campaign commit**, append the finding to `Plugins/CkFoundation/.claude/reports/DECISIONS.md` (amend item 19), keep the now-green test as a pin, and **narrow the campaign to anchor #4** (TeamListener — Phase 3 option B only) |
| Test can't observe teardown directly (e.g. neither fires nor times out cleanly, or you cannot tell WHY it is red) | Instrument first | Enable the framework's own breadcrumbs — console `Log CkEcs VeryVerbose` and `Log CkInteraction VeryVerbose` — and read the run log: `[DESTRUCTION] Entity [N] set to 'End Play'` proves the pipeline ran; `Interaction [N] EndInteraction with [Failed]` (`CkInteraction_Processor.cpp:64`) proves the end-request was CONSUMED. Present-first + absent-second = the request-consumption gap (H1) |

## Phase 2 — diagnose: ranked hypotheses

Ranked by likelihood from the code read at HEAD (2026-07-02). Each has the experiment that
discriminates it. Stop when one survives; do not fix two hypotheses at once.

| Rank | Hypothesis | Basis | Discriminating experiment |
|---|---|---|---|
| **H1 — request-consumption gap** (most likely) | EndPlay processors DO fire (scheduler brackets them since `6b57b2b9b`), but the `Request_EndInteraction(Failed)` they queue lands on an interaction entity that is itself pending-kill; `FProcessor_Interaction_HandleRequests` excludes pending-kill (`CkInteraction_Processor.h:18`), and `FProcessor_Interaction_EndPlay` is empty (`CkInteraction_Processor.cpp:72-80`), so the Finished broadcast (only emitted at `CkInteraction_Processor.cpp:67`) never happens | Whole chain read from source; only the runtime observation is missing | VeryVerbose log during the red test: `set to 'End Play'` lines present for target+interaction, `EndInteraction with` line ABSENT |
| **H2 — world-teardown no-pump** (independent, also real) | At PIE-end, `Deinitialize` (`CkEcsWorld_Subsystem.cpp:156-182`) resets schedulers and frees the registry without pumping the destruction pipeline — under THIS reading the TODOs' literal text ("doesn't get called") is still true today | Read from source; consequences at world death are bounded (registry dies wholesale) but the TODO cleanup provably never runs | `[EDITOR-VERIFY]` — cannot be automated in the AS harness (the world dies with the test). Manual: open the editor, PIE into the CkInteraction gym (console `Ck_Gym_List` to find it), console `Log CkEcs VeryVerbose`, start a Timed interaction, hit Stop mid-flight, then read `Saved/Logs/` for the absence of `set to 'End Play'` lines for the interaction entities after the stop |
| **H3 — TODOs fully stale** | The Phase-1 test passes; scheduler window + request path both work | Contradicted by the code read (H1 chain), so lowest rank — but the test outranks my reading | The Phase-1 test itself; if green, take the stale branch of the Phase-1 gate |

Rejected while reading (log these in your report so they aren't re-litigated):
- ~~"the EndPlay processor's view excludes mid-destroy entities"~~ — backwards; `CK_IF_END_PLAY`
  *requires* the EndPlay tag (`CkEntityLifetime_Fragment.h:43-47`).
- ~~"signals randomly disconnect during teardown"~~ — that was the 2023 storage bug, fixed by the
  fenced `in_place_delete` workaround (`2c8319c1c`); do not reopen it.

**Fences for this phase:** do NOT touch global fragment-storage pointer-stability /
`in_place_delete` (settled, deliberate design — DECISIONS.md §45; a load-bearing invariant). Do
NOT resurrect any stalled branch (DECISIONS.md item 25 lists them as do-not-resurrect). Do not "fix" anchors #6/#7
(replication-owned, separate campaign).

## Phase 3 — solution menu, ranked

Fix ONE ranked option, prove it, then stop. Ranked for the H1 outcome (re-rank per your Phase-2
survivor).

### A (recommended) — make the dying interaction's end observable inside the EndPlay window

Two variants; pick **A1** unless review prefers the request idiom:

- **A1 — fill the empty `FProcessor_Interaction_EndPlay`:** broadcast
  `UUtils_Signal_Interaction_OnInteractionFinished(..., Failed)` from the interaction entity's own
  EndPlay processor (it is guaranteed to run inside the window for pipeline-destroyed
  interactions). Add a finished-guard (e.g. flag on `FFragment_Interaction_Current`) so an
  interaction that finished normally in the same frame doesn't double-broadcast.
- **A2 — EndPlay-window request drain:** a new processor with the HandleRequests view but
  `CK_IF_END_PLAY` instead of `CK_IGNORE_PENDING_KILL`, `Group = FGroup_EndPlay`,
  `RunAfter = TDepList<FProcessor_InteractTarget_EndPlay, FProcessor_InteractSource_EndPlay>` —
  consumes the exact `Request_EndInteraction(Failed)` the target's EndPlay already queues
  (mission's "ordering fix / deferred-destroy pass placement"). More moving parts than A1, but
  keeps "requests are the only mutation path".

| | |
|---|---|
| Files touched | `CkInteraction/Interaction/CkInteraction_Processor.h/.cpp` (+ `CkInteraction_Fragment.h` for the A1 guard flag; + registration line for A2) |
| Proof obligations | Phase-1 test green; no double-`OnInteractionFinished` when an interaction completes normally in the destroy frame (extend the test or add a sibling); the listener callbacks (`CkInteractTarget_Processor.cpp:174-209`, `CkInteractSource_Processor.cpp:134-164`) tolerate being invoked while the target is pending-kill — their `Cast`/`CK_ENSURE_IF_NOT` guards must not start firing: **zero new ensures across the suite** |
| Risk | Medium-low; scoped to CkInteraction; changes when (not whether) an existing signal fires |
| Rollback | Revert the single commit; the red test then documents the regression |

### B — explicit unbind-on-destroy sweep for the affected features

Two independent halves — **separate commits**:

- **B-interaction:** purge stale `_InteractionFinishedSignals` entries (dead interaction keys) on
  the SURVIVING party. Without A this only stops the bookkeeping leak — the source still never
  hears Failed, so B-interaction alone does not close anchor #1/#2. Pair with A or don't bother.
- **B-team (this is the whole campaign if Phase 1 took the stale branch):** retire the three
  `FTag_TeamListener` TODOs (`CkTeam_Utils.cpp:376/402/428`). Proof obligation first: find or add
  signal introspection ("does this entity have zero connections across the three team signals?")
  — enumerate what `TUtils_Signal` exposes before designing; if nothing, a per-entity listener
  refcount alongside `DoAddTeamListener` is the fallback. Success: a CkTests AS autotest asserting
  the tag is gone after last unbind, and that `CkTeam_Processor.cpp:64/:78` views no longer visit
  the entity. |

Risk: low (Team) — additive bookkeeping; rollback per-commit revert.

### C — `PostFireBehavior::Unbind` adoption at the two raw bind sites

`Unbind` = one-shot auto-disconnect after first fire (`CkSignal_Utils.inl.h:105-113`).
`OnInteractionFinished` fires at most once per interaction, so semantics fit and the manual
`release()` bookkeeping in both `OnInteractionFinished` callbacks shrinks. **But it does NOT fix
the defect** — a destroyed interaction never fires, so auto-unbind never triggers either. Adopt
only as hygiene riding an A commit, never as the fix. Enumerate affected sites first (exactly 2;
suite-wide adoption today is 21 `Unbind` vs 163 `DoNothing` — this is not a mass migration).

### Fenced wrong paths — do not walk these

| Path | Why fenced |
|---|---|
| Change fragment-storage pointer stability / remove `in_place_delete` | Settled, deliberate design — global, not signal-only (DECISIONS.md §45); the per-signal opt-ins are 3-years load-bearing against random disconnects (`2c8319c1c`). Touching it silently re-opens that bug class. |
| Blanket "auto-unbind everything on destroy" signal refactor | 150 signal definition sites (146 feature-level `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` invocations + the 4 chained expansions inside `CkSignal_Macros.h`), 130 bind sites — blast radius is every feature for a defect proven in one module. The campaign is anchored, keep it anchored. |
| Suppress/downgrade any ensure that fires during the fix | Violates the silent-error mandate (DECISIONS.md item 22: ensures over logs). An ensure firing in your new EndPlay-window code is your bug, not noise. |
| Resurrect a stalled branch that "already fixed this" | DECISIONS.md item 25: stalled branches are do-not-resurrect. Re-derive any idea you find there as fresh, reviewed work. |
| Pump the destruction pipeline inside `Deinitialize` to "fix" H2 without a maintainer call | Ordering hazard: schedulers/processors are mid-death there; the comment at `CkEcsWorld_Subsystem.cpp:171-174` shows the free-order is deliberate. If H2 matters for a real symptom, write it up in ADJUDICATIONS.md instead. |

## Phase 4 — promotion

1. **Baseline BEFORE any edit:** run the full CkTests suite and record pass/fail counts and the
   NAMES of failing tests, plus ensure-fire count from the run log (mechanics: load
   `ck-tests-authoring-and-running`). No baseline = you cannot claim "zero new ensures" later.
2. Land the red test (Phase 1) as its own commit — it documents the defect independent of the fix.
3. Land the fix (Phase 3) through `ck-change-control` as a **framework-invariant-class** change
   (it alters when a framework signal fires): review gates, commit shape, and messaging live
   there.
4. **Re-run the whole suite; report the delta** in the exact form "baseline N failing {names} →
   now M {names}; ensures baseline E → E". Any new red or new ensure: revert to known-good first,
   re-diagnose, re-sequence — do not stack a fix on a broken base.
5. Append the outcome to `Plugins/CkFoundation/.claude/reports/DECISIONS.md` (new numbered item:
   what was proven, which anchors closed, which — like H2 or B-team — were left open and where
   they are recorded). Delete the two TODO comments only in the commit that makes them false.

## Common mistakes

- **Grepping with the mission-brief patterns and concluding the anchors are gone.** The comment
  says `doesn't get called` (not "don't"); the Team TODOs say `unbound` (not "unbind"). Use the
  Provenance commands.
- **Using the Grep tool on `Plugins/CkTests/Script` or `Plugins/CkFoundation/Script`** — the
  superproject `.ignore` makes it silently return zero matches there. Use Bash `rg --no-ignore`.
- **Conflating entity teardown with world teardown.** The EndPlay window exists only while the
  scheduler ticks; PIE-end (`Deinitialize`) never runs it. A claim proven for one says nothing
  about the other.
- **Spawning the target on the autotest entity.** Destroying it then cascades into your own test
  entity (`Request_DestroyEntity` recursively marks lifetime dependents,
  `CkEntityLifetime_Utils.cpp:65-66`) and the test kills itself. Own entity, always.
- **Reading a green Phase-1 run from a stale binary.** AS recompiles on editor boot, but C++ fix
  + old build = the test exercised nothing. Re-run the gate on the final binary (build/run
  mechanics: `ck-build-and-env`).
- **Treating the 21-module bind-without-EndPlay list as 21 defects.** It is a lead list; the
  dangerous shapes are cross-entity connection storage (2 sites) and bind-adds-tag-nothing-removes
  (TeamListener). Report it as such.
- **Fixing H1 and H2 in one commit.** They are independent mechanisms with independent proof
  obligations; H2 additionally needs a maintainer call (see fences).

## Provenance and maintenance

Campaign facts measured/verified 2026-07-02 against CkFoundation HEAD `7330c1bab` (superproject
`d:\Repos\BusterBlock`). Re-verify each volatile fact before trusting it
(Git Bash, cwd `d:\Repos\BusterBlock`):

```bash
# Anchors #1/#2 (corrected pattern — "doesn't", not "don't")
rg --no-ignore -n "doesn't get called" Plugins/CkFoundation/Source/CkInteraction
# Anchor #3 (test-suite confession)
rg --no-ignore -n "target destruction would leak" Plugins/CkTests/Script/CkInteraction
# Anchor #4 (corrected pattern — "unbound", not "unbind")
rg --no-ignore -in "TODO.*unbound" Plugins/CkFoundation/Source/CkRelationship
# Anchor #5 (fenced)
rg --no-ignore -n "in_place_delete" Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h
# Anchors #6/#7 (adjacent, out of scope)
rg --no-ignore -n "temporary fix" Plugins/CkFoundation/Source
# TODO-vs-scheduler dating
cd Plugins/CkFoundation && git log -1 --format="%h %ad %s" --date=short -S "This processor doesn't get called" -- Source/CkInteraction && git log --follow --format="%h %ad %s" --date=short -- Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h | tail -2
# All Phase-0 counts: re-run the Phase-0 block verbatim (cwd Plugins/CkFoundation/Source)
```

Volatile facts to re-check on any drift: the 2026-07-02 counts (130/126, 169/149, 23 EndPlay
registrations, 21/163 PostFireBehavior split, 2 raw processor binds, 21-module lead list, 150
`CK_DEFINE_SIGNAL*` definition sites — 146 feature-level `_AND_UTILS_WITH_DELEGATE` invocations +
4 chained expansions inside `CkSignal_Macros.h`); the empty `FProcessor_Interaction_EndPlay` body;
the `CK_IGNORE_PENDING_KILL` filter on `CkInteraction_Processor.h:18`; the no-pump `Deinitialize`
at `CkEcsWorld_Subsystem.cpp:156-182`; fragment-storage pointer stability stays settled design
(DECISIONS.md §45 — A3 was resolved 2026-07-02, the fence is now "settled invariant", not
"pending ruling"); DECISIONS.md items 19/22/25 wording. If the Phase-1 test already exists in
`Plugins/CkTests/Script/CkInteraction/`, a prior session ran this campaign — read its DECISIONS.md
entry before redoing anything.
