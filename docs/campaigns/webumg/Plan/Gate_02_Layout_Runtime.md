# Gate 2 — Layout runtime

> **Status:** 🟡 In progress (entered 2026-07-31)
> **Depends on:** Gate 1 ✅ (DECISIONS 1–4 decided — D1 = option C)
> **Estimate at entry:** 4–8 sessions (Phase 0 estimate, re-dated 2026-07-31)

## Goal

After this gate: an automated run loads each layout-only corpus IR (L1–L9), builds a live Slate widget tree through a Yoga-backed flex panel, arranges it at the reference viewport, and every node's arranged rect is within **±1 px** of the IR's `box` rect (the §10 layout tolerance, ratified with measured numbers at exit). The compiled proof of the Slate↔Yoga measure reconciliation exists.

## Entry criteria (ran 2026-07-31)

- [x] Gate 1 exit re-verified: DECISIONS recorded; corpus + goldens committed (`7ec0e6116`).
- [ ] **Build baseline captured via toolbox** before any engine-code change: clean editor compile + automation test counts (pass/fail + failing names). Record here.
- [x] Yoga pinned: `c766885` at `D:\tmp\ckstyle-phase0\thirdparty\yoga` (clone verified in Phase 0; re-verify SHA at vendor time).
- [x] Mimicry sources named (root non-negotiable #1): `CkThirdParty` vendor shape (EnTT/Jolt/fmt), `CkUsf` (small Runtime module with Slate/Graphics deps), `CkTimer` (quartet reference — though CkWebUmg is not an ECS feature module; it is Slate/asset infrastructure, closer to CkUsf's shape).

## Work items

1. **Vendor Yoga** into `Source/CkThirdParty/Public/CkThirdParty/yoga-c7668858/` (the `yoga/` dir only — 19 .cpp / 59 h; strip tests/bindings). Compiled TUs need a Build.cs touch: CkThirdParty is header-only today — if it has no .cpp compilation path, compile Yoga inside CkWebUmg instead (include-only from CkThirdParty). Resolve against the real Build.cs at execution; do NOT guess. NEW INFRASTRUCTURE — small.
2. **`CkWebUmg` Runtime module**: Build.cs (CkModuleRules), uplugin entry (Default phase, standard allowlist), `Claude.md`. Deps: Core/Log/Settings/ThirdParty + Slate/SlateCore/UMG. No ECS deps until a feature needs them.
3. **IR loader**: `.ckui.json` → `FCkWebUmg_IrNode` tree via FJsonObjectConverter-style parsing (harness path; the Gate 4 DataAsset import factory wraps this later). Schema-version check; hard error on mismatch.
4. **`SCkWebUmg_FlexPanel : SPanel`**: builds/owns a Yoga node tree mirroring its children. `OnArrangeChildren` runs `YGNodeCalculateLayout(alloted W/H)` and arranges from Yoga results; `ComputeDesiredSize` runs an unconstrained pass. Text leaves get measure funcs via `FSlateFontMeasure`. Config: `UseWebDefaults`, `pointScaleFactor` fixed (harness determinism), grid setters never exposed. This is the campaign's highest-risk compiled artifact — the PriorArt §3 paper argument becomes code here.
5. **Runtime builder**: IR node tree → widget tree (flex panels + `SColorBlock`-style solids + `STextBlock` for labels). Layout-only fidelity; paint richness is Gate 3.
6. **Rect-diff harness in CkTests**: automation test per L-page — load IR, build, arrange offscreen at 1920×1080, walk arranged geometry, compare each node vs IR `box`; report per-page max deviation and per-node failures. Runs headless via the toolbox (`--test`).
7. **Threshold ratification**: present measured per-page deviations to Adam; ratify ±1px (or argue from data).

## Expected observations at the gate — and branches

| I will run | I expect | If instead | Prewritten response |
|---|---|---|---|
| Harness on L1/L2/L5/L9 (pure flex) | max deviation ≤ 1px | small constant offsets | check border/padding accounting (Yoga inner-size vs Slate geometry origin) before touching the algorithm |
| Harness on L4 (wrap + align-content) | ≤ 1px | rows misplaced | check `UseWebDefaults` + alignContent mapping; Yoga wrap-reverse semantics vs extraction |
| Harness on L6 (absolute + inset) | ≤ 1px using `inset.authored` anchors | stretch cases wrong | verify all-four-sides path uses left+right as size, not width |
| Harness on pages with text labels | labels don't perturb fixed-size siblings | text measure shifts layout | measure-func returns wrapped size at AtMost constraint via FSlateFontMeasure; compare against IR text box |
| Double harness run | identical rects | jitter | pointScaleFactor / float accumulation — fix at source, never widen tolerance |

## Risks

| Risk | Sizing |
|---|---|
| Slate two-pass vs Yoga (the campaign's named highest risk) | Paper-resolved (PriorArt §3); the compiled proof is work item 4 — if it fails at ±1px, STOP per methodology §5 and write the addendum before any tuning |
| Yoga-vs-Blink divergence on corpus edge cases | The harness exists to measure exactly this; deviations become numbers, not vibes |
| CkThirdParty may not compile TUs today | Resolved at work item 1 by reading the real Build.cs |
| Editor running during builds | Toolbox + hook own this; never Build.bat directly |

## Exit criteria — ALL land in the same commit as the last work item

- [ ] All L-pages within ratified threshold; per-page numbers in PROGRESS.md
- [ ] Baseline vs exit automation counts diffed (no regressions claim requires the entry baseline)
- [ ] `ck-change-control` done-checklist for a new-module change
- [ ] Module `Claude.md` shipped; PLAN.md row + this header updated, same commit
- [ ] Adam ratifies the threshold → gate closes
