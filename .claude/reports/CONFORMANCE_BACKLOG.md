# CONFORMANCE_BACKLOG.md — BusterBlock vs the settled consumer standard

Produced by the consumer-skill campaign (2026-07-03). Each item is a BusterBlock deviation from
the standard the `ck-game-*` skills now teach (see [DECISIONS.md](DECISIONS.md) §46–97 for the
rulings). Ordered by leverage: correctness/footgun risk first, doc rot second, polish last.
Effort: S (<1 session) / M (1–3 sessions) / L (multi-session). Items marked ⚖ are blocked on an
open adjudication.

Headline: the hard contract is CLEAN — the boundary breach sweep found zero violations in BB game
code (no game-side C++ fragments/processors, no `_Requests` writes, no raw registry access, no
friend-boundary breaches, no stock ensures — DECISIONS §92). Everything below is softer drift.

## Correctness / footgun risk

1. **[M] Migrate the deprecated `_NonReplicating` autotest subclasses to the ActorRelay-channel
   form** — ~10 tests (Tests/Rental/*, DayCycle_DeferredConstruction, plus the
   `UBb_StoreDriver_EntityScript_NonReplicating` production-adjacent subclass). The exit ramp is
   named in the subclass's own header; the current form is DECISIONS §90. Exercises the real
   replication path instead of dodging it.
2. **[M] Replace the GOAP WS-dirty bootstrap seeds with real observers** —
   `Script/ECS/Npc/BB_Npc_EntityScript.as:925-938` (`WantsToRoam`, `AtWorkHours` INTERIM seeds).
   Maintainer-named band-aid class (worst debt). Also retires the SM↔planner WS-key oscillation
   throttle if the observers are done right.
3. **[M] StoreDriver defer-rescan retry budget → readiness promise** — the `66ac804db` retry
   budget self-describes as "interim shape"; replace with min-found polling + OnStoreReady promise
   per DECISIONS §67/§86 (timers are never the fix for discovery races).
4. **[M] Door/entryway spatial scoping → explicit store ownership** — `287ee6601`'s
   interior-anchored radius prune is rung 3 of the scoping ladder (DECISIONS §65); explicit refs
   are rung 1. In-code comment already flags it as interim.
5. **[S] DepositOrchestrator TransientEntity fragment-ghost leak** —
   `BB_CheckoutCounter_DepositOrchestrator.as:104-110`, FOLLOWUP noted in-code.
6. **[S] Direct static `UCk_Utils_IskmProxy_UE::Add(...)` call sites → `utils_iskm_proxy::Add`** —
   `BB_CandyDealer_EntityScript.as:488`, `BB_AmbientNpc_EntityScript.as:60` (works only because
   that Add isn't mixin-bound; violates the always-utils_* rule).
7. **[M] Dead `MinExpectedDoors/Counters/Gondolas` ExposeOnSpawn fields** — kept only for Params
   positional stability (DECISIONS §59 append-only trap made them permanent); deletion needs the
   caller sweep it was avoiding.

## Config / repo hygiene

8. **[S] Remove the dead `_ProcessorInjectors` key** — `Config/DefaultCkFoundation.ini:3-4`;
   matches no C++ member in any plugin (DECISIONS §83). Harmless but teaches every reader a
   phantom mechanism.
9. **[S] Remove the stale gameplay-tags source line** — `Config/DefaultGameplayTags.ini:12`
   references `/CkTests/GameplayTags_Tests_CkDT`, which no longer exists anywhere in CkTests.
10. **[S] Fix the stale `.ignore` line** — references `Plugins/CkApplication/Content` but no
    CkApplication plugin exists in BB. ⚖ The bigger `.ignore` question (`/Script` hiding) is
    ADJUDICATIONS A7 — do not act on it until ruled.
11. **[S] Refresh the NeverCook comment** — `Config/DefaultGame.ini:126` still cites the
    superseded `Script/Dev/` test location; tests live in the BusterBlockTests plugin.

## Documentation rot (the campaign's template exists to prevent recurrence)

12. **[M] Rewrite BB root CLAUDE.md from `PROJECT_TEMPLATE/CLAUDE.md.template`** — stale
    BbGameMode/BbGameEngine C++ section (classes don't exist), stale `Script/Tests|Gyms` paths
    (moved to Plugins/BusterBlockTests in `68616a8a5`), Store/AI described as C++ subsystems but
    rewritten in AS, restated doctrine, no verified-stamps, no divergence ledger.
13. **[S] Start BB's divergence ledger** — its actual deviations (e.g. the broad `*.md` gitignore
    footgun, the `.ignore` stance pending A7) are currently undocumented.
14. **[S, framework-side] Gym spec §9 stale console commands** — uncorrected in
    `Plugins/CkTests/Script/Common/CkGym_CreationSpecification.txt` itself.
15. **[S, separate repo] Venus `Script/Claude.md` teaches removed `ck::SelfEntity`** (lines
    51/138) — tombstone or delete next time Venus is touched.

## Polish / convergence

16. **[S] Commit-message dialect convergence** — plain `feat: Added ...` style coexists with
    `type(scope):`; converge on the latter.
17. **[M] Adopt a per-feature `DebugSettings` struct shape** — BB uses ad-hoc DevViz files; the
    Venus struct (`Vns_Targeting_Feature.as:130-164`) makes debug-draw discoverable per instance.
    Optional adoption; pattern is provenance-labeled in ck-game-debugging-playbook.

## Provenance

Compiled 2026-07-03 from eleven authoring agents' verified findings (each item's evidence lives in
the skill that taught the standard it deviates from, or in DECISIONS §46–97). Re-verify an item
before working it — several are marked interim IN the code and may already be fixed; check
`git log --oneline -- <path>` first.
