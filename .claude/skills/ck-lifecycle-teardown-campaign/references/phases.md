# Phases 0-4 — baseline, reproduce, diagnose, solution menu, promotion

Reference for `ck-lifecycle-teardown-campaign`: the executable phase sequence. Load the phase you are on.

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

        auto SourceParams = FCk_InteractSource_Spec();
        // Direct _Member write is the corpus-standard AS form here: this Spec struct ships no
        // CK_DEFINE_CONSTRUCTORS, so the BlueprintReadWrite surface is the only population path
        // (every committed CkInteraction gym/autotest does exactly this).
        SourceParams._InteractionChannel = Channel;
        _Source = utils_interact_source::Add(LocalHandle, SourceParams);

        // Target on its OWN entity — destroying it must not touch the source or the test entity
        _TargetOwner = utils_entity_lifetime::Request_CreateEntity(LocalHandle);
        auto TargetParams = FCk_InteractTarget_Spec(Channel);
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

