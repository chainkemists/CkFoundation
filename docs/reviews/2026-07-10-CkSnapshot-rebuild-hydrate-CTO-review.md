# CkSnapshot v2 (Rebuild + Hydrate) — CTO Design Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the design author's assistant will pick up your notes from there.

> **Pre-implementation review.** No implementation plan has been authored yet. The artifact under review is the **design spec** — the architectural commitments before plan-writing burns effort. If you flag a blocker now, we revise the spec; if you green-light, we proceed to author the phased implementation plan (Phase 0 first), then come back to you for plan review. This mirrors the workflow of the original CkSnapshot design review ([2026-05-20-CkSnapshot-design-CTO-review.md](2026-05-20-CkSnapshot-design-CTO-review.md)) — which this design, if approved, **supersedes**.

---

## Reviewer brief

### Your role

Senior reviewer / architect. You are reviewing a proposed **replacement architecture for CkSnapshot save/load**: retiring the registry-image + repair model (GREEN-LIT by you 2026-05-21, built out through the M1/M2 campaign) in favor of **rebuild + hydrate** — save/load converged onto the replication pipeline. Specifically:

1. Catch architectural issues that would be expensive to discover mid-migration — this touches CkEcs core (scheduler, handler registry, EntityScript spawn path), not just CkSnapshot.
2. Judge whether retiring an architecture you green-lit 14 months of Ck-time ago is warranted by the evidence, or whether the current model should be hardened instead (the spec presents both as Model A vs Model B).
3. Scrutinize the verified-corrections sections — six pillar claims were adversarially verified against code and came back PARTIAL; the design folds in their corrections. Check the corrections are sufficient, not just acknowledged.
4. Rule on the five open forks in spec §7 — they are deliberately NOT settled and your input is wanted.
5. Either green-light for plan authorship, or list specific blocking concerns.

You are expected to **read code in the repo** — don't review the spec in isolation. The spec cites file:line for every load-bearing claim; spot-check the ones that carry the most weight (list in section D below).

### What's being built

The maintainer's (Adam's) framing, verbatim: *"improving the save load logic of this framework … it is crucial that it is not only robust and bug-free but also has great code ergonomics so that existing and future features can work with the save-load easily … avoid footguns or tripwires that creep up in every feature of the framework (see CameraLayer_Utils.cpp local change for an example where the original implementer should NOT have to know about save load when designing the feature)."*

The tech director's target, verbatim: *"ideally, we would like the save/load to work similar to a replicated feature — build the Entity as normal, all processors are gated to not run while we are loading (including Setup), load the minimum required data for a fragment, similar to how we replicate data — this means that for any feature, replication and save/load should be the same 'code' … only have one Setup that happily runs regardless of whether it's loaded save or new Entity."*

The spec's answer: **save = per-entity recipe (EntityScript class + spawn params + topology — the same recipe `_ReplicationData_EntityScript` already ships to clients) + hydration payload (the existing `FCk_RepData_*` structs)**; **load = teardown → travel → rebuild through the normal spawn path with simulation processors gated (new `ECk_ProcessorLoadPolicy` trait, default gated) → hydrate through the same Apply-handler registry replication uses (dispatcher moved to a late group) → settle → open gate.** Five staged phases; the current model stays green until Phase 5 deletes it; its capture code survives as a test-only registry-diff "fidelity oracle."

### Design spec location

[2026-07-10-CkSnapshot-rebuild-hydrate-design.md](../specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md)

### Critical context — read before reviewing

- **Your own prior review:** [2026-05-20-CkSnapshot-design-CTO-review.md](2026-05-20-CkSnapshot-design-CTO-review.md) — the v1 blocker-3 resolution ("spawn-params explicitly dropped … making Construct universally idempotent would be a far bigger contract change") is the exact decision this design reverses. The spec argues the M2 reconstitution campaign is the compounding cost of that choice.
- **The uncommitted working tree** (`git diff` in the CkFoundation submodule, 16 files as of 2026-07-10) — the live M2 state: CkCamera adopt-or-add, CkEcsExt Transform `_Previous` re-seed, CkPhysics `NeedsSetup` rep-defer + restore-seed retry, `ECk_ReconstitutionPhase` EarlyWindow machinery. These are the spec's Exhibit A for "footguns are structural."
- `Plugins/CkFoundation/CLAUDE.md` + `Source/CLAUDE.md` — doctrine of record (non-negotiables, replicated-fragment `RegisterLazy` contract, trait conventions).
- Replication pipeline the design converges onto: `Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/` (handler registry, dispatch processor, `NotReady`/timeout semantics) and `Source/CkEcs/Public/CkEcs/Net/EntityReplicationDriver/` (the recipe-replication path, `CkEntityReplicationDriver_Fragment.cpp:162-307`).
- Scheduler trait plumbing the gate mirrors: `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorTraits.inl.h` (`BuildDescriptor`), `CkProcessorDescriptor.h` (`PumpPolicy` precedent), `CkProcessorScheduler.cpp` (main pass + pump + version cache).
- Current restore core for the Model A half of the comparison: `Source/CkSnapshot/Public/CkSnapshot/Snapshot/CkSnapshot_Restore.cpp`, `Subsystem/CkSnapshot_Subsystem.cpp` (load state machine), `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_RestoreMarker.h` (the "every new replicated feature needs its own processor" comment).

Engine/env facts (verified 2026-07-02 per root CLAUDE.md): UnrealEngine-Angelscript **5.7.x** (5.7.4 on disk), EnTT **3.16.0** vendored, global `in_place_delete` tombstone mode via the `component_traits` specialization in `CkHandle.h:71-77`.

### Design decisions already settled this session (do NOT relitigate unless you see a real problem)

1. **The problem is structural, not incidental** — quantified: 119 `CK_REGISTER_SNAPSHOTABLE` sites, 12 features of hand-copied restore processors + 9 done-tags, forced RoundTrip/Transient classification per holder/record family, plus per-feature restore special-cases still accreting (three uncommitted right now).
2. **Convergence with replication is the goal, per the tech director's directive** — the review question is *how*, not *whether* save/load and replication should share per-feature code.
3. **Staged migration, Model A green until cutover** — no big-bang; each phase independently shippable with its own gate.
4. **Six pillar claims were adversarially verified** (six independent reviewers instructed to refute; all PARTIAL, corrections folded into the spec): normal-spawn re-seeds replication (restore processors deletable); no generic "setup settled" predicate exists (counterexamples: Goap, CrowdAgent inverted marker, Probe hidden aggregate, chained setups); net recipe-replay confirmed but disk needs handle-remap + non-asset-ref ensures; 8 features mutate Params without replicating (closed list); label-identity trustworthy only for attributes + inventory containers; signals broadcast synchronously and cannot be scheduler-gated, and 3 processors misbehave at dt=0.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **The core bet:** "loading a save = the server late-joining its own past session." Does save-fidelity == late-join-fidelity hold as a *contract*, given the authority-side Params-mutation gap (spec §4.5 [V4])? Is the 8-feature closed list credible as *closed*, or is unreplicated authoritative mutation an open class that will keep leaking?
- **`Produce`/`Apply` on the existing `FCk_ReplicatedFragmentHandlerRegistry`** vs a separate persistence registry that shares payload structs — is coupling save to the net handler registry right, or does it entangle two lifecycles (a handler change for net semantics silently changes save semantics)?
- **Recipe capture** (spec §4.2): are the disk-specific constraints sufficient — handle-remap through `FSnapshotContext` for spawn-params members, loud ensure on non-asset object refs, CDO fallback for non-asset archetypes? What's missed?
- **Identity rules** (spec §4.2 [V5]): recipe-spawned entities id-mapped by the loader; Construct-created children label-matched only where the record enforces uniqueness; unlabeled children save-transient by loud contract. Is that split principled or a wedge that grows case-by-case?

#### B. Convention compliance

- `ECk_ProcessorLoadPolicy` mirrors `PumpPolicy`/`NetModeRequirement` trait plumbing exactly (one enum + descriptor field + `if constexpr` in `BuildDescriptor` + graph-node mirror). Right shape? Right default (gated)?
- Moving `FProcessor_ReplicatedFragments_Dispatch` to a late group (immediately before `FGroup_Replication`) — the spec claims this preserves the applied-before-`OnReplicationComplete` contract pinned by the `Ck.Attribute.Net.*` tests while fixing the Setup-stomps-apply class. Verify against `CkProcessorGroups.h` ordering and the two-signal lifecycle contract in `CkEcs/CLAUDE.md`.
- Naming: "persistence handler registry," `Produce`, hydration/fidelity-oracle vocabulary — consistent with house lingo?

#### C. Version-specific API specifics

- EnTT usage shrinks to `basic_continuous_loader` id-remap for recipes/topology (no more per-fragment `get<T>` for feature fragments). Any 3.16 contract the recipe path still leans on that the spec doesn't state?
- `FInstancedStruct` disk round-trip of spawn params under `FObjectAndNameAsStringProxyArchive` on UE 5.7 — the 2026-05 review verified this for 5.5; anything changed?

#### D. Highest-weight spot-checks (the claims the design stands on)

- `CkVelocity_Utils.cpp:41-44` + `CkVelocity_Processor.h:235-238` — normal Add seeds the rep container; the restore processor's own comment says Construct-abstention is its sole reason to exist.
- `CkVelocity_Utils.cpp:94` — `Request_OverrideVelocity` is an **immediate** write (why ordering must be global, not request-deferral).
- `CkEntityReplicationDriver_Fragment.cpp:227-272` — clients rebuild by re-running Construct with replicated spawn params (the recipe precedent).
- `CkPredictedVelocity_Processor.cpp:40-48` — the dt==0 divide + `_PreviousDeltaTime=0` poison, allegedly reachable by **today's** pre-save `Request_PumpToQuiescence`.
- `CkRecord_Utils.h:949-968` — label uniqueness enforcement (the boundary of label-matching).

#### E. Risks — sized correctly?

- The spec names the **same-frame pump-ordering race** (§4.4) as the claim most likely wrong, pinned by a Phase-2 repro test before Phase 3 builds on it. Is that sequencing sufficient, or does it deserve a Gate-0 spike?
- Save-format hard break at Phase 3/5 (no migration path exists today either). Timing argument: do it before real player saves exist. Agree?
- Construct-replay cost at load vs image-restore cost — unmeasured (no perf claims made). Does any phase gate need a perf budget before you'd sign?

#### F. The five open forks (spec §7 — your ruling wanted, not just review)

1. Unlabeled Construct-children save-transient (recommend: accept; naming is the designer fix).
2. Modifier persistence (recommend: keep save-transient; final values bake via synthetic modifier).
3. Mid-load construction-signal semantics (recommend: accept — identical to normal boot).
4. Fate of image capture (recommend: keep as test-only fidelity oracle).
5. Format-break timing (recommend: now, pre-ship).

### Output format — fill in the CTO Review Response section below

Be direct. If the design is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers tied to spec sections, not vague concerns. If you rule CHANGES REQUESTED, give the exact, minimal sign-off conditions.

---

## CTO Review Response

### Verdict

**CHANGES REQUESTED** — on the spec text, not the direction. Model B is the right call, retiring the registry-image model is warranted by the evidence, and the five forks are ruled below (four as recommended, one conditionally). Three contract gaps must be closed in the spec before Phase 2/3 plan authorship. **Phases 0 and 1 are approved for plan authorship immediately** — nothing in the blockers touches them.

On reversing my v1 blocker-3 ruling, stated plainly: the 2026-05 ruling ("drop spawn params; Construct never re-runs; universal Construct idempotency is a far bigger contract change") optimized against one large contract change and instead bought a repair layer that charges the same cost in per-feature installments — [CkSnapshot_RestoreMarker.h:17-19](../../Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_RestoreMarker.h) promises the growth in writing ("every NEW replicated feature … needs its own such processor"), and this branch paid four more installments in a single day (`15434d8ef`, `5eda3ac8a`, `fa2b5ac9d`, `860ab0f2a` — the brief calls these uncommitted; they are committed on `feature/save-load-improvements` as of this review, which makes them better evidence, not worse). The contract Model B actually needs is narrower than v1 feared: *re-runnable-from-recipe*, which the client rebuild path already imposes on every replicated feature (`CkEntityReplicationDriver_Fragment.cpp:227-297`, verified — Construct re-runs with replicated spawn params). The one place that analogy genuinely breaks is Blocking issue 1. Reversal warranted; the evidence did its job.

### Blocking issues

1. **§3/§4.2 — the late-join analogy breaks for authoritative Construct side effects, and the identity split has a crack exactly there.** On a client, recipe replay is safe because authority-gated mutations inside Construct silently no-op and the real children arrive as separately replicated entities. On a **loading server**, Construct replays *with* authority: default grants (starting items, default abilities, driver-spawned subordinates) execute again while their saved counterparts also rebuild as first-class recipes → duplicates. Conversely, a Construct-granted child the player has since lost must be *removed*, and a value-overlay Apply has no absence semantics. The committed default-pawn suppression (`860ab0f2a`) is this exact bug class at the GameMode level — the design deletes that machinery without stating what replaces the general case. §4.2's split (runtime-spawned = recipe, id-mapped; Construct-created = label-matched) does not classify entities spawned *during an owner's Construct* that are individually persistable and destroyable (a starting inventory item is both). Spec must add: (a) the classification rule (a construction-window stamp is the obvious candidate); (b) **subtractive reconciliation** — hydration must be able to express absence (the handler contract's `Remove` path exists; record-bearing features need authoritative child-set semantics, not just per-child overlay); (c) how the loading-server rebuild avoids duplicate authoritative side effects **without reintroducing per-feature suppression machinery**. This is the one place "loading a save = the server late-joining its own past session" is currently false, and it is load-bearing for the whole §1 verdict.

2. **§4.4 — the residual-race closure is not an implementation detail; as sketched it collides with the pinned two-signal contract.** The DAG half of the dispatcher move is verified sound: every Setup-bearing group precedes `FGroup_Replication` ([CkProcessorGroups.h](../../Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h) group chain), so a pre-Replication dispatch slot still precedes `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete`. But pump passes run after the full main pass — so for an entity composed in `FGroup_Gameplay_Script` whose feature Setup lives in an earlier group, `NeedsSetup` drains in the pump *after* even a late dispatch, and the stomp survives the move for same-frame-composed entities. The sketched fix ("defer entries for entities composed this frame to the next tick") then pushes the **initial** apply to frame N+1 while the ReplicationComplete tag — added at construction completion (`CkEntityReplicationDriver_Fragment.cpp:290-295`) — fires in `FGroup_Replication` at frame N: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete` breaks, and not on an edge case but on the common newly-replicated-entity path. Note the committed Velocity/Acceleration fix (`5eda3ac8a`, NotReady-while-NeedsSetup) already quietly weakened that documented contract for those two features — evidence the contract needs a *global* resolution, and the late-dispatch redesign is the opportunity. Spec must pick the mechanism explicitly: gate the ReplicationComplete fire on the entity's pending-entry queue being drained (restores a uniform strong contract — my recommendation), or scope the deferral to load-hydration entries only and accept the documented per-feature softness for net. Phase 2's gate must list the two-signal lifecycle pins alongside the stomp repro.

3. **§5 vs §4.5/Phase 5 — the fidelity oracle's eyes are on the delete list.** The oracle diffs registry images via the capture machinery; Phase 5 deletes "raw fragment capture for feature fragments + most of the 119 registrations." Post-cutover — and for every future feature whose author "never thinks about save/load" — the oracle cannot serialize fragments that have no registration, so it goes blind precisely where §8's promise depends on it (the maintainer's #1 requirement is answered *by the oracle*). Spec must state the post-Phase-5 coverage mechanism: registrations retained under a test-only compile gate (accepting a small, test-only, non-behavioral per-fragment obligation — and saying so honestly, per the v1 review's "opt-in line, honestly" precedent), or a generic presence/topology diff plus opt-in value diff — and enumerate which fragment classes the oracle can and cannot see. Additionally the oracle's capture set must include `*_Params` fragments, otherwise the 8-feature Params-mutation class ([V4]) stays invisible to the very instrument meant to convert it from a one-time census into an enforced invariant.

### Non-blocking suggestions

1. **RepData structs become persisted-format surface** the moment a handler is Save-flagged. Net payloads are ephemeral (both ends run the same build); save payloads cross builds. Phase 1 doctrine must require tagged-property (UPROPERTY-walked) payload serialization — which tolerates field add/remove — plus CoreRedirects discipline on rename. This is strictly better than the current byte-exact image (which tolerates nothing); lock it in deliberately rather than inheriting it by accident.
2. **Count hygiene.** My grep counts 166 `CK_REGISTER_SNAPSHOTABLE` line-hits across 19 top-level module dirs vs the spec's 119/18 (definitions and comments likely inflate mine), and 23 `FCk_RepData_*` structs vs the spec's 24. Order-of-magnitude arguments unaffected; Phase 0's baseline doc should re-derive both with invocation-only patterns so the numbers are reproducible.
3. **Save-time audit warnings** (hydration payload targeting an unlabeled child, §4.2) must name the owner entity, the EntityScript class, and the child's fragment set — a designer must be able to act on the warning without learning snapshot internals, or the warning just relocates the footgun.
4. **Pre-save settle:** state explicitly that Model B keeps a pre-`Produce` quiescence step and whether it is kernel-scope or full-set (today's `Request_PumpToQuiescence` is full-set dt=0, reached via `CkSnapshot_Capture`). The Phase 0 dt==0 fixes land regardless — correct sequencing.
5. **Phase 2 gate:** add the inverse assertion — kernel-listed processors DO tick during the synthetic load window. Over-gating presents as a silent hang, the worst-diagnosable failure shape this design can produce.
6. **Perf:** agree no perf budget is needed for sign-off (no claims made — correctly). Phase 3's gate should *record* a load-time measurement on the representative world as a tracked baseline, not a pass/fail bar.
7. **UE 5.7 re-verify:** `FInstancedStruct` disk round-trip under `FObjectAndNameAsStringProxyArchive` was verified on 5.5 in the May review; I know of no 5.7 change but did not verify against 5.7.4 source — fold a one-assert smoke into Phase 3 rather than trusting a result across an engine bump. EnTT-side §C is clean: the May v2 addendum verified `basic_continuous_loader` against the shipped 3.16 header, and its caveat (the loader only remaps ids routed through it) is honored by design — spawn-params handles route through `FSnapshotContext`, and `CkHandle.h:318-326` already documents exactly that contract on the handle fields themselves.

### Rulings on the five open forks (§7)

1. **Unlabeled Construct-children: ACCEPT save-transient.** I verified the boundary ([CkRecord_Utils.h:949-975](../../Source/CkRecord/Public/CkRecord/Record/CkRecord_Utils.h)): label-matching is enforceable only under `DisallowDuplicateNames`, and unlabeled entries are rejected under that policy — the proposed contract lands exactly on the line the code already draws. Do NOT mint persistent per-child IDs: that is a parallel identity system (SaveKey reborn at child granularity) and re-imposes the per-feature bookkeeping this design exists to delete. Naming the timer is the designer-visible fix; the loud audit (suggestion 3) is the enforcement.
2. **Modifiers: KEEP save-transient; final values bake via the synthetic-modifier Apply.** Matches replication's client-visible contract and today's de-facto behavior. Document the double-count hazard by name: a game system that re-applies a persisted buff on load *on top of* a baked final value counts it twice — owning systems either re-author modifiers on hydrate (their own payload, through the request path) or accept the bake, never both. Revisit only on a concrete revocation-across-load requirement.
3. **Mid-load construction signals: ACCEPT — identical to normal boot.** Signals broadcast synchronously and cannot be scheduler-gated ([V6] holds); simulation signals stay silent because their broadcasting processors are gated. Worth stating in the doc as a win: Model B retires the old "signal bindings don't survive snapshot" contract class — BeginPlay genuinely re-runs, so boot-time binds are rebuilt by the same code that builds them at boot. Do not queue construction signals.
4. **Image capture: KEEP, test-only — conditional on blocker 3.** The oracle is the systemic answer to the maintainer's requirement; a "keep" without a specified post-Phase-5 coverage mechanism is hollow.
5. **Format break: NOW, pre-ship.** [CkSnapshot_Header.h:49-53](../../Source/CkSnapshot/Public/CkSnapshot/SaveGame/CkSnapshot_Header.h) confirms the format already hard-breaks with no cross-compatibility (the v1→v2 bump is on record). The cost is zero until real player saves exist; break at Phase 3 and build no migration tooling.

### Convention compliance spot-checks performed

All of §D's highest-weight claims were opened and **confirmed**:

- `CkPhysics/Public/CkPhysics/Velocity/CkVelocity_Utils.cpp:41-44` — normal `Add` seeds the rep container; `:94` — `Request_OverrideVelocity` is an immediate write (a `Request_` that violates non-negotiable #5; the spec's global-ordering argument stands on it, correctly).
- `CkPhysics/Public/CkPhysics/Velocity/CkVelocity_Processor.h:235-241` — the restore processor's own comment names Construct-abstention as its sole reason to exist; `CkVelocity_Processor.cpp:85-121` — Setup seeds `_CurrentVelocity` from Params (the stomp mechanism).
- `CkPhysics/Public/CkPhysics/PredictedVelocity/CkPredictedVelocity_Processor.cpp:24-48` — dt==0 divide (non-host) plus the `_PreviousDeltaTime = 0` poison that defers the divide to the host's next real tick.
- `CkEcs/Public/CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.cpp:150-307` — both recipe-rebuild paths (ConstructionInfos `:162-200`, EntityScript `:227-297` including the `UCk_Utils_EntityScript_UE::Add(..., EntityScriptClass, SpawnParams, ...)` Construct re-run and the `:290-295` ReplicationComplete tag timing that drives blocker 2).
- `CkEcs/Public/CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.cpp:290-354` — `IsNameStableForNetworking` ensure precedent (`:300-308`) and authority-side recipe capture (`:336-347`).
- `CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer_Processor.h:24-32` — dispatcher currently `FGroup_Gameplay_Script`.
- `CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h` — full group DAG; `CkProcessorDescriptor.h:80,138-140` + `CkProcessorTraits.inl.h:190-249` — `PumpPolicy` trait plumbing the `LoadPolicy` proposal mirrors (shape verified: enum + descriptor field + `if constexpr (requires {...})` detection; the new trait slots in cleanly at `:216`).
- `CkEcs/Public/CkEcs/Snapshot/CkSnapshot_RestoreMarker.h` (full) — the unbounded per-feature obligation, in the code's own words.
- `CkEcs/Public/CkEcs/Handle/CkHandle.h:318-338` — handle fields Transient with the remap-only persistence contract already documented in place.
- `CkRecord/Public/CkRecord/Record/CkRecord_Utils.h:935-984` — label-uniqueness boundary.
- `CkSnapshot/Public/CkSnapshot/SaveGame/CkSnapshot_Header.h:49-53` — hard version reject.
- `git log`/`git show` on `feature/save-load-improvements` — the four M2 repair commits (Exhibit A, now committed).
- Greps: `CK_REGISTER_SNAPSHOTABLE` census (166 line-hits / 19 module dirs), `ReplicateOnRestore|RestoreRedrive` headers (16 files across ~12 features — consistent with the spec), `RegisterLazy` census (21 files — matches "21 features with handlers"), `FCk_RepData_*` struct count (23), `Request_PumpToQuiescence` sites (subsystem + snapshot capture).

### Design / architecture observations

- **Model A vs Model B:** retirement is warranted. The decisive evidence is not the counts — it is that the repair obligation is *documented as unbounded* by the current model's own marker header, and that Model A's best hardening move (generalize the 12 restore processors into one framework re-drive) is literally Phase 1 of the Model B migration. The migration subsumes the alternative; if Model B later stalled, Phase 1 would still have been worth shipping. That is a well-shaped bet.
- **`Produce`/`Apply` on the shared registry (vs a separate persistence registry): endorse.** Convergence is the directive; a parallel registry sharing payload structs would re-diverge the two paths and double the per-feature declaration burden. The lifecycle-entanglement risk the brief names is real but managed: per-handler transport flags, plus suggestion 1's format discipline, plus the oracle. Be honest in §8's framing that the per-feature burden converges to replication's declaration burden — it does not go to zero; server-only stateful features (GOAP state, aggro tables, cooldowns) still owe a Save-flagged payload they never owed the net. The spec already says this; keep saying it.
- **`ECk_ProcessorLoadPolicy`:** right shape, right default. Safe-by-default is *the* property that makes the footgun-free promise credible — it is the exact inversion of `FSnapshotPolicy_*`'s forced choice, and it is why feature authors can stay load-ignorant. Per-processor (not group) gating necessity is confirmed by the Timer counterexample (Setup/HandleRequests/Update sharing `FGroup_Gameplay_TimeDelta` per the group header's own comment).
- **The six adversarial corrections are folded as design constraints, not appended as acknowledgments** — late dispatch exists *because* [V1]'s immediate-write finding killed request-deferral ordering; the design deliberately builds no "setup settled" predicate *because* [V2] refuted one; the dt==0 fixes are Phase 0 *because* [V6] showed today's pre-save pump is already exposed. That is what "corrections are sufficient" looks like, with two exceptions the blockers cover: [V5]'s identity split stops one case short (blocker 1), and [V4]'s closed list has no enforcement instrument unless the oracle can see Params (blocker 3).
- **Naming:** "recipe," "hydration," "fidelity oracle," "persistence handler registry" (rename at cutover) — all consistent with house lingo; `Produce`/`Apply` symmetry is good. No objections.

### Sign-off conditions (only if "CHANGES REQUESTED")

Spec revisions, each one section of text — no direction change and no new research required:

1. **§3/§4.2:** classification rule for Construct-window-spawned persistable children + subtractive reconciliation semantics + the loading-server duplicate-side-effect answer (blocker 1).
2. **§4.4 + §6 Phase 2 row:** the precise race-closure mechanism (fire-gating on pending-drained, or load-scoped deferral with the net softness documented), and the two-signal lifecycle pins added to Phase 2's gate (blocker 2).
3. **§5 + §6 Phase 5 row:** the oracle's post-Phase-5 capture mechanism, its coverage taxonomy, and `*_Params` inclusion in the capture set (blocker 3).

Phases 0–1 may be planned and implemented while these land. One re-review cycle scoped to the three revised sections flips this to GREEN-LIGHT; I do not need to re-read the rest.

---

### Reviewer

- **Name:** CTO (Claude Fable 5)
- **Date:** 2026-07-10

---

## v2 re-review addendum

**Date:** 2026-07-11
**Re-reviewer:** CTO (Claude Opus 4.8)
**Spec under re-review:** `docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md` @ v2 (`ec9587e58` on `feature/save-load-improvements`, unpushed).
**Scope:** §4.2, §4.4, §5 and their §6 rows only, per the sign-off conditions. I did not re-read the rest.

### v2 verdict

**GREEN-LIGHT WITH NON-BLOCKING NOTES.**

All three sign-off conditions are closed at the mechanism level, not merely acknowledged, and I verified the load-bearing claim under each against code rather than trusting the prose. Plan authorship may proceed for every phase. The notes below are for plan-writing / implementation, not spec gates — one of them (N1) is load-bearing enough that I want it named in §4.2 and reflected in the Phase 3/4 gate wording, but it does not block.

### Blocker → v2 resolution (each verified against code)

| Blocker | v2 location | Verification I performed | Verdict |
|---|---|---|---|
| 1 — loading-server Construct replay duplicates authoritative side effects; no absence semantics | §4.2 provenance taxonomy (EngineOwned / ConstructSpawned / RuntimeSpawned) + subtractive reconciliation + Construct doctrine line | The taxonomy's central claim is that ConstructSpawned is classified *mechanically at spawn time* by "is the lifetime owner pre-FinishConstruction." Confirmed the state is real and queryable: `FTag_EntityScript_ContinueConstruction` / `FTag_EntityScript_FinishConstruction` / `FTag_EntityScript_BeginPlay` / `HasBegunPlay` (`CkEntityScript_Fragment.h:23-27`), stamped by the spawn pipeline (`CkEntityScript_Processor.cpp:182`). So the classification is framework-central, not a per-feature decision — the property that made blocker 1 closeable. Subtractive reconciliation (rebuilt-labeled-but-not-saved → `Request_DestroyEntity`; saved-with-no-match → loud orphan) gives the absence semantics I asked for; ConstructSpawned carrying no recipe makes construction-window duplicates structurally impossible, mirroring the client's authority-gated suppression. | **Resolved.** See N1 — the taxonomy's cut line surfaces one adjacent coupling that needs naming. |
| 2 — race-closure sketch collides with the pinned two-signal contract | §4.4 fire-gating: `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` additionally requires the entity's pending-apply queue empty | This is the mechanism I recommended. Confirmed the fire-tag timing it hangs on: the tag is added at construction completion and fires in `FGroup_Replication` (`CkEntityReplicationDriver_Fragment.cpp:290-295`), which is exactly the same-frame edge that would have broken `Values_AppliedBefore_OnReplicationComplete` under a naive next-tick defer. Gating the fire on `FTag_RepFragments_PendingApply` absence makes "values applied before fire" true uniformly by construction; the 5s/2s dispatcher timeout bounds a stuck entry so there is no permanent hang. The spec correctly notes this *restores* the contract `5eda3ac8a` had quietly weakened per-feature. | **Resolved.** See N2 (dependent-aggregation subtlety — Phase-2-gated, not a spec gap). |
| 3 — fidelity oracle loses its capture machinery to the Phase 5 delete list; `*_Params` invisible | §5 three-tier taxonomy (structural / Produce-diff / `CK_WITH_FIDELITY_ORACLE`-gated deep-diff) + named blind spot; Phase 5 row retains registrations under the gate | Tier 1 (structural: entt storage iteration + type names, no serialize path) is a real EnTT 3.16 capability and genuinely covers fragments with *zero* registration — my exact concern ("the oracle cannot serialize fragments that have no registration"). `*_Params` are in the capture set from Phase 0, so the [V4] 8-mutator list has an enforcement instrument, not just a census. The residual blind spot (undeclared mutation on an ungated fragment) is named honestly with a review-rule + CI-grep mitigation — the "opt-in line, honestly" standard I asked for. Phase 5 row now *moves* registrations under the test-only gate rather than deleting them. | **Resolved.** |

Fork rulings (§7) are transcribed faithfully, including the modifier double-count hazard and the "BeginPlay genuinely re-runs" framing. §6 gate rows reflect each mechanism (Phase 2: the three lifecycle pins + inverse over-gating assertion; Phase 3: duplicate/absence tests + 5.7.4 `FInstancedStruct` smoke + load-time baseline; Phase 5: gate-not-delete). No objection to any.

### Non-blocking notes (fold into plan / spec text; none block authorship)

1. **(Load-bearing — name it in §4.2 and the Phase 3/4 gates.) RuntimeSpawned entities are only duplicate-safe once their *spawner's* control state is hydration-covered.** The taxonomy cuts at FinishConstruction: anything a driver spawns later — and the canonical case does exactly this, `BB_StoreDriver_Hfsm_Tasks.as:55,138,239,361,502,627,753,874` spawns subordinates from HFSM *tasks*, i.e. runtime, post-BeginPlay — is RuntimeSpawned and respawned from its recipe by the loader. That is correct and duplicate-free **only if** the spawning driver does not also re-execute the spawn after gate-open. The load-gate (§4.3) protects the during-load window (SM processors are gated, so tasks don't fire mid-load); post-open safety then depends entirely on the driver's SM resuming from its *hydrated* state rather than re-entering a pre-spawn state — which is precisely Phase 4's "SM redrive-as-hydration." So there is a real intra-migration coupling: **at Phase 3, a RuntimeSpawned subordinate whose spawner's SM state is not yet hydration-covered will double-spawn** (recipe respawn + post-open re-execution). This is not a hole — Tier 1 structural diff catches it as a duplicate-entity line — but Phase 3's gate says "zero *unexplained* diffs," and these diffs must be *explained* (annotated as Phase-4-pending), not treated as failures. Two concrete asks: (a) §4.2 should state the coupling — "a RuntimeSpawned entity's spawner must hydrate past the spawn decision or the two collide"; (b) the Phase 3 row should expect-and-annotate driver-respawn duplicates for not-yet-hydration-covered spawners, and Phase 4 should list closing them as an explicit exit criterion. The client analogy holds and is worth stating: a late-joining client is safe here only because the driver's re-spawn is authority-gated; the loading server has no such gate, so spawner-state hydration is load-bearing in a way it never was for net.

2. **(Phase-2 gate, already covered — flagging for the implementer.) Fire-gating must aggregate over dependents, not just self.** `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` fires on all-dependents-complete (`Get_IsReplicationCompleteAllDependents`). Gating on pending-apply-empty must hold for the whole dependent set, or a parent fires while a child still has an unapplied entry. The spec's Phase 2 gate (the three lifecycle pins + stomp repro) will surface this if it regresses; no spec change needed, but the plan should call out "pending-apply-empty is evaluated across dependents" so it isn't discovered by a red pin.

3. **(Trivial.) §2 still says "119 sites / 18 modules" and "24 RepData structs"; §4.5 says 24; my Phase-0-style grep gives 166 raw `CK_REGISTER_SNAPSHOTABLE` line-hits / 19 module dirs and 23 `FCk_RepData_*` structs.** The order-of-magnitude argument is untouched. Phase 0's "re-derive with invocation-only patterns" task (already in the §6 row) will reconcile these; just make sure the reconciled numbers replace the §2 headline figures so the diagnosis quotes its own reproducible count.

### Closing

The v2 author did the thing the first review asked for: closed each condition with a *mechanism*, and — on blocker 1 — added a construction-provenance taxonomy that is genuinely framework-central rather than a pile of per-feature suppression. My re-review's one substantive find (N1) is not a defect in the fix; it is the next layer of the same truth the fix exposed — that "loading = authoritative replay" makes spawner *control-flow* state as load-bearing as spawner *data* state, at the BeginPlay/runtime boundary the same way blocker 1 was at the Construct boundary. Naming it in §4.2 and threading it through the Phase 3/4 gates is a text edit, and the oracle already makes it non-silent. GREEN-LIGHT — proceed to plan authorship for all phases; land N1's text edits opportunistically, not as a gate.

### Re-reviewer signature

- **Name:** CTO (Claude Opus 4.8)
- **Date:** 2026-07-11 (v2 re-review)
