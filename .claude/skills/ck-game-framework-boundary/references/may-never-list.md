# The MAY-NEVER list

Reference for `ck-game-framework-boundary`: every prohibited move across the game/framework boundary, each with the mechanical reason it is prohibited.

## 1. The MAY-NEVER list

Each row: the rule, the one-sentence rationale, and the doctrine citation. These are not style
preferences — every one is either a root non-negotiable the maintainer enforces in review, or a
maintainer ruling recorded 2026-07-03. `<Prefix>` = your game's class prefix (BusterBlock uses
`Bb_`, Venus uses `Vns_`).

| # | A game may NEVER… | Because… | Doctrine cite |
|---|---|---|---|
| 1 | Mutate ECS state outside the request pipeline | Utility functions enqueue; processors mutate — direct mutation breaks iteration safety, replication capture, and frame ordering | Root CLAUDE.md non-negotiable **#5**; `ckecs-architecture-contract` §3 "Requests — the deferred-mutation contract" |
| 2 | Touch fragment members directly / bypass the Utils surface | Fragments are friend-gated by design; Utils is the ONLY public API of a feature — everything else is an implementation detail free to change under you | Root CLAUDE.md "Lingo" (Utils row) + "Encapsulation" block; `ckecs-architecture-contract` §4 |
| 3 | Swallow errors — log-and-continue where validation failed, or use stock `ensure`/`ensureMsgf`/`check` | Maintainer rationale, verbatim: "logs get ignored, ensures do not"; stock ensures also compile out in Shipping while `CK_ENSURE_IF_NOT` stays active | Root CLAUDE.md non-negotiables **#2/#3**; decision table in `ck-change-control` "Never silently handle an error" |
| 4 | Ship a public C++ API verified in fewer than three environments (C++, Blueprint, AngelScript) | "Works in C++" is one third of done — AS is a first-class consumer of every public API on this engine fork | Root CLAUDE.md non-negotiable **#4**; verification recipe in `ck-angelscript-interop` §4 |
| 5 | Depend on Tier-5 / editor modules from runtime code | Editor modules don't exist in packaged builds; the link succeeds in-editor and the packaged game fails to boot | `Source/CLAUDE.md` §"T5 — editor modules (… runtime code must NEVER depend on these)" + "Module-authoring rules" |
| 6 | Author gameplay `AActor` subclasses when an EntityScript serves | EntityScripts are the placeable unit (~95% Actor replacement); the Actor path forfeits ECS lifetime, composition, and test harness support | Maintainer ruling 2026-07-03 (settled doctrine); exceptions in §1.6 below; archetypes in `ck-game-entity-composition-patterns` |
| 7 | Introduce global access / service-locator patterns (manager singletons, static registries, "get the X subsystem") | The maintainer's named **worst debt** category (2026-07-03); globals break scoping, multi-world PIE, and testability | Maintainer ruling 2026-07-03; sanctioned alternatives in §1.7; `ckecs-architecture-contract` §6 "there is no DI module" |
| 8 | Edit framework submodule code from a game session without switching to framework change-control | Framework changes carry their own gates (classification, three-environment verification, per-plugin versioning) that a game session isn't running | `ck-change-control` (whole skill); per-plugin versioning convention (§1.8 below) |
| 9 | Copy framework internals into game code to dodge a limitation | A fork-by-copy silently drifts from every upstream fix and hides the gap from the maintainer — that situation is a **gap report** (§2), never a paste | Root non-negotiable **#6** (unwritten-norm forks: ask); §2 below |

### 1.1 Mutations go through requests

The framework's write model: game code calls `utils_<feature>::Request_*` (or builds an
`FCk_Request_*` struct); the feature's processor drains the queue and mutates on its tick.
Consequence you must design around: **the mutation is not visible on the line after the call** —
read results via the feature's signal (`BindTo_On*` payload) or settle a frame. The four reasons
the contract is deferred, the copy-then-reset idiom, and the completion-delegate mechanism:
`ckecs-architecture-contract` §3. Corpus scale (BusterBlock, measured 2026-07-03): 2,682
`Request_*` call sites in hand-written game AS — this IS the mutation idiom, not an occasional
ceremony.

Game-authored features follow the same shape on their own fragments: an AS processor drains the
game's own request fragment. That is composition, not a breach — the rule is about *framework*
state, and about never mutating *any* ECS state outside a processor.

### 1.2 The Utils surface is the API; fragments are private

`ck::FFragment_*` runtime fragments declare their processors and Utils as friends; nothing else
can write `_Members` (root CLAUDE.md "Encapsulation"). From AngelScript you physically cannot
reach C++ fragment internals — the temptation exists only in game C++. Never add a `friend`
declaration to a framework fragment to let your class in (that is a framework edit — rule 8 —
and a design breach besides). If the read you need has no `Get_*`/`TryGet_*` on the Utils
surface, that is a **gap** (§2), not an invitation.

### 1.3 Errors are loud

```angelscript
// AngelScript form (the corpus standard — 221 uses in BusterBlock Script/, verified 2026-07-03):
if (ck::EnsureIfNot(ck::IsValid(Store), f"<Prefix>Checkout: no store bound for [{InHandle.ToString()}]"))
{ return; }
```

```cpp
// C++ form — never stock ensure/ensureMsgf/check:
CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid <Feature> Handle [{}]"), InHandle)
{ return {}; }
```

The recovery block must be a *correct* silent-failure path — not a fallback that hides the
problem. Expected, non-error absence is different: use `TryGet_*` (returns an invalid handle)
and branch on `ck::IsValid`. The ensure-vs-TryGet decision table: `ck-change-control`,
"Never silently handle an error".

### 1.4 Three environments — for YOUR public APIs too

Non-negotiable #4 binds any C++ a game ships as a callable surface (a `<Prefix>` utils BFL, a
save/load bridge): verify in C++, Blueprint, and AngelScript before calling it done. Blueprint
steps an agent cannot perform are labeled `[EDITOR-VERIFY]` with exact clicks
(`ck-change-control` §4). Most games rarely hit this rule — the corpus norm is AS-first gameplay
with near-zero C++ (see §5) — but it binds the moment you write reflected C++.

### 1.5 Tier discipline

`Source/CLAUDE.md`'s module tier table bands all 99 modules T0–T5; deps point same-or-lower,
and **runtime never depends on T5** (editor/UncookedOnly). For a game: your runtime game module's
`Build.cs` may list T0–T4 Ck modules; anything editor-only (test harnesses, editor tooling) goes
in a separate editor-target plugin. Corpus example (BusterBlock):
`Source/BusterBlock/BusterBlock.Build.cs` documents that the CkTests dependency was deliberately
moved to the editor-only `BusterBlockTests` plugin "so the game module never links it".

### 1.6 No gameplay Actors — and the legitimate exceptions

Maintainer-settled (2026-07-03): EntityScripts are the placeable unit. The corpus confirms the
~5% Actor remainder is principled, not habitual — each exception exists because an engine system
demands a real Actor:

- The **player pawn** (CharacterMovementComponent + replicated movement).
- **Physics props** (thrown items, Chaos destructibles — e.g. classes deriving
  `ACk_Chaos_FracturedDestructible_Base`).
- The **UE framework chain** (GameMode/GameState/PlayerController/PlayerState/HUD) — extend the
  `ACk_*_UE` bases and bridge to ECS in `EcsConstructionScript` / by spawning a
  `UCk_EntityScript_WithActor_UE`.
- **Actor-bridged stations** where a minigame needs real actor machinery
  (`UCk_EntityScript_WithActor_UE` pairs).

Everything else — NPCs included (BusterBlock's NPCs are pawn-less: zero `Possess`/`SpawnActor`
in `Script/Npc/`, verified 2026-07-03) — is a pure entity. Full archetype catalog and how pure
entities get visuals: `ck-game-entity-composition-patterns`.

### 1.7 No globals — the sanctioned alternatives

When you reach for a singleton, one of these is the answer:

| Need | Sanctioned mechanism | Where taught |
|---|---|---|
| "Who is my subject/context?" from a node deep in a composition | `ck::Ctx` context walking (ContextOwner; re-root with `Request_OverrideToSelf` — context root ≠ lifetime owner) | `ckecs-architecture-contract` §6; `ck-game-entity-composition-patterns` |
| Designer-tunable values resolved per-entity | CkProvider data-asset value providers | `ckecs-architecture-contract` §6 |
| A world-scoped coordinating brain | A **driver** EntityScript that discovers subordinates by tag query and injects handles downward | `ck-game-driver-architecture` |
| "The singleton might not exist yet" | Per-feature Acquire tickets with `Promise_OnReady` — dependency injection over re-discovery. Corpus example (BusterBlock): `utils_daycycle::AcquireDayCycle` | `ck-game-driver-architecture` |

Root CLAUDE.md "Lingo" says it flatly: "There is no module named 'DI' — these two
[ContextOwner, CkProvider] are it." Do not build the third.

### 1.8 Framework edits need framework change-control

The moment your cursor is inside `Plugins/CkFoundation/` (or CkTests/CkGameplayDebugger), you are
a framework contributor: load `ck-change-control`, classify the diff (docs-only / additive /
behavior / framework-invariant), and run its gates. Check the target plugin's `CLAUDE.md` for a
per-plugin **Versioning** section and follow its bump ritual if present (exemplar: GitLink's
`CLAUDE.md` — version define + `.uplugin` bump + version-log row + rebuild). Also keep the repos
straight operationally: commit the submodule change in the submodule repo, push it, THEN bump the
superproject's gitlink — a superproject pointer at an unpushed SHA breaks `git submodule update`
for every teammate.

---

