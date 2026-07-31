# CkStyle — HTML/CSS → UMG Toolkit
## Campaign Brief (feed this to the executing model)

**Owner:** Adam · **Host:** CkFoundation · **Target:** UE5, C++ · **Platform:** Windows / PowerShell / `D:\Repositories\`

---

## 0. Mission

Build a toolkit inside CkFoundation that turns an HTML+CSS mockup into UMG. A designer or the developer mocks a screen in a browser, iterates there (fast, hot-reload, real layout engine), then runs a conversion that produces UMG that matches the mockup — measurably, not approximately.

"Faithfully" is the load-bearing word in this campaign. It must be **defined as a number and enforced by a test harness** before any conversion code ships. If there is no pixel-diff gate, this project degenerates into a pile of heuristics that mostly work on the three test pages someone happened to try.

---

## 1. Operating rules for the executing model

These are non-negotiable and apply to every phase.

1. **Never assume, always verify.** Every architectural claim about UE5 internals (Slate arrangement, `UPanelWidget` slot semantics, `UWidgetBlueprint` asset construction, DPI curve application, `FSlateBrush` behaviour) must be backed by a `path:line` citation into the engine source or the plugin source. "I believe UMG does X" is not acceptable output. Read the code.
2. **Gate every phase.** Do not begin phase N+1 until phase N's exit criteria are met *and* Adam has explicitly approved. Present evidence at each gate, not a summary of intent.
3. **Present tradeoffs, do not resolve them silently.** Every decision point marked **[DECISION]** below stops the campaign until Adam chooses. Bring options with measured costs, not a recommendation dressed as a conclusion.
4. **Fix bugs on contact.** If a defect in existing CkFoundation code is found while working, fix it in the same pass. Do not file it. Exception only if the fix is genuinely multi-day and blocked on missing infrastructure — and then say so explicitly with the reason.
5. **No silent property drops.** Any CSS property the pipeline encounters and does not support must produce a diagnostic in the conversion report, with source file and line. Silent degradation is the primary failure mode of every tool in this category.
6. **Anti-sycophancy.** If Adam proposes something the evidence contradicts, say so and show the evidence. Do not fold on pushback without new evidence.
7. **Write down what you verified.** Each phase produces a `DECISIONS.md` and `VERIFIED.md` with `path:line` citations. Sessions lose context; the files do not.

---

## 2. Phase 0 — Recon and build/buy decision (GATE)

Do not write a line of pipeline code until this is done.

### 0.1 Prior art to actually evaluate (not skim)

| Thing | Why it matters | What to determine |
|---|---|---|
| **WebToUMG** (commercial UE5 plugin, ~mid-2026) — `https://www.youtube.com/shorts/LGb5o01Y6mA` | Directly overlapping product. May obviate the entire campaign or provide a fidelity baseline to beat. | Import modes, CSS coverage, whether output is editable WBP, whether flexbox reflows at runtime or is baked, licensing/source availability. |
| **rengeln/StyledWidgets** — `https://github.com/rengeln/StyledWidgets` | CSS-like stylesheet + selector engine for UMG, UE5.1-era, source available. | Is its selector/cascade engine reusable? How does it apply styles to `SWidget` at runtime? License. |
| **Yoga** (Meta's flexbox engine, C/C++) | The only realistic path to *real* flexbox semantics inside Slate. | License, build integration as a UE ThirdParty module, API surface for measure-callbacks (needed for text nodes), CSS Grid support (likely absent — confirm). |
| **litehtml** | Full HTML/CSS box model with a host-provided draw container; could emit widgets instead of pixels. | Flexbox/Grid coverage (expected weak — verify), whether "draw container as widget emitter" is viable, license. |
| **Chromium DevTools Protocol** (headless) | Gives *computed* styles and layout boxes, i.e. removes cascade/specificity/inheritance/`calc()`/custom-properties from scope entirely. | `DOM.getDocument`, `CSS.getComputedStyleForNode`, `DOM.getBoxModel`, `Page.captureScreenshot`. Confirm they return what this pipeline needs. |
| **UE `SWebBrowser` / Tracer WebUI** | The "just ship a browser" alternative. | Establish why it is rejected (perf, memory, console platform support, input/focus integration, no native UMG interop) — write this down so it is not relitigated every three months. |

### 0.2 Exit criteria

- A `PriorArt.md` with a verdict per row and evidence.
- An explicit **build vs. buy** recommendation with the reasoning, presented to Adam.
- **[DECISION 0]** Adam approves proceeding to build, or redirects.

---

## 3. Architecture — the shape being proposed

Three stages, decoupled by a stable intermediate representation. The IR is the most important artifact in the whole project; everything else is replaceable around it.

```
  HTML + CSS                Extraction                    IR                     Emission
 ┌───────────┐        ┌──────────────────────┐      ┌────────────┐        ┌────────────────────┐
 │ mockup/   │──────▶ │ headless Chromium via │────▶ │ *.ckui.json│──────▶ │ UMG                │
 │  *.html   │        │ CDP → computed styles │      │  (schema-  │        │ (WidgetBlueprint   │
 │  *.css    │        │ + box model + assets  │      │  versioned)│        │  or runtime build) │
 └───────────┘        └──────────────────────┘      └────────────┘        └────────────────────┘
                                │                                                    ▲
                                │  Page.captureScreenshot                            │
                                ▼                                                    │
                          golden PNG ─────────────── pixel diff harness ──────────────┘
```

**Why Chromium as the extractor rather than an in-engine CSS parser:** the browser *is* the specification. Asking it for computed styles removes cascade, specificity, inheritance, `em`/`rem` resolution, custom properties, `calc()`, shorthand expansion, and vendor prefixes from the project's scope — collectively that is more work than the rest of the pipeline combined, and getting it subtly wrong is exactly how "faithful" quietly stops being true. The same browser instance also produces the golden screenshot, so the fidelity oracle and the data source are the same engine.

**Cost of that choice:** a Node/Chromium toolchain dependency in the authoring pipeline. It is an *editor-time* dependency only — nothing ships in the runtime or in a cooked build. That constraint must hold.

### Proposed module layout

| Module | Type | Contents |
|---|---|---|
| `CkStyle` | Runtime | IR USTRUCTs, `UCkFlexPanel`/`SCkFlexBox`, brush materials, runtime widget builder |
| `CkStyleEditor` | UncookedOnly | Import factory, `UWidgetBlueprint` emitter, asset actions, live preview |
| `CkStyleHarness` | Editor / DeveloperTool | Fidelity harness, pixel diff, corpus runner, CI entry point |
| `ckstyle-extract` | Node CLI (outside engine) | CDP-driven extraction to `*.ckui.json` |

Names are a proposal; conform to whatever CkFoundation's existing module conventions dictate — verify against the existing modules before creating anything.

---

## 4. The decisions that must be made before implementation

Each of these changes the shape of the whole tool. Bring evidence, let Adam pick.

### **[DECISION 1] Layout runtime — how does flex actually run in UMG?**

| Option | Fidelity | Reflow / responsive | Idiomatic UMG | Maintenance |
|---|---|---|---|---|
| **A. Bake absolute positions into `UCanvasPanel`** | High at the authored resolution, degrades everywhere else | None — anchors only approximate it | Yes, but unreadable slot soup | Low |
| **B. Lower to native panels** (`UHorizontalBox`/`UVerticalBox`/`USizeBox`/`UOverlay`) | Lossy — flex-grow/shrink/basis, `align-items`, `gap`, wrap have no clean equivalents | Native | Yes, hand-editable | Low |
| **C. Yoga-backed `SCkFlexBox : SPanel`** | Real flexbox semantics | Real reflow, media-query-capable | No — a custom panel designers must learn | Ongoing Slate + Yoga upkeep |
| **D. C as the fidelity baseline + an optional lowering pass** that rewrites trivially-mappable containers into B | High, degrades gracefully | Real | Mixed | Highest |

Note when evaluating C: Slate's `ComputeDesiredSize`/`OnArrangeChildren` two-pass model needs to be reconciled with Yoga's measure-callback model, particularly for text nodes whose size depends on available width. **Verify this against Slate source with `path:line` citations before recommending C** — it is the single highest-risk assumption in this campaign.

### **[DECISION 2] Output form**

- **Generated `UWidgetBlueprint` assets** — inspectable, designer-editable, works with existing tooling; but asset generation via editor scripting is fiddly and diffs are binary.
- **Generated C++ `UUserWidget` subclasses** — text-diffable, code-reviewable, fits Adam's C++-first workflow; but not visible in the UMG designer.
- **Runtime construction from a `UDataAsset` holding the IR** — no codegen at all, hot-swappable, but nothing appears in the designer and Blueprint authors are locked out.

### **[DECISION 3] Regeneration model**

What happens when the HTML changes after someone has hand-edited the output? Proposed: **generated assets are strictly read-only** (`WBP_Foo_Generated`), and all hand-authoring happens in a subclass or in bound C++. Any "merge my edits back" scheme is a trap — it is unsolvable in general and produces silent data loss in practice. If Adam wants merge, the campaign needs an explicit conflict-resolution design first.

### **[DECISION 4] v1 CSS surface**

Draw the line explicitly and write it down. Proposed v1 in-scope: flexbox, box model, backgrounds/borders/radii, typography, images, `position: absolute/relative`, `overflow`, `opacity`, `transform`, `:hover`/`:active`/`:disabled`. Proposed v1 out-of-scope: CSS Grid, inline formatting contexts with mixed inline boxes, pseudo-elements, transitions/animations, `filter`, `clip-path`, `mix-blend-mode`. Out-of-scope items must still produce diagnostics, not silence.

---

## 5. Intermediate representation — sketch

Version the schema from day one (`"schema": 1`). Every field is a *computed*, resolved, absolute value — the IR contains no CSS syntax.

```jsonc
{
  "schema": 1,
  "source": { "html": "ui/hud.html", "viewport": [1920, 1080], "dpr": 1.0 },
  "assets": [ { "id": "img0", "src": "art/frame.png", "kind": "raster" } ],
  "fonts":  [ { "family": "Inter", "weight": 600, "src": "fonts/Inter-SemiBold.ttf" } ],
  "root": {
    "id": "n0",
    "tag": "div",
    "ck": { "name": "RootPanel", "widgetClass": null, "bind": null },
    "box": { "content": [0,0,1920,1080], "padding": [8,8,8,8], "border": [1,1,1,1], "margin": [0,0,0,0] },
    "layout": {
      "display": "flex", "direction": "column", "justify": "flex-start",
      "align": "stretch", "wrap": "nowrap", "gap": [8,8],
      "grow": 0, "shrink": 1, "basis": "auto",
      "position": "relative", "inset": null, "zIndex": 0
    },
    "paint": {
      "background": { "type": "color", "rgba": [12,14,18,230] },
      "borderRadius": [4,4,4,4],
      "borderColor": [80,90,110,255],
      "boxShadow": [ { "offset": [0,2], "blur": 8, "spread": 0, "rgba": [0,0,0,120], "inset": false } ],
      "opacity": 1.0,
      "transform": { "translate": [0,0], "rotate": 0, "scale": [1,1], "origin": [0.5,0.5] },
      "overflow": "hidden"
    },
    "text": null,
    "states": { "hover": { "paint": { "background": { "type":"color", "rgba":[20,24,30,230] } } } },
    "children": [ /* … */ ],
    "unsupported": [ { "property": "backdrop-filter", "value": "blur(4px)", "source": "ui/hud.css:112" } ]
  }
}
```

The `unsupported` array is mandatory and propagates into the conversion report. The `ck` object is the escape hatch (see §7).

---

## 6. Mapping table — the actual work

Build this as a living document in the repo; it is the spec the harness tests against.

| CSS / HTML | UMG target | Notes / hazards |
|---|---|---|
| `div` (flex container) | `UCkFlexPanel` (or lowered native panel) | Per DECISION 1 |
| text node, `p`, `h1-6`, `span` | `UTextBlock`; `URichTextBlock` when inline formatting present | Slate has no inline formatting context — mixed inline boxes are a hard limit |
| `img` | `UImage` + imported `UTexture2D` | Import as UI group, no mips, correct compression |
| `button` | `UButton` + child, with `:hover`/`:pressed` from state IR | |
| `input`, `textarea` | `UEditableTextBox` / `UMultiLineEditableTextBox` | |
| `ul`/`ol`/`li` | Flex column; markers via generated text | |
| `overflow: auto/scroll` | `UScrollBox` | Changes layout semantics — verify against Yoga output |
| `overflow: hidden` | `Clipping = ClipToBounds` | Costs a stencil/scissor — measure |
| `background-color` | `UBorder` brush tint or panel brush | |
| `border-radius` | **No native support.** Requires an SDF rounded-rect material brush in `CkStyle` | Known gap — build this early, it blocks most real mockups |
| `linear-gradient` / `radial-gradient` | Material-backed brush | |
| `box-shadow` | Material-backed brush or 9-slice | Outer shadows need layout headroom — they do not expand the box in CSS |
| `opacity` | `RenderOpacity` | Verify it composites per-widget-tree, not per-widget |
| `transform` | `RenderTransform` | Semantics align well: neither affects layout |
| `position: absolute` | `UCanvasPanel` slot with anchors + offsets | |
| `z-index` | Slot `ZOrder` / sibling order | CSS stacking contexts are more complex than paint order — scope this |
| `font-family` | Font asset mapping table (explicit config) | Web fonts must be imported as `UFont` |
| `letter-spacing`, `line-height` | `FSlateFontInfo` + text block settings | **Expect divergence** — see §8 |
| `text-overflow: ellipsis`, `white-space` | Wrap / clipping settings on the text block | |
| `display: grid` | `UGridPanel` / `UUniformGridPanel` for trivial cases only | Out of v1 per DECISION 4 |
| `px` | Slate units 1:1 at DPI scale 1.0 | Interaction with UMG's DPI curve is a real hazard — see §8 |
| `%`, `vw`, `vh` | Resolved by Chromium at extraction | Which is why the extraction viewport must be an explicit, recorded input |

---

## 7. Authoring ergonomics — what makes it usable rather than a demo

A pure CSS→UMG dump with machine-generated widget names is useless to gameplay code. The `data-ck-*` attribute set is the escape hatch and should be designed in Phase 1, not bolted on later:

- `data-ck-name="HealthBar"` → generated widget variable name, `IsVariable = true`
- `data-ck-widget="UCkProgressBar"` → force a specific C++/BP widget class instead of the inferred one
- `data-ck-bind="Health"` → emit a `BindWidget` or property binding hook
- `data-ck-ignore` → do not emit this subtree (mockup scaffolding, annotations)
- `data-ck-slot="Content"` → mark a named insertion point for runtime-injected children

Plus: a live preview path. An editor utility that watches the `.html` on disk, re-runs extraction, and rebuilds the preview widget in-editor is what converts this from a batch converter into a workflow. Scope it in Phase 5.

---

## 8. Known hard problems — do not let these get hand-waved

The executing model must produce a written position on each before Phase 4, not discover them during it.

1. **Text metrics will not match.** Chromium's shaping stack and Slate's font rasterizer produce different advances, line heights, and baselines for the same font at the same size. This is the number-one source of "why does it look 4px off." Decide a strategy: bake measured line boxes into fixed-size containers, accept a tolerance band, or make text a designated tolerance exception in the harness.
2. **`border-radius` has no native UMG path.** Everything modern depends on it. The SDF material brush is a Phase 3 blocker, not a nice-to-have.
3. **DPI scaling.** UMG applies a project-wide DPI curve. If the IR is extracted at 1920×1080 and the game runs at 2560×1440 with a 1.33 scale, does the output double-scale? Decide the reference-resolution contract and verify it against the engine's DPI application site with a `path:line` citation.
4. **Intrinsic sizing.** Percentage heights, `min-content`/`max-content`, and aspect-ratio boxes interact badly with Slate's desired-size pass. Enumerate which cases the pipeline supports.
5. **Inline formatting.** A paragraph with a bold span, an inline icon, and a link is trivial in CSS and has no faithful UMG equivalent. `URichTextBlock` covers some of it. Define the supported subset explicitly.
6. **Stacking contexts.** CSS stacking is not sibling order. Decide whether to flatten to paint order (and diagnose when that would be wrong) or model it properly.
7. **Asset determinism.** Two conversions of the same input must produce byte-identical output, or the harness is worthless and version control becomes noise.

---

## 9. Phase plan with exit criteria

Each phase ends with a hard gate: evidence presented, Adam approves, then proceed.

| Phase | Deliverable | Exit criteria |
|---|---|---|
| **0. Recon** | `PriorArt.md`, build/buy verdict | DECISION 0 made |
| **1. IR + extraction** | `ckstyle-extract` CLI, `*.ckui.json` schema v1, `data-ck-*` spec, 20-page golden corpus with reference PNGs | Extraction is deterministic across runs; schema reviewed; DECISIONS 1–4 made |
| **2. Layout runtime** | Layout implementation per DECISION 1, plus the pixel-diff harness | Harness runs the full corpus in CI and reports a per-page score; **layout-only** pages (no paint, no text) hit the agreed threshold |
| **3. Paint layer** | Brush materials: rounded rect, gradients, shadows, borders. Text styling. Asset import pipeline | Corpus pages with backgrounds/borders/radii hit threshold; text pages hit the separate text tolerance from §8.1 |
| **4. Emission** | Editor module producing the DECISION 2 output form; read-only generated assets per DECISION 3 | Round-trip: convert → open in editor → render → diff against golden, still within threshold. Regeneration is idempotent |
| **5. Ergonomics** | `data-ck-*` handling end to end, live-reload preview, conversion report UI | A non-trivial screen from an actual project (Grimveil or Rewind 99) converts and is wired to real gameplay data |
| **6. Hardening** | Coverage report, diagnostics for every unsupported property, CI integration, module `Claude.md` docs matching CkFoundation's existing documentation hierarchy | Corpus green in CI; unsupported-property diagnostics verified to fire on a deliberately-hostile test page |

---

## 10. Definition of "faithful"

This must be a number, agreed at the Phase 1 gate. Proposed starting point, to be argued with:

- **Layout:** every element's final rect within **±1 px** of the Chromium box model at the reference resolution.
- **Colour:** per-pixel ΔE below an agreed threshold on non-text regions.
- **Text:** separate, looser tolerance — a per-glyph-run bounding-box comparison rather than a pixel diff, because §8.1 makes pixel-exact text unattainable.
- **Coverage:** conversion emits **zero** silent drops. Every unsupported property appears in the report with `file:line`.

If a page cannot meet these, the harness fails and the tool says so loudly. It never quietly produces something that looks close.

---

## 11. First instruction to the executing model

> Execute Phase 0 only. Produce `PriorArt.md` with a verdict and evidence for every row in §2.1, including licence terms and, where source is available, `path:line` citations for the claims you make about how each one works. Then present a build-vs-buy recommendation with the reasoning exposed. Do not begin Phase 1. Do not write pipeline code. If you find that an existing solution already meets the §10 definition of faithful, say so plainly — the correct outcome of this phase may be that the campaign is cancelled.
