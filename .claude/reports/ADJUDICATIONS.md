# ADJUDICATIONS.md — escalations awaiting the maintainer's call

These are the genuine forks found during the 2026-07-02 documentation campaign: places where two
principals who know this framework could reasonably choose opposite sides AND the choice materially
shapes future code. Everything else was decided and recorded in [DECISIONS.md](DECISIONS.md).

Per the maintainer's standing instruction ("the agent just needs to ask me"), this file is the
ask-list. Each open item states both sides, the evidence, and the interim stance the docs teach
until a call is made. When you rule on an item, move it to DECISIONS.md with the ruling. Resolved
items keep their heading as a tombstone pointing at the DECISIONS entry (link stability).

---

## A1. `TOptional` in UPROPERTY / UFUNCTION surfaces — ban or allow?

- **Old doctrine:** never use TOptional in reflected contexts (historically a UHT limitation).
- **What code does now:** the 3 newest fragment-data files use it — 12 `UPROPERTY` TOptionals in
  `CkAudioDirector_Fragment_Data.h:44` and `CkPmg_Fragment_Data.h:173-191`. UE 5.5+ UHT accepts
  TOptional properties. The rest of the codebase (100+ modules) still uses the enum-mode + value
  pattern the ban produced.
- **Side A (keep the ban):** one optional idiom across the codebase; enum-mode + value pattern is
  BP/AS-friendlier (TOptional pins are awkward in BP; AS exposure needs checking); consistency with
  380+ existing enum-mode APIs.
- **Side B (lift the ban):** the mechanical reason is gone; TOptional is more direct; the newest
  code — presumably written with the most current judgment — already uses it.
- **Interim stance taught:** match the file/feature you are editing; do not churn existing APIs in
  either direction. New standalone features: prefer the enum-mode pattern until this is ruled.
- **Ruling needed on:** new-code default going forward.

## A2. C++ automation test pretty-name family in CkTests

- **Old doctrine (spec §14):** `CkTests.UnitTests.<Module>.<Subject>.<Scenario>`.
- **What code does now:** split corpus — roughly half the 195 hand-written C++ tests use the newer
  `Ck.<Feature>.*` family (Snapshot, Registry, Net already moved); generated net stubs emit
  `Ck.<Feature>.Net.AS_*` exclusively.
- **Side A (`CkTests.UnitTests.*`):** matches the written spec; groups all plugin tests under one
  automation-tree root.
- **Side B (`Ck.<Feature>.*`):** what the newest code and both generators emit; shorter; groups
  tests by feature next to their AS siblings.
- **Interim stance taught:** new tests follow the existing prefix of the feature family they join;
  greenfield features use `Ck.<Feature>.*` (the direction the generators already enforce).
- **Ruling needed on:** canonical family + whether the stragglers get renamed (rename churns CI
  filters and history).

## A3. Fragment storage `in_place_delete` / pointer stability — RESOLVED 2026-07-02, demoted to DECISIONS §45

- **STATUS: no maintainer ruling needed.** The question this item escalated ("was the ungated
  form intended?") is answered by git — which, per this campaign's own triage rule, settles it
  without a call. Full record: [DECISIONS.md §45](DECISIONS.md).
- **True chain (re-derived from `git log -G component_traits` + diffs, 2026-07-02):**
  `745507381` (2024-03-07) introduced the global `entt::component_traits<Type>` specialization
  INSIDE the `#if NOT …HANDLE_DEBUGGING` gate — debug-scoped at birth, message and code agreeing.
  `6b54d2e384` (2024-04-12) only relocated the still-gated block (a DEBUG_NAME storage fix).
  **`06938bba3` (2026-02-17, "feat: fragments are always pointer stable") deliberately lifted it
  out of `#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING`** — today's unconditional form at
  `CkHandle.h:71-77` is a considered design decision, not debug residue.
- **History (corrected):** `2c8319c1c` (2023-11-09, signal random-disconnect fix) added per-type
  in-class `static constexpr auto in_place_delete = true;` members to the signal fragments — NOT
  an `entt::storage_type` specialization. Those opt-ins are now shadowed by the global trait.
- **Scope note (still true):** pointer stability applies globally — every fragment type, every
  config including Shipping; EnTT owning groups are statically forbidden framework-wide
  (`group.hpp:697` static_assert rejects stable types).
- **What remains actionable:** the owning-groups performance ceiling is a prioritization question,
  not a doctrine fork — tracked as `ck-feature-frontier` candidate 5 (measure first).

## A4. Blessed "entity template / preset" pattern post-CkTemplate deletion

- **History:** CkTemplate and CkEcsTemplate were removed in `ad045415b` ("Remove deprecated
  CkTemplate and CkEcsTemplate modules"). Nothing was documented as their successor.
- **What code offers today:** EntityScript spawn params (`FInstancedStruct`, `CkEcs/EntityScript/CkEntityScript.h:65`),
  `CkProvider` data-asset providers, `CkEntitySpawner` placement actors, `CkDynamic` runtime
  composition. Each covers part of the old surface; none is named the archetype mechanism.
- **Side A (bless the existing pieces):** EntityScript spawn params + CkProvider already cover the
  preset surface; canonizing them costs nothing and matches what new code actually reaches for.
- **Side B (build a dedicated successor):** CkTemplate was deleted, not replaced; a purpose-built
  preset/archetype mechanism would unify four partial mechanisms instead of blessing the overlap.
- **Interim stance taught:** Source/CLAUDE.md's decision tree points "entity presets / archetypes"
  at EntityScript spawn params + CkProvider, explicitly marked INFERRED.
- **Ruling needed on:** the canonical answer to "I want a reusable entity preset" — so the docs can
  drop the INFERRED label and teach one pattern.
- **Related debt (no ruling needed, just awareness):** `CkScripts/CkEcsTemplateReplacer.ps1` still
  scaffolds from the deleted CkEcsTemplate module — stale tooling.

---

# Consumer campaign additions (2026-07-03)

Escalated per the same bar (two knowing principals could choose opposite sides AND the choice
materially shapes future games). Seven candidates surfaced during authoring; four were decided
below the bar (DECISIONS §47, §73, §93; N1–N6 routed as frontier nominations); these three stand.

## A5. Designer-facing trait composition — Venus trait entities vs BB code composition

- **Side A (Venus trait entities):** `TArray<UVnsTrait_ConstructionScript>` on the EntityScript;
  each trait = a child entity via entity_extension + context-override + gameplay label, findable
  by tag (`D:\Repos\Venus\Script\ECS\WeaponTraits\Vns_Base_WeaponTrait.as:24-53`). Designers
  compose variants in the editor without touching AS. `[SINGLE-EXEMPLAR]` — BB has zero
  `utils_entity_extension` uses (verified 2026-07-03).
- **Side B (BB code composition):** variants are thin `_Assets.as` subclasses; composition lives
  in DoConstruct code. Reviewable in diffs, testable asset-free, but designer changes need a
  programmer.
- **What each optimizes for:** A = designer ergonomics/iteration; B = diff-reviewability + the
  asset-free-test property (DECISIONS §55).
- **Lean:** teach B as the default (it is the tested, current-era corpus norm), offer A as the
  designer-heavy alternative. Not confident because the maintainer named Venus's "good practices"
  unprompted and trait entities are its standout.
- **Interim taught:** ck-game-entity-composition-patterns presents both, B as default, A
  provenance-labeled.

## A6. `ck::ScopedStat()` function openers — doctrine or project convention?

- **Side A (doctrine):** first line of every utils/processor function. Project-wide in BB
  (`257ff8df5`); it is what makes the drop-to-C++ rule (§47) measurable at all — without stats
  there is no "very clear perf concern", only vibes.
- **Side B (convention):** BB-only (Venus predates it); a whole-codebase mandate on every
  function body of every consumer is a real tax; the framework's own stat discipline lives in
  ck-performance-and-analysis without mandating per-function openers.
- **Campaign's own split:** two authoring agents independently landed on opposite sides
  (feature-recipe hedged; angelscript-gameplay promoted at medium).
- **Lean:** adopt as recommended default for new projects (serves measure-first doctrine, cost is
  one line), stop short of may-never-omit. Not confident because it is the kind of style mandate
  the maintainer has historically ruled on personally (cf. §30 named-namespace mandate).
- **Interim taught:** "adopt project-wide or not at all; the corpus does" — both skills aligned to
  this wording by the Phase-2 fixer.

## A7. Repo-root `.ignore` hiding `Script/` — search ergonomics vs tooling blindness

- **Side A (hide, BB's standing choice):** 185k+ lines of AS (plus 160k generated) drown ripgrep
  results for engine/plugin work; the maintainer's own environment keeps it hidden.
- **Side B (don't hide, bootstrap's recommendation for NEW projects):** the exclusion silently
  blinds every rg-backed agent tool to the project's PRIMARY gameplay language; the recurring
  cost is documented (root CLAUDE.md provenance caveat, this campaign's own AUTHORING_RULES had
  to warn every agent, memory records repeated zero-match incidents).
- **What each optimizes for:** A = human/agent search signal-to-noise on framework work;
  B = agent correctness on gameplay work (the dominant work in a consumer project).
- **Lean:** B for new consumer projects (hide `Script/Generated/` only), keep A in BB (its
  sessions skew framework-side). Not confident because it inverts a deliberate choice the
  maintainer made in his own daily environment.
- **Interim taught:** ck-game-project-bootstrap recommends the minimal `.ignore` (Generated-only
  hidden) and documents both stances + the rg --no-ignore escape hatch.

## A8. Empty-filter probe traces — should `InFireOverlaps` fire with no filter?

- **Context (2026-07-26 bug-fix pass):** `Request_MultiLineTrace`/`Request_MultiShapeTrace`
  (CkProbeTrace_Utils.cpp) early-return ALL hits when `Get_Filter().IsEmpty()` — necessarily,
  since `MatchesAny` against an empty filter matches nothing — but that early path also skips
  the `InFireOverlaps` Begin/EndOverlap block and the per-hit debug draw. The line/shape twins
  additionally disagreed on drawing the miss; that asymmetry was fixed (line now mirrors shape).
- **Side A (current, preserved):** empty filter = "return hits, no side effects" — overlaps and
  per-hit draws are filter-gated features.
- **Side B:** empty filter = "accept all" — overlaps and draws should fire for every hit, same
  as a match-all filter would.
- **Not decided:** B changes runtime overlap semantics for empty-filter probes; needs a
  maintainer call. Interim: behavior preserved (A), asymmetry only fixed.
