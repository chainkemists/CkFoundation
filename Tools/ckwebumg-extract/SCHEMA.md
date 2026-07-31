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
  "assets": [],                       // images referenced by pages — NOT YET POPULATED (Gate 1 open item)
  "fonts": [],                        // fonts in use — NOT YET POPULATED (Gate 1 open item)
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
| `box` | object | `content`/`padding`/`border`/`margin`, each `{x,y,w,h}` in **absolute page px** (2-decimal), from `DOM.getBoxModel` quads. Quads are post-transform geometry; when `paint.transform` is non-null, `box` is the transformed AABB — consumers needing the untransformed box invert via `paint.transform` (Gate 2 concern, recorded here so it isn't rediscovered) |
| `layout` | object | see below |
| `paint` | object | see below |
| `text` | object\|null | present when the element has direct text content (folded from child text nodes, whitespace-collapsed) |
| `states` | object\|null | per forced pseudo-class (`hover`/`active`/`disabled`): a **diff** of supported computed properties vs the base state, e.g. `{"hover": {"background-color": "rgb(70, 115, 195)"}}` |
| `children` | array | Node[], order-sorted |
| `unsupported` | array | diagnostics — see below |

### `layout`

`display`, `direction` (flex-direction), `justify`, `align` (align-items), `alignSelf`, `wrap`, `gap` `[column,row]` px, `grow`, `shrink`, `basis` (computed string — `auto` or px), `position`, `inset` (null when static; else computed `top/right/bottom/left` strings — px or `auto`), `zIndex` (int, `auto`→0), `order` (int — informational; children already sorted), `boxSizing`, `overflow` `[x,y]`.

### `paint`

- `background`: null | `{type:"color", rgba:[r,g,b,a]}` | `{type:"gradient"|"image", computed:"<verbatim computed background-image>"}` — the `computed` escape is temporary Gate-1 honesty: gradient parsing into typed stops is Gate 3 scope; recording beats dropping.
- `borderRadius` `[tl,tr,br,bl]` px · `borderWidth` `[t,r,b,l]` px · `borderColor` rgba (top edge; per-side colors are out of v1 surface → diagnosed).
- `boxShadow`: null | `{computed:"…"}` (same temporary escape, Gate 3 parses it).
- `opacity` 0–1 · `transform`: null | `{computed, origin}` · `visibility`.

### `text`

`content` (collapsed), `family` (computed stack, verbatim — font *mapping* to UFont is emitter-side config, brief §6), `sizePx`, `weight` (100–900), `style`, `lineHeightPx` (null = `normal`), `letterSpacingPx`, `color` rgba, `align`, `whiteSpace`, `textOverflow`, `transformCase`, `decoration`.

### `unsupported` (the no-silent-drops contract, brief §1 rule 5)

One entry per author-set property outside the v1 surface (DECISION 4 proposal — allowlist lives in `src/extract.mjs` `SUPPORTED_PROPERTIES`/`SUPPORTED_SHORTHANDS`):

```jsonc
{ "property": "backdrop-filter", "value": "blur(4px)", "source": "<file>:<line>" }
```

Provenance from `CSS.getMatchedStylesForNode` (`regular`-origin rules only — UA and inspector-injected styles are not author intent). Inline-`<style>` line numbers are rebased onto the document via the stylesheet header's `startLine`. Element-`style=""` attributes report `inline style=`. Custom properties (`--*`) are never diagnosed — they are inputs Chromium already resolved.

## Determinism contract

Same input file + same browser version ⇒ byte-identical JSON (verified: two independent runs of `smoke.html` byte-identical, 2026-07-31, Chrome 150.0.7871.188 — golden PNGs also byte-identical on this machine). Mechanisms: fixed viewport/dpr, `--force-color-profile=srgb --hide-scrollbars --disable-lcd-text --force-device-scale-factor=1`, animation/transition/caret neutralization via inspector-origin stylesheet, `document.fonts.ready` barrier, explicit key order, 2-decimal rounding, no timestamps.
