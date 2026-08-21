---
name: ck-game-persistence-recipe
description: 'Use when making a CkFoundation gameplay feature survive save/load — declaring fragment postures, writing setup that reads restored values, consuming load completion, holding references across a load, and round-trip tests; not for the snapshot machinery itself or for save-file/slot UI.'
---

# Making a feature survive save/load

The decision procedure for a feature author. The machinery it rests on — the v3 rebuild+hydrate
load machine, the capture classification rules, the hold, the handler registrar shapes — is
documented in `Plugins/CkFoundation/Source/CkSnapshot/Claude.md`, and **that doc wins any
disagreement with this skill**.

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| Understand or change the load machine, the hold, capture classification | `CkSnapshot/Claude.md` (reference, not a recipe) |
| Build the save/load MENU — slots, thumbnails, metadata | `CkSnapshot/Claude.md` § "Slot metadata" |
| Add a fragment/processor/handler macro to CkFoundation itself | `ck-macros-and-codegen` |
| Compose the feature in the first place | `ck-game-feature-recipe` |
| Decide test layer / prevent shared-world contamination | `ck-game-testing-discipline` |

---

## The whole burden, in three lines

1. **Declare a posture** on every fragment your feature adds — that is the entire opt-in/opt-out.
2. **Write the feature's normal Setup/reconcile so it reads Durable fragments as inputs** and
   rebuilds every Session fact from them. There is no restore hook.
3. **Only a consumer whose lifetime is decoupled from what it reads** (widget, subsystem,
   cross-entity task) needs `Promise_OnLoadComplete`.

**One predicate, said once.** When your code must ask "is a load running?", the consumer spelling is
**load-in-progress, read at the instant the work is DECIDED** — `Get_IsLoadInProgress`.
`Get_IsRebuildInProgress` is the **framework-internal spawn-suppression** predicate: not a consumer
guard, and never a way to time a read.

---

## Row 1 — Adding a persisted feature

*Some of this feature's state must survive a save. What do you write?* If the feature is
AngelScript, almost certainly no handler: every AS-declared fragment already rides CkDynamic's
blanket `Produce`/`HydrationApply` pair.

| # | Rule | Anchor |
|---|---|---|
| 1.1 | Compose a persisted feature in `DoConstruct` or `DoBeginPlay` — never from a gameplay-signal callback, whose firing processor is held during a load, so the feature never exists when its payload applies and the payload is dropped at the apply timeout. | `CkSnapshot/Claude.md` § "NotReady-before-any-mutation" |
| 1.2 | Declare a posture on every fragment the feature adds — that IS participation; an AS feature writes no handler of its own. | `CkDynamic/CkDynamic_Fragment.cpp` blanket registrar |
| 1.3 | Rebuild every Session fact from the Durable ones in the feature's own resident Setup/reconcile — the framework offers no restore hook, and a load-keyed "restore rebind" processor is an anti-pattern. | `CkSnapshot/Claude.md` § Anti-patterns |
| 1.4 | Register a C++ feature through a named participation shape (`Register_SaveOnly`, `Register_NetAndSave_SharedApply`/`_SplitApply`) — a `Produce` without a `HydrationApply` does not compile. | `CkEcs/Persistence/CkPersistenceHandlerRegistry.h` |

---

## Row 2 — Declaring postures

*Durable or Session?* The question is **"is this rebuilt from Durable facts?"** — not "is this a
handle", which is the coarse rule that has broken real driver→subordinate links.

| # | Rule | Anchor |
|---|---|---|
| 2.1 | Declare `Durable` when the world would be wrong without the fact, `Session` when the feature's own construction/setup rebuilds it — and write the one-line "rebuilt from X" reason beside every `Session`, because on an existing feature a `Session` declaration is a deliberate DROP of state that used to round-trip. | `CkEcs/Snapshot/CkSnapshot_Posture.h` |
| 2.2 | Make a handle `Durable` only when its target is itself persisted and lives outside the owner's construction subtree (a pooled-channel subordinate, a world singleton, another feature's entity); a handle to a construct-rebuilt child (SceneNode, probe, Interactable, UnrealComponent, Tween, Ism/Iskm proxy, Timer, StateMachine, Trigger, CrowdAgent, attribute) or to a `UObject` is `Session`. | `CkSnapshot/Claude.md` § "The incident class all of this comes from" |
| 2.3 | Split a fragment that mixes durable facts with runtime scratch into a Durable half and a Session half — field-level `UPROPERTY(Transient)` opt-out is retired and now reds the resolver. | `CkSnapshot/Claude.md` § "A declaration never overrides a derivation" |
| 2.4 | Never declare `Durable` on a tag, a delegate-carrying fragment or a `*Requests` fragment — those derive `Session`, and one that also carries a real value field reds as "SPLIT ME" rather than being silently reclassified. | `Ck.Snapshot.Meta.FragmentPostureCoverage` |

The ratchet's allow-list only ever shrinks, and a shipped game drains it to zero. An `Undeclared`
fragment is a red, not a transitional state.

---

## Row 3 — Writing setup that reads durable inputs

*Where is a restored value readable?* In a Setup/resident processor, or `Promise_OnHydrated`.
Nowhere else, by contract.

| # | Rule | Anchor |
|---|---|---|
| 3.1 | Read restored values from a Setup or resident processor (or `Promise_OnHydrated`), never from `DoConstruct`/`DoBeginPlay`, which observe construct defaults by contract. | `Ck.Snapshot.Ordering.BeginPlayObservesConstructDefaults` |
| 3.2 | Make the reconcile **resident and idempotent against the Durable fact**, dirty-marked by the feature — then it is correct on construct, on mutation, and after a load, without knowing a load happened. | `CkSnapshot/Claude.md` C3; the game's own exemplar index |
| 3.3 | Track work Setup still owes with a Session marker consumed by Setup and removed by the hydration handler — never by comparing a value against its starting param, which is indistinguishable for the entity whose saved value IS that param. | `CkSnapshot/Claude.md` § "The mirror rule" |
| 3.4 | When a value arrives through the handler's deferred requests, read it from the feature's own request-drain/reconcile, not at the hydrated edge — `Applied` means enqueued, and the write lands later. | `CkTimer/.../CkTimer_Fragment.cpp:52-79` |

---

## Row 4 — Consuming load completion

*Which promise, and what does it actually claim?*

> **Ready to resume means every payload applied, every request those applies issued drained, physics
> stepped, probe overlaps converged, and no game time elapsed since the load began — and nothing
> more: construction and `DoBeginPlay` ran throughout, values a handler restored by enqueueing
> deferred requests may still be in flight in their feature's own queue, and any subsystem that
> rebuilds on its own retry cadence — the input intent stack, discovery/acquire tickets, an
> asynchronous asset load — is still rebuilding after the world is handed back.**

| # | Rule | Anchor |
|---|---|---|
| 4.1 | Use `Promise_OnHydrated` (bound from `DoBeginPlay`) for anything scoped to one entity, and `Promise_OnLoadComplete` only for a consumer whose lifetime is DECOUPLED from what it reads — widget, subsystem, cross-entity task. | `UCk_Utils_Snapshot_UE::Promise_OnHydrated` / `::Promise_OnLoadComplete` |
| 4.2 | Reconcile level-triggered for anything neither promise claims — physics and probe-overlap facts, and any subsystem rebuilding on its own retry cadence — instead of reading it once at the promise edge. | `CkSnapshot/Claude.md` § "What the promise may claim" |
| 4.3 | Bind `Promise_OnLoadComplete` from a GameInstance-lifetime object, or watch the epoch-stamped `READY TO RESUME` breadcrumb: the promise survives the load's travel, but a delegate target on a pre-travel entity does not. | `CkSnapshot/Claude.md` § "Why the subsystem and not an entity signal" |
| 4.4 | Branch on `Get_DidLoadComplete(Result)`, never on `== Success` — a `Succeeded_WithLoss` load completed, and its losses are named in the report. | `UCk_Utils_Snapshot_UE::Get_DidLoadComplete` |

### Consumer checklist

1. **Same entity?** Setup/resident processor or `Promise_OnHydrated` — not `OnLoadComplete`, which
   answers the strictly later, coarser "is the world coherent".
2. **Decoupled lifetime?** `OnLoadComplete` is your hook — and route any world-scoped read taken
   from a callback that can fire mid-load through it.
3. **Bind from something that survives the travel** (see 4.3).
4. **Branch on `Get_DidLoadComplete`, never `== Success`.**
5. **Needed a fact the sentence does not promise?** Reconcile it level-triggered from the durable
   fact — resident, idempotent, dirty-marked — instead of sampling once at the promise edge.
6. **`Promise_OnHydrated` is not a restored-vs-fresh discriminator.** It fires immediately for a
   fresh entity too (`CkSnapshot_Utils.cpp`, the "nothing pending" branch); discriminate on the
   Durable carrier handle instead.

---

## Row 5 — Holding references across a load

*A handle you kept is a handle the load may have tombstoned.* This is the dead-HUD / dead-panel /
despawned-NPC class.

| # | Rule | Anchor |
|---|---|---|
| 5.1 | Let a `Durable` fragment hold a handle only to an entity that is itself persisted — the posture ratchet does NOT catch this structurally, so the save-time capture AUDIT line is the only detector, and it fires on every save. | `Snapshot/CkSnapshot_CaptureV3.cpp:447-463` |
| 5.2 | Never park a load-bearing world reference in an `ExposeOnSpawn` spawn-params handle field: those are remapped once, mid-rebuild, with no retry, and a target not yet mapped becomes `entt::null` permanently. | `CkSnapshot/Claude.md` § "Spawn-params handle refs are remapped MID-rebuild" |
| 5.3 | Re-resolve a cross-entity reference lazily on use — stored handle first, heal via the same discovery the spawn used, memoise the result — and treat an invalid handle as "not yet", never as "gone". | `CkSnapshot/Claude.md` § "The incident class"; the game's own exemplar index |
| 5.4 | Put a bounded grace and a loud escape **with its own driver** in front of any terminal action taken on a missing reference — absence one frame after resume is not absence. | `RESILIENCE_TENETS.md` tenets 7 + 8 |

---

## Row 6 — Writing the round-trip test

*A persistence change with no red-then-green did not happen.*

| # | Rule | Anchor |
|---|---|---|
| 6.1 | Ship the round-trip test in the same change as the persistence, and show it red without the change. | `ck-game-testing-discipline` |
| 6.2 | Assert the **observable** across **two** save/load cycles — presentation, count, roster, what a player would see — not the fragment's bytes. | `SaveEveryCycle` on `FCk_SnapshotRoundTrip_Spec` (CkTests) |
| 6.3 | Use a process-level test (Gauntlet) for anything needing navmesh, input, UI-visible state or physics convergence — a PIE-scoped autotest cannot observe those, and a green one there proves nothing. | `ck-game-testing-discipline` |
| 6.4 | Classify a red by its ERROR CONTENT, never by its name: a sole infra-error line in a test's window (asset-search DB, revision control, an audio decoder, a foreign console warning) is a roamer that stole an unrelated test. | `ck-tests-authoring-and-running` |

---

## Known gaps — recorded, do not fight them

| Gap | What it means for you |
|---|---|
| AngelScript has no save-transient marker/exclusion API (`utils_snapshot.as`) | A probe/infra tag added to a persisted subtree emits a capture-AUDIT warning script cannot silence. Maintainer item. |
| The hydration quarantine is not stamped on mapped-but-unhydrated entities on the ESCALATED path | A one-shot Setup is not reliably post-hydration when a load escalates — which content-heavy loads do. Rule 3.2's resident idempotent reconcile is the consumer-side answer regardless; the framework fix is an open fork (two attempted fixes were reverted after A/B: they changed *what* runs in the escalated window, not just when). |
| `Get_PlacementForOccupant`'s grid back-ref is written only for a valid occupant, and the load path passes possibly-invalid occupants by design | Branch on grid validity first; do not trust the occupant→placement chain after a load. |
| The posture ratchet does not structurally flag a `Durable` fragment holding a handle to a non-persisted target | It is the C1 structural rule's own prohibited shape. Rule 5.1's capture AUDIT is the detector — read your save log. |
