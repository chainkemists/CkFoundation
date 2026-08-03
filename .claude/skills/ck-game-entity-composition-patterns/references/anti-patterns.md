# Anti-patterns — with the incidents that proved them

Reference for `ck-game-entity-composition-patterns`: composition shapes that have already cost a session, and why.

## 4. Anti-patterns — with the incidents that proved them

Maintainer ruling (2026-07-03): the two worst debt classes in consumer code are **anything
requiring global access** and **band-aid bootstraps**. Rows 1 and 4 are those, respectively.

| # | Anti-pattern | Incident evidence (verified unless marked) | Correct pattern |
|---|---|---|---|
| 1 | **Registry-wide tag discovery grabs foreign entities.** A global `TAG_<X>` scan on a map with two+ instances of your context claims the other one's entities. | BusterBlock commit `287ee6601` "fix(storedriver): scope door discovery to the store footprint"; in-code post-mortem at `BB_StoreDriver_EntityScript.as:621-657` — foreign entryways "both lure shoppers to the wrong building and thrash this store's occupancy"; fixed by interior-anchored radius pruning. | **Scope discovery to your context**: explicit dependency refs > per-scope tags > radius from an interior anchor. Never leave a raw global scan in shipping code. |
| 2 | **Discovery tag stamped before composition finishes.** Consumers race and see half-built entities. | Every StoreDriver binder carries the ensure: "tag stamped before the feature finished composing?" (`BB_StoreDriver_EntityScript.as:856-858`). | Stamp the discovery tag LAST inside `utils_<feature>::Add`, after all fragments/children exist. This is the #1 lesser-model failure mode — `ck-game-feature-recipe` owns the canonical warning. |
| 3 | **Self-overriding feature composed on a shared entity.** | The FlyerRecipient-on-agent bug — §3 Rule 3, `BB_Npc_EntityScript.as:566-576`. | Own scene-node child per such feature. |
| 4 | **WS-dirty bootstrap band-aid.** GOAP planners replan only on WorldState-dirty; seeding fake WS writes at spawn so the first plan happens is a band-aid, not a pattern. | `BB_Npc_EntityScript.as:925-934`: "planners only replan on WS-dirty — a tourist with no store … would otherwise park in Idle_StandWatch forever", hence a `WantsToRoam=true` bootstrap, with an `INTERIM`-marked sibling seed right below it. | Name it **DEBT** and backlog it. If you must bootstrap, mark the write `INTERIM` with the real observer named, exactly as the corpus does. Never teach it forward. |
| 5 | **EntityTagQuery All-mode consumed without delta-gating.** The query re-delivers the FULL match set every evaluate pass, by design. | Fix commit `531b0c956` "perf(ecs): gate driver discovery handlers on population delta" (verified in git log 2026-07-03). Frame-cost magnitude (~250ms/frame full-population rebuild) is `[INFERRED — session-memory only, not re-measured]`. | Consume `Result.Get_Added()` / `Get_Removed()` deltas (corpus: `Script/Objectives/Tutorial/BB_Objective_RewindRental.as:71-75`), or dedupe against your own bound-set fragment. |
| 6 | **Arming async promises before the entity is ready for an INLINE resolution.** Promises can resolve synchronously (dependency already ready); a handler that touches not-yet-written state hides in tests and breaks in production ordering. | `BB_StoreDriver_EntityScript.as:398-421` — "Arm the async inputs LAST (ck-promise-inline-resolution discipline)" + an `INVARIANT — DO NOT change` block on the deferred-SM-start hang mode; `BB_Npc_EntityScript.as:397-401` — cache `_CachedSelfEntity` BEFORE arming a promise that "fires … SYNCHRONOUSLY when the driver is already ready". | In DoConstruct: state → SM/dispatch → cached handles → arm promises LAST. Assume every promise can fire inline, this frame. |
| 7 | **Expecting an enter event when spawning inside a trigger volume.** | `[INFERRED — session-memory autotest gotcha; no in-code cite found in the 2026-07-03 sweep. Confirm against CkSpatial/Trigger sources before relying on it.]` | After spawning inside a volume, query current overlap state explicitly rather than waiting for OnEntityEntered. |
| 8 | **Replicated entity script spawned under a non-replicating owner.** | One line: [REP_DEBUG] flood, silently-broken tests — `ck-game-replication-patterns` owns the rule, evidence, and fix. | Spawn under the project's replicated anchor. |

---

