# Consumer-skill campaign — PROGRESS.md (living log)

> Campaign: `ck-game-*` consumer skill library + PROJECT_TEMPLATE + triage reports.
> Mission brief lives in the user's kickoff prompt (2026-07-03 session); stable content summarized
> under "Charter" below. **This doc dies when** the campaign's Phase 2 report ships; on death,
> replace body with a tombstone pointing at the shipped skills.

## Charter (stable)

- Build the CONSUMER layer: skills for building GAMES on CkFoundation, generic to any consuming
  project. BusterBlock is reference corpus, NOT the standard — triage divergences per decision
  rule (doctrine wins by default; uniform deliberate superior practice promotes with
  `[PROMOTED FROM CORPUS]`; only genuine principal-level forks escalate — expect 3–7 total).
- Write scope: ONLY `Plugins/CkFoundation/.claude/` (skills `ck-game-<name>/`, PROJECT_TEMPLATE/,
  reports/). Everything else read-only; no mutating git.
- Outputs: 8–12 `ck-game-*` skills, `PROJECT_TEMPLATE/`, DECISIONS.md + ADJUDICATIONS.md
  (APPEND to existing framework-campaign files — they use stable append-ordered IDs),
  CONFORMANCE_BACKLOG.md (new).
- Audience: zero-context mid-level / Sonnet-class sessions. Ground truth only; single-exemplar
  patterns labeled `[SINGLE-EXEMPLAR]`; editor-only steps `[EDITOR-VERIFY]`.

## Current state  <!-- update every session -->

**As of 2026-07-03 (session 1, Phase 1 complete):** All 12 deliverables ON DISK and verified:
10 skills at `.claude/skills/ck-game-{project-bootstrap 475, feature-recipe 522,
angelscript-gameplay 423, entity-composition-patterns 331, driver-architecture 401,
replication-patterns 367, debugging-playbook 351, testing-discipline 348, build-and-cook 313,
framework-boundary 298}/SKILL.md` (lines) + `PROJECT_TEMPLATE/{CLAUDE.md.template 98, README.md
74}`. Maintainer answered all 5 Phase-0 questions (rulings recorded in AUTHORING_RULES + DECISIONS
header): AS-first standard; driver MVC = proudest, endorsed; global access + band-aids = worst
debt; manual bootstrap; Venus = second consumer (recon done — venus-recon.md). Reports
consolidated: DECISIONS §46–97 + nominations N1–N6 appended; ADJUDICATIONS A5–A7 appended
(trait composition, ScopedStat, .ignore /Script); CONFORMANCE_BACKLOG.md created (17 items).
**Phase 2 COMPLETE (2026-07-03):** three reviewers ran — FACTUAL 0 blocking / 0 important /
3 minor ("unusually rigorous"; all 48 cited commits exist, counts exact); GENERICITY 0/5/5
(leakage clean; A5–A7 label placements + one skeleton dedup); USABILITY 0/2/4 (cold session
completes bootstrap + recipe end-to-end). Fixer applied ALL important + 11/12 minors across 7
files; 1 minor skipped (structural: e0de34899 retold in 3 skills — acceptable drift risk,
recorded here). One reviewer suspicion REFUTED by consolidator: `ck::OwnerEntity` exists
(CkUtils_Common.as:26,31) — replication skill untouched.

**CAMPAIGN DONE. Standing blocker for the maintainer:** CkFoundation `.gitignore:49` blanket
`*.md` rule — every campaign file (skills, reports, template) is on disk but UNTRACKED in the
submodule; nothing is committable until the un-ignore lands (verified via git check-ignore).
Also untracked-not-ours in the submodule: CkWatermark/Generated, ~25 CkUsf GeneratedLooks
uassets — left for their owning session.
**Blocked on:** maintainer — (a) un-ignore + commit, (b) rulings on A5/A6/A7, (c) veto scan of
DECISIONS §46–97.

### Phase 0 headline findings (evidence in scratchpad reports)

- **AS is ~97% of gameplay code**: 934 game .as / 185,725 lines vs 16 C++ files / 2,188 lines
  (SaveLoad glue, ABb_Character, nav filters); last 200 commits: 2,536 .as vs 22 .cpp diffs.
  BB authors ZERO C++ fragments/processors — 135 AS processor classes, 592 entity-script
  subclasses across 102 `Script/ECS/<Feature>/` dirs. BPs are shells (3 BIEs total).
- **Canonical feature arc** (from git): designer data → `_Feature.as` (dynamic handle + fragments
  + signals) → logic namespace + unit AutoTest same-commit → `_Utils.as` composer + compose test
  → processors + EntityScript spawn vehicle → driver integration + e2e test → placement Content
  LAST. Tests same-commit-as-behavior, uniformly.
- **9 entity archetypes**; 3 load-bearing ownership rules (ActorRelay owner for replicated spawns;
  context-root ≠ lifetime-owner; interactable on child probe-node).
- **Test layers**: 216 AutoTests / 23 gym features / 22 Gauntlet / 0 C++ unit tests. Coverage norm
  real for new work, patchy tail (~36 cosmetic dirs untested). Tests live in
  `Plugins/BusterBlockTests/` (moved after Shipping-staging incident `e0de34899`).
- **Top incident categories**: replication misuse; deferred/timing misuse; signal lifecycle
  (unbind leaks, channel mismatch); cross-test contamination; PIE-vs-packaged.
- **Wiring**: Ck plugins self-enable (`EnabledByDefault:true` — .uproject never lists them);
  minimal game C++ = one Runtime module; GameMode is a BP; engine GUID registration is
  per-machine. Project CLAUDE.md is stale in places (BbGameEngine C++ classes, test paths,
  Store/AI C++ described but rewritten in AS).
- **Triage already resolved**: `_ProcessorInjectors` key in DefaultCkFoundation.ini matches no
  C++ member in any plugin (rg over Plugins --glob *.h/*.cpp, 2026-07-03) — stale key, silently
  ignored; doctrine (CK_REGISTER_PROCESSOR) stands. → DECISIONS + CONFORMANCE_BACKLOG.
- **Doc-map**: 8 consumer gaps confirmed uncovered; adjudication constraints A1/A2/A4 bind
  consumer skills; ck-debugging-playbook/ckecs-* skills are the main cite targets.

### Proposed Phase 1 skill list (adapted; 9 skills)

bootstrap, feature-recipe, angelscript-gameplay, entity-composition-patterns,
debugging-playbook, testing-discipline, build-and-cook, framework-boundary,
**+ replication-patterns** (incident category #1 earns its own skill; driver/discovery folded
into composition; save/load = section in feature-recipe, corpus too thin for a skill).

## Decision log

| Date | Decision | Why |
|---|---|---|
| 2026-07-03 | Append consumer-campaign entries to EXISTING reports/DECISIONS.md + ADJUDICATIONS.md rather than new files | Files use stable append-ordered IDs and self-describe as continuing records; two parallel DECISIONS files would fork the doctrine change-log |
| 2026-07-03 | Campaign progress doc lives here (reports/), not docs/ | Write scope is .claude/ only |

## Dated entries (append-only, newest first)

### 2026-07-03 — CLAUDE.md.template accidental discard + restoration
- Maintainer's git client "discard" deleted `PROJECT_TEMPLATE/CLAUDE.md.template` from disk
  (not recycled). Root cause of exposure: its `.template` extension dodges the `.gitignore`
  blanket `*.md`, making it the ONLY campaign file visible as untracked — and therefore the only
  one a discard could hit. All other campaign files were unaffected (verified by tree listing).
- Restored by resuming the authoring agent (content from its context, byte-faithful) with the
  Phase-2 GEN-6 fix folded in. Verified: 124 lines / 99 non-blank (pre-deletion metric was 98
  non-blank; +1 = the GEN-6 comment wrap), GEN-6 wording present.
- Standing hazard until the `.gitignore` un-ignore lands: this file remains the one
  discard-vulnerable artifact. When un-ignoring, commit it FIRST.

### 2026-07-03 — session 1 kickoff
- Confirmed: 14 skill junctions at BB root `.claude/skills/` (Get-ChildItem, LinkType Junction),
  all visible in session skill list.
- Confirmed: no ck-game-* leftovers from the interrupted prior attempt (CkFoundation/.claude tree
  listed — only framework skills, reports {DECISIONS, ADJUDICATIONS}, scripts).
- Read: CkFoundation root CLAUDE.md in full (doctrine anchors for triage).
- Launched 6 read-only discovery agents; all instructed on the rg --no-ignore /Script blindness.

## Open items

| Item | Status | Next step |
|---|---|---|
| Phase 0 agent reports | running | synthesize on completion |
| ≤5 maintainer questions | pending | after synthesis |
| Phase 1 authoring | pending | after answers (or provisional if none) |
| Phase 2 review + fix | pending | after Phase 1 |
