# `*.ckui.json` — IR schema v1

> **Written:** 2026-07-31 (Gate 1, in progress — fields may still move until the Gate 1 schema review; this doc updates in the same commit as any extractor change that alters output shape).
> Contract: every value is **computed, resolved, absolute** (px, rgba 0–255, unitless numbers). The IR contains **no CSS syntax** — if a CSS string appears outside a documented `computed` escape field, that is a schema violation, not a feature.

## Top level

```jsonc
{
  "schema": 1,
  "source": {
    "html": "smoke.html",            // basename of the extracted page
    "viewport": [1920, 1080],        // extraction viewport — a recorded INPUT (brief §6)
    "dpr": 1,
    "browser": "Chrome/150.0.7871.188" // determinism is per-browser-version (Gate_01 risk 1)
  },
  "assets": [ { "id": "img0", "src": "ckui-assets/img0.png", "origSrc": "assets/red1x1.png",
                "kind": "raster", "intrinsic": [1, 1] } ],
                                      // deduped by resolved URL, ids in first-reference order;
                                      // src = BROWSER-NORMALIZED copy written next to the IR
                                      // (canvas-decoded: raw bytes are Chromium's color-managed
                                      // interpretation, gamma/color chunks resolved away) —
                                      // IR bundles are self-contained; origSrc = author's path
  "fonts": [],                        // loaded web fonts ({family, weight, style}) — system-font
                                      // stacks live on each node's text.family; mapping to UFont is emitter config
  "diagnostics": [],                  // page-level issues: {kind, node, detail};
                                      // kinds: "duplicate-ck-name", "unknown-ck-attribute"
  "root": { /* Node */ }              // the <body> element
}
```

## Node

Emitted depth-first; ids `n0, n1, …` assigned in **pre-sort document order** (stable across runs). Children are **sorted by computed `order`** (stable) — Yoga has no `order` property, so paint/layout order is baked here (PriorArt §3).

Omitted entirely: `display:none` subtrees, `data-ck-ignore` subtrees, non-rendered nodes (no box model), `<script>/<style>/<link>/<meta>/<title>`.

| Field | Type | Notes |
|---|---|---|
| `id` | string | `n<N>`, stable |
| `tag` | string | lowercase element name |
| `ck` | object\|null | the `data-ck-*` escape hatch — see [DATA_CK_SPEC.md](DATA_CK_SPEC.md); null when no ck attributes present |
| `asset` | string\|null | for `<img>`: id into top-level `assets` |
| `attributes` | object\|null | semantic DOM attributes (`type`/`value`/`placeholder`/`disabled`/`checked`/`alt`/`href`), verbatim; boolean attributes → `true`. Carries form-control state the computed style cannot (a disabled button's disabledness) |
| `box` | object | `content`/`padding`/`border`/`margin`, each `{x,y,w,h}` in **absolute page px** (2-decimal), from `DOM.getBoxModel` quads. Quads are post-transform geometry; when `paint.transform` is non-null, `box` is the transformed AABB |
| `boxUntransformed` | object | present only on nodes whose geometry moves under a transform (own or ancestor's): the same four rects re-measured with all transforms neutralized (inspector-origin `transform:none` pass). This is the rect the layout runtime reproduces; the transform reapplies at paint as a render transform. Serialized after `unsupported` (attachment order — deterministic, just not the visual field order) |
| `layout` | object | see below |
| `paint` | object | see below |
| `text` | object\|null | present when the element has direct text content (folded from child text nodes; `<br>` folds to a newline = a FORCED break the consumer honors; whitespace collapses within each segment) |
| `states` | object\|null | per forced pseudo-class (`hover`/`active`/`disabled`): a **diff** of supported computed properties vs the base state, e.g. `{"hover": {"background-color": "rgb(70, 115, 195)"}}` |
| `children` | array | Node[], order-sorted |
| `unsupported` | array | diagnostics — see below |

### `layout`

`display`, `direction` (flex-direction), `justify`, `align` (align-items), `alignSelf`, `alignContent`, `wrap`, `gap` `[column,row]` px, `grow`, `shrink`, `basis` (computed string — `auto` or px), `position`, `inset` (null when static; else computed `top/right/bottom/left` strings — px or `auto` — plus `authored`: the sides the author actually pinned, recovered from matched rules because Chromium resolves all four and erases intent; decides anchors at emission), `zIndex` (int, `auto`→0), `order` (int — informational; children already sorted), `boxSizing`, `overflow` `[x,y]`.

Note: `layout` carries no width/height/margin/padding *values* — they are derived by differencing the four `box` rects (used values beat specified values for fidelity). It does carry `sizingAuthored`: which of `width`/`height`/`min-*`/`max-*`/`basis` the author *declared* (non-`auto`) — the sizing twin of `inset.authored`. Chromium gives an authored size priority over cross-axis stretch; a consumer reproducing layout from inputs needs that priority bit, which used values alone erase. (Added 2026-07-31 during Gate 2 bring-up; the L1 stretch defect is the motivating case. Known gap, recorded: presence only — an authored width *combined with* `flex-grow` on the same axis would need the authored value too; no corpus page exercises that yet.)

### `paint`

- `background`: null | `{type:"color", rgba:[r,g,b,a]}` | `{type:"image", asset, size, position:[x,y], repeat}` | `{type:"gradient", gradientType, angleDeg, stops, computed}` — gradients are typed (Gate 3): `gradientType` `linear|radial|conic|unparsed`, `angleDeg` (linear only; CSS convention, 0 = to top), `stops:[{rgba, posPct|null}]`. Radial gradients additionally carry `center:[cx,cy]` + `radius:[rx,ry]` **absolute px against the painted box** (prelude resolved: shape, size keyword incl. corner/side rules, position); an unparseable prelude omits them — the consumer diagnoses. The verbatim `computed` string is always kept alongside. `size`/`position` on images are computed strings (may contain `%` — background positioning percentages are relative placement semantics, not resolvable to px independently of the painted box; the Gate 3 brush owns that math).
- `borderRadius` `[tl,tr,br,bl]` px — `%` radii resolved against the border box (horizontal basis); an elliptical result (H≠V) is out of v1 surface → diagnosed with `source:"computed"`, horizontal value kept. `borderWidth` `[t,r,b,l]` px · `borderColor` rgba (top edge, kept for uniform-border consumers) · `borderColors` `[t,r,b,l]` rgba per side (Gate 3: per-side widths/colors are in-surface — the consumer bakes bespoke border rings).
- `boxShadow`: null | `{layers, computed}` — typed (Gate 3): `layers:[{rgba, offset:[x,y], blur, spread, inset}]` all px, in CSS order (first layer paints topmost). An unparseable value omits `layers` (consumer diagnoses); the verbatim `computed` is always kept.
- `opacity` 0–1 · `visibility`.
- `transform`: null | `{computed, matrix, origin}` — Chromium computes every transform down to a matrix; 2D becomes typed `matrix:[a,b,c,d,tx,ty]` (CSS matrix order, tx/ty px), `matrix3d` keeps `matrix:null` with the verbatim `computed` (a consumer that cannot type it diagnoses, never guesses). `origin:[x,y]` absolute px within the node. Matrix coefficients round to **6** decimals, not the schema-wide 2 — they are unitless, and 2-decimal rounding denormalizes rotations.

### `text`

`content` (collapsed), `family` (computed stack, verbatim — font *mapping* to UFont is emitter-side config, brief §6), `sizePx`, `weight` (100–900), `style`, `lineHeightPx` (null = `normal`), `letterSpacingPx`, `color` rgba, `align` (computed keyword — may be `start`/`end`; emitter owns direction-resolved mapping), `whiteSpace` (classic keyword, synthesized from Chrome 150's `white-space-collapse` + `text-wrap-mode` longhands — the shorthand left the computed list), `textOverflow`, `transformCase`, `decoration`, `lineBoxes` (array of `[x,y,w,h]` page-px rects, one per rendered line of the folded text, from `Range.getClientRects()` — the §8.1 comparanda; absent when the node was not resolvable).

### `unsupported` (the no-silent-drops contract, brief §1 rule 5)

One entry per author-set property outside the v1 surface (DECISION 4 proposal — allowlist lives in `src/extract.mjs` `SUPPORTED_PROPERTIES`/`SUPPORTED_SHORTHANDS`):

```jsonc
{ "property": "backdrop-filter", "value": "blur(4px)", "source": "<file>:<line>" }
```

Four diagnostic classes share this array:
1. **Name-level** — author-set property outside the allowlist (example above).
2. **Value-level** — supported property, out-of-surface value: `display` outside `flex/inline-flex/block/inline-block/inline/flow-root/none`, `position` outside `static/relative/absolute` (so `display:grid` and `position:sticky` are loud, not silently laid out wrong).
3. **Pseudo-elements** — a matched `::before`/`::after` rule (property `"::before"`, rule's file:line) — pseudo content paints pixels the IR cannot represent.
4. **Computed-divergence** — `source:"computed"`: elliptical border radii, differing per-side border colors. No author line exists; the divergence is between the computed result and the v1 surface.

`source` paths are relativized against the page's directory — committed IRs must be checkout-root independent.

Provenance from `CSS.getMatchedStylesForNode` (`regular`-origin rules only — UA and inspector-injected styles are not author intent). Inline-`<style>` line numbers are rebased onto the document via the stylesheet header's `startLine`. Element-`style=""` attributes report `inline style=`. Custom properties (`--*`) are never diagnosed — they are inputs Chromium already resolved.

## Determinism contract

Same input file + same browser version ⇒ byte-identical JSON (verified: two independent runs of `smoke.html` byte-identical, 2026-07-31, Chrome 150.0.7871.188 — golden PNGs also byte-identical on this machine). Mechanisms: fixed viewport/dpr, `--force-color-profile=srgb --hide-scrollbars --disable-lcd-text --force-device-scale-factor=1`, animation/transition/caret neutralization via inspector-origin stylesheet, `document.fonts.ready` barrier, explicit key order, 2-decimal rounding, no timestamps.
