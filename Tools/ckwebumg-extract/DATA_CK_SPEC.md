# `data-ck-*` — authoring escape hatch (spec v1)

> **Written:** 2026-07-31 (Gate 1). Ratified at the Gate 1 schema review; extended only by appending, never by repurposing.
> Purpose (brief §7): a pure CSS→UMG dump with machine names is useless to gameplay code. These attributes carry designer/developer intent through extraction into the IR's `ck` block and, at Gate 4, into the emitted widget tree.

| Attribute | IR field | Emitter contract (Gate 4) | Extractor status |
|---|---|---|---|
| `data-ck-name="HealthBar"` | `ck.name` | Widget variable name; `IsVariable = true`. Must be unique per page — extractor will diagnose duplicates (open item) | ✅ extracted |
| `data-ck-widget="UCkProgressBar"` | `ck.widgetClass` | Force the named C++/BP widget class instead of the inferred mapping | ✅ extracted (passthrough string; class validation is emitter-side) |
| `data-ck-bind="Health"` | `ck.bind` | Emit a `BindWidget`/property-binding hook named `Health` | ✅ extracted |
| `data-ck-slot="Content"` | `ck.slot` | Named insertion point for runtime-injected children | ✅ extracted |
| `data-ck-ignore` | — | Subtree never appears in IR or output (mockup scaffolding, annotations) | ✅ honored (verified: smoke.html scaffold-note absent from IR) |

Rules:
- Attributes are inert in the browser — mockups stay previewable with zero tooling.
- Unknown `data-ck-*` attributes are a **diagnostic** (not silence, not an error) — misspellings must not vanish. (Extractor open item, lands with the corpus hostile page.)
- `ck` is `null` when no attributes are present — absence is distinguishable from empty.
