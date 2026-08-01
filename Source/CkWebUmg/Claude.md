# CkWebUmg

**Purpose:** Runtime side of the HTML/CSS→UMG toolkit (webumg campaign — `docs/campaigns/webumg/`).
Loads `*.ckui.json` IR documents (extracted by `Tools/ckwebumg-extract`, schema:
`Tools/ckwebumg-extract/SCHEMA.md`) and builds live Slate widget trees whose layout runs through a
Yoga-backed flex panel (campaign DECISION 1, option C).

**Type:** Runtime. **Depends on:** CkCore, CkLog, CkThirdParty (Yoga @ `c7668858`, MIT) + Slate/SlateCore/Json.

---

## Key API

- `ck::webumg::LoadIrDocument[FromFile]` (`Ir/CkWebUmg_IrLoader.h`) — `*.ckui.json` → `FCkWebUmg_IrDocument`.
  Hard-fails (ensure + empty optional) on schema mismatch or malformed structure; never a partial tree.
- `ck::webumg::BuildWidgetTree` (`Builder/CkWebUmg_Builder.h`) — IR → widget tree; returns the root
  plus an `IR id → widget` map the fidelity harness uses to compare arranged rects vs IR boxes.
- `SCk_WebUmgFlexPanel` (`FlexPanel/CkWebUmg_FlexPanel_Slate.h`) — one panel per CSS flex container;
  nested containers are nested panels (local Yoga passes — exact decomposition). Real layout runs in
  `OnArrangeChildren` (allotted geometry known); text leaves use Yoga measure callbacks.

## Load-bearing decisions (do not re-derive)

- **CkThirdParty compiles `/fp:precise`** — Yoga's undefined-dimension system is quiet-NaN +
  `std::isnan`; UE's default `/fp:fast` folds those checks away and NaN cascades through every
  layout (and `nan <= tolerance` can fold to *true* in tests — the harness's `FMath::IsNaN`
  bit-check guards against exactly that false-green).
- **Min/max clamps apply to Yoga only when they bound at extraction** (used == clamp): Yoga floors
  the flex-base size to min *before* grow distribution; Blink clamps the target *after*. Feeding a
  non-binding min into Yoga shifts every sibling's grow share.
- **Children of non-flex parents get grow/shrink zeroed** — computed `flex-*` values exist on every
  element but only mean something inside a flex formatting context.
- **Grow-containers with auto basis bake their used main size as basis** — nested containers are
  separate panels, so the parent's Yoga tree sees them as content-less leaves; content-driven basis
  is unrecoverable locally. Positions/gaps stay live; that node's grow share is fixed at the
  reference layout. Known v1 limitation.

- **Sizing policy (v1, reference-viewport contract):** items get explicit used sizes on any axis
  Yoga is not asked to compute; grow axes ride basis+grow, stretch cross-axes stretch, text leaves
  measure. This keeps Yoga math real without pretending the IR still has unresolved percentages.
- **Text measure returns the IR-recorded box** (Chromium's truth) and *logs* the Slate-side
  measurement (VeryVerbose) — the §8.1 font-metric divergence is collected as data for Gate 3, not
  imported into every sibling's position, and not silently baked either.
- `UseWebDefaults` on, `pointScaleFactor` = 1, box-sizing pinned border-box. Yoga's inert grid
  setters are never exposed (PriorArt §3 trap).
- IR structs are plain aggregates (Technique-Context precedent) — the reflected/asset form is
  Gate 4 scope, don't add UPROPERTYs here piecemeal.

## Anti-patterns

- Don't feed Yoga specified-CSS strings — the IR contains computed absolutes only; if a CSS-syntax
  string shows up outside documented escape fields, fix the extractor, not the consumer.
- Don't "fix" layout deviations by widening the harness tolerance — Gate 2's threshold is the
  product contract (±1px at reference resolution).
- Don't add paint richness here ahead of Gate 3 (SDF radius/gradient/shadow brushes land there).
