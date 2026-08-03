---
name: ck-lifecycle-teardown-campaign
description: "Use when executing Ck entity-teardown and signal-unbind work, especially mid-interaction destruction, leaked bindings, TeamListener cleanup, or EndPlay gaps; not for triage."
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


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Phases 0-4 — baseline, reproduce, diagnose, solution menu, promotion | `references/phases.md` |

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
