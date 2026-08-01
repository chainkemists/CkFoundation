# CkWebUmg — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-01 (branch `feature/webumg-campaign`, tip `e4edd5952`+boundary commit):** **Gate 2 ✅ CLOSED** — ±1px ratified by Adam; 9/9 L-pages green in the default suite; 884/884 full-suite vs 875 baseline. **Gate 3 opened** (Plan/Gate_03_Paint_Layer.md) — pixel harness first (work item 1), Slate paint inventory (item 0) alongside.
**Baseline being diffed against:** 884/884 (2026-08-01, `BuildTest-WebUmg-FullRegression2.log`).
**Next action:** Gate 3 items 0+1 — Slate paint capability inventory (cited), then FWidgetRenderer pixel harness with L-page sanity check.
**Blocked on:** nothing until Gate 3 exit (paint threshold + shadow decision + text tolerance = Adam). Open Adam-side items: R3 WebToUMG purchase; harness migration to CkTests when sibling work lands.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-31 | DECISION 0 = BUILD | PriorArt evidence chain; Adam approved | Never (gate passed) |
| 2026-07-31 | Module family = CkWebUmg | Adam picked; CkStyle collides | Never |
| 2026-07-31 | Extractor rides system Chrome via puppeteer-core (no bundled Chromium download) | Chrome present; smaller toolchain; version recorded per-extraction | If version drift bites (Gate_01 risk 1) |
| 2026-07-31 | CLI lives at `Tools/ckwebumg-extract/` (plugin root), outside `Source/` | UBT must never see it; brief calls it "outside engine" | If plugin packaging complains |

## Dated entries (append-only, newest first)

### 2026-08-01 (session 2, Gate 3 opened) — pixel harness calibrated; paint-gap baseline measured
- Gate 2 closed (±1px ratified), boundary commit `a94d787de`. Slate paint inventory cited (FSlateRoundedBoxBrush + FSlateBrushOutlineSettings CornerRadii `SlateBrush.h:130-143`; SComplexGradient/SSimpleGradient exist) — radius/borders/axis-gradients need no materials.
- **Pixel harness landed** (`CkWebUmg_PaintFidelity_Test.cpp`, FWidgetRenderer → RT → per-pixel vs golden PNG, text-leaf rects masked; L-pages gate at 0.2% failing budget, P/T report-only until Adam ratifies a paint threshold). Three bring-up defects fixed with evidence: RT resource init; **toolbox runs automation with `-nullrhi`** → pixel lane requires `--no-nullrhi` (flag exists exactly for this; CI needs a second lane invocation — rect suite stays in the default lane); **FWidgetRenderer ctor arg is bUseGammaSpace** — `true` double-encodes sRGB (probes matched sRGBencode(golden) on the exact transfer curve: 12→61, 190→224, 60→133) → `false`.
- **Calibration proof:** L1–L5, L8 at exactly 0.0000% failing pixels (2,073,600 compared each). Rect-green ⇒ pixel-green for solids holds.
- **Measured paint-gap baseline (per-page failing %, channel delta > 4):** L6 2.89% (z-index paint order unimplemented — §8.6 stacking, now quantified); L7 29.36% (overflow:hidden clipping unapplied); L9 3.20% (unattributed 60,000 px — needs diff-image dump, next item); P1 radius 2.22%; P2 gradients 32.41%; P3 shadows 6.06%; P4 opacity/transform 16.97%; P5 images 13.87%; smoke 0.08%; T1/T2/T3 ≤ 0.0003% (masking verified).
- Next: builder clipping (overflow) + z-order paint sorting + diff-PNG dump tool, then P-feature work items in Gate_03 order.

### 2026-08-01 (session 2, Gate 2 execution, cont.) — L-corpus 9/9 at ±1px, honestly
- **False-green caught and killed:** first harness run reported 9/9 with `nan` rects — under UE's default `/fp:fast`, `nan <= tolerance` folded to true AND Yoga's NaN-sentinel undefined system broke (finite 1920×1080 in → NaN out, proven by instrumented run). Fix: `FPSemantics = FPSemanticsMode.Precise` on CkThirdParty (UBT knob verified at `VCToolChain.cs:1265-1266`); harness now hard-fails NaN via bit-pattern `FMath::IsNaN` on every node. Memory note saved (`ue-fpfast-vendored-nan-code`).
- **Convergence 0→5→8→9**, each fix a named mechanism from exact deviation arithmetic (no tolerance widening):
  (1) `sizingAuthored` added to IR (authored sizes beat cross-axis stretch — the 640/320px family); (2) percent flex-basis → `SetFlexBasisPercent` (was parsed as px); (3) children of block parents get grow/shrink zeroed (computed flex-* is meaningless outside a flex context — L7); (4) min/max extracted as values and applied **only when binding at reference** — non-binding mins shift Yoga's grow distribution because **Yoga floors the flex-base to min pre-distribution while Blink clamps the target post-distribution** (L2's 402=202+200 signature; a real Yoga-vs-Blink divergence the harness caught exactly as designed); (5) grow-containers with auto basis bake used main size as basis (local-decomposition limit — nested panels are content-less leaves to the parent tree; documented).
- **Final: 9/9 L-pages ≤ ±1px on all non-text nodes, NaN-checked**; text-leaf measure callback verified firing with real constraint modes (W=AtMost/H=Exactly) and logging the Chromium-vs-Slate gap (e.g. 200×200 recorded vs 131×38 Slate default font) — Gate 3 data.
- Bring-up diagnostics stripped (Display → VeryVerbose per their own comments). Full 875-baseline regression run in flight (mandatory: Jolt now compiles /fp:precise).
- Corpus goldens re-extracted twice this session (sizingAuthored, minSize/maxSize) — SCHEMA.md updated in step each time.

### 2026-07-31 (session 2, Gate 2 execution) — module + Yoga compile green; harness running
- Baseline captured pre-change: **875/875 automation tests, 0 failed** (BuildTest-WebUmg-Baseline.log, 9m40s).
- Landed in Source/: Yoga @ c7668858 vendored into CkThirdParty (19 TUs under `Public/CkThirdParty/yoga-c7668858/`, include path added to Build.cs — Jolt precedent confirmed cpps-under-Public compile in-module); CkWebUmg module (IR loader, SCk_WebUmgFlexPanel, builder, logs); uplugin entry after CkUsfEditor.
- Confirmed: DebugGame Editor build **Succeeded**, log shows all 19 Yoga TUs + 5 CkWebUmg TUs actually compiled (not skipped) — first-attempt green after staged self-review fixes (slot API, TStaticArray init, SNullWidget singleton mutation).
- Toolbox trap re-confirmed: `--generate` fails on this machine (project-file generator demands a VS2022 IDE install; UBT itself builds fine). Matches the existing memory note "drop --generate".
- Harness (`Private/CkWebUmg_LayoutFidelity_Test.cpp`, complex automation test over `corpus/golden/L*.ckui.json`, ±1px on non-text nodes, text leaves report-only) added; build+test with pattern `WebUmg` in flight.
- CkTests deviation recorded in DECISIONS.md: harness interim home is CkWebUmg (CkTests' Build.cs dirty with sibling WIP).

### 2026-07-31 (session 2) — Corpus complete (20 pages), 9 extractor bugs found and fixed, full determinism proof
- Committed Gate 0→1 checkpoint to **`feature/webumg-campaign`** (built from `dev` via temp index — working checkout is the sibling session's `feature/usf-grid-materials` and was not touched). First attempt force-added ignored `node_modules/` (`git add -f` on the dir); rebuilt the commit clean, verified `git ls-tree | grep -c node_modules` → 0.
- Delegated 19-page corpus authoring to an Opus executor agent (L1–L9 layout, P1–P5 paint, T1–T3+C1 text/controls, H1 hostile). Agent reported 9 extractor bugs; **I re-verified B1/B3/B6/B8 against the artifacts before fixing** (all held), then fixed all 9:
  B1 whiteSpace lost (Chrome 150 dropped the shorthand from computed list → synthesized from `white-space-collapse`+`text-wrap-mode`); B2 background-position missing (read `-x`/`-y` longhands); B3 `%` radii leaked as bare numbers (resolve against border box; elliptical → diagnostic); B4 alignContent never emitted; B5 per-side border colors silently dropped (→ computed-divergence diagnostic); B6 value-level gaps (`display:grid`/`position:sticky` now diagnosed via VALUE_RULES); B7 `::before/::after` invisible (→ diagnosed with rule file:line); B8 absolute machine paths in `source` (→ relativized; cross-checkout determinism restored); B9 `disabled` attribute invisible (→ `attributes` field for semantic DOM attrs).
- Bonus from agent findings: `inset.authored` records which edges the author pinned (Chromium resolves all four, erasing anchor intent — Gate 2/4 needs it).
- Ran: full re-extraction (20 pages, zero crashes) + per-bug assertion script (all 9 fixes confirmed in output) + determinism sweep — **all 20 IRs and all 20 PNGs byte-identical across two runs** (Chrome 150.0.7871.188).
- CEF probe closed: `-cefdebug=<port>` → CDP supported (`WebBrowserSingleton.cpp:343-347`); feasible but heavier than system Chrome; not adopted (DECISIONS.md standing note).
- Known-not-done: H1 duplicate `ck.name` nodes both still emit the name (emitter dedupe policy = Gate 4); `fonts[]` untested (no web fonts in corpus); T4 (mixed inline weights) folded into T3 by the agent — corpus is 20 pages counting smoke.
- Inferred (unconfirmed): PNG determinism across machines/GPUs — unchanged risk, per-machine goldens acceptable.

### 2026-07-31 (later) — Extractor working end-to-end on smoke page
- Built `Tools/ckwebumg-extract/` (Node ESM, puppeteer-core@24 on system Chrome; no Chromium download). `npm install` clean, 0 vulnerabilities.
- Ran: `node src/extract.mjs corpus/smoke.html` → IR + golden PNG. Confirmed by direct assertion script against the JSON: children re-sorted by computed `order` (0,1,2 ⇒ HEALTH/STAMINA/CONFIRM, matches golden pixels); `:hover` state diff captured via `CSS.forcePseudoState` (`{"hover":{"background-color":"rgb(70, 115, 195)"}}`); `data-ck-name/bind` extracted; `data-ck-ignore` subtree absent (string-searched whole IR); `%` width resolved absolute (62% → 223.36px).
- Confirmed: diagnostics fire with exact file:line — `backdrop-filter: blur(4px)` reported at `smoke.html:25`, verified against the file with `sed -n '25p'`. Two bugs found+fixed on contact: (1) injected neutralization CSS was self-diagnosed as author CSS → moved injection to inspector-origin `CSS.createStyleSheet` (harvester reads `regular`-origin only); (2) inline-`<style>` line numbers were sheet-relative → rebased via stylesheet header `startLine`.
- Confirmed: determinism — two independent extractions of smoke.html: IR byte-identical (`diff` empty) AND golden PNG byte-identical (`cmp` clean). Chrome/150.0.7871.188.
- Inferred (unconfirmed): PNG byte-determinism may not hold across machines/GPUs — Gate_01 risk row stands; per-machine goldens acceptable since the Gate 2 harness compares UMG↔golden on the same machine.
- Specs written: `Tools/ckwebumg-extract/SCHEMA.md` (IR v1 as implemented, incl. post-transform box caveat + temporary `computed` escapes for gradient/shadow pending Gate 3 parsers), `DATA_CK_SPEC.md`.

### 2026-07-31 — Campaign scaffolded; Gate 1 opened
- Gate 0 closed: PriorArt.md / VERIFIED.md / DECISIONS.md landed (Phase 0 originals archived at `D:\tmp\ckstyle-phase0\`); brief copied in as CampaignBrief.md.
- Confirmed: Node v24.14.0, npm 11.9.0, Chrome at `C:\Program Files\Google\Chrome\Application\chrome.exe` (commands run this session).
- Doc set created per ck-methodology (PROMPT/PLAN/PROGRESS/Gate_01).

## Open items
| Item | Status | Next step |
|---|---|---|
| Gate 1 work items 1–9 | in progress | see Gate_01_IR_Extraction.md |
| R3: buy WebToUMG (~$56) for baseline | waiting on Adam | Adam purchases; corpus run against it lands in Gate 2 evidence |
