# CkParticles VFX Cookbook

Recipes for recreating an existing Niagara effect as a **code-only** CkParticles behavior — no
Niagara graph is ever opened, no generated asset is ever hand-edited.

A recipe is the durable artifact of one recreation. It records what the source effect *is*
(archaeology), how it maps onto CkParticles/CkUsf (translation), and how a future session verifies
the result. Written well, a recipe lets a fresh session re-derive or re-tune the effect without
re-opening the source asset.

---

## What a recipe is not

- **Not** a place for behavior logic. Logic lives in `Shaders/CkParticles/Behaviors/Behavior_<Name>.ush`
  and its C++ mirror. A recipe that contains the effect's math has put it in the wrong file.
- **Not** a Niagara asset description. It describes *the effect*, so it stays true after the source
  pack is deleted.
- **Not** a promise of fidelity. Sections 12–13 exist precisely so unverified claims stay labeled.

---

## Where the evidence comes from

The archaeology sections must be grounded in the **extracted corpus**, not in a Niagara editor
session. The corpus is produced by `CkAssetExporter`:

```powershell
$env:CK_VFX_CORPUS_EXPORT='1'
./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor `
    --test --test-pattern ExportVfxCorpus --discover-fresh `
    --output=Saved/Logs/BuildTest-Corpus.log --project="<project-root>"
```

It writes `Saved/CkVfxCorpus/`:

| Path | Contents |
|---|---|
| `index.json` | manifest of everything exported (per-asset failures recorded, never skipped) |
| `systems/<PackPath>/<Name>.json` / `.txt` | emitter stacks, Rapid-Iteration constants, override pins, rasterized curves, renderers |
| `materials/<PackPath>/<Name>.json` | blend/shading/params + expression histogram (the "material tricks" list) |
| `textures/<PackPath>/<Name>.png` / `.json` | source texture → PNG (capped 1024) + metadata |
| `meshes/<PackPath>/<Name>.obj` / `.json` | LOD-0 carrier geometry + bounds/UV stats |

`Saved/` is gitignored, so the corpus is **machine-local and regenerable, never committed**. A recipe
must therefore quote the numbers it depends on rather than pointing at a corpus path alone — quoting
is what makes the recipe survive without the corpus.

> **Corpus absent?** That is the normal state on a fresh checkout. Regenerate it with the command
> above before writing archaeology sections. Do **not** substitute the Niagara editor, and do not
> write archaeology from memory or from another recipe.

---

## Recipe file layout

One file per source system: `Cookbook/<SourceSystemName>.md`. Every recipe carries all 14 sections
below, in this order. A section that genuinely does not apply says `N/A — <reason>`; it is never
deleted, because a missing section is indistinguishable from an unresearched one.

Each archaeology claim is tagged so a reader can separate evidence from reasoning:

- **`[corpus]`** — read out of the extracted corpus; cite the file and the JSON key.
- **`[engine]`** — read out of the checked-out engine source; cite `file:line`.
- **`[inferred]`** — reasoned, not confirmed; say what would confirm it.
- **`[visual]`** — established by looking at the running effect; say who looked and when.

---

### 1. Source system and provenance

Full source object path, the pack it ships in, its license posture, and the corpus files used as
evidence. State explicitly that the source asset was **not** opened for editing.

### 2. Visual intent in plain language

Two to five sentences a non-graphics reader could act on. What does a viewer actually see, in what
order, over what duration? This is the section a future tuning session reads first.

### 3. Emitter inventory

Every emitter: name, enabled/disabled, CPU/GPU sim target, renderer type. Disabled emitters are
listed **and marked disabled** — their absence from the recreation is then a recorded decision
rather than an oversight.

### 4. Spawn mode, cadence, lifetime, coordinate space, and bounds

Spawn module(s) and counts, loop behavior and duration, particle lifetime, local vs world space,
and the bounds mode. These four facts decide which CkParticles template the effect routes through;
get them wrong and the cadence is wrong no matter how good the shader is.

### 5. Niagara module and curve facts extracted from the corpus

The Rapid-Iteration constants and rasterized curves that carry the effect's numbers — sizes, colors,
alphas, dynamic-parameter channels — with enough key values (at minimum t=0, the midpoint, and t=1)
that the recreation's curves are reproducible from the recipe alone.

### 6. Renderer type, orientation, size, and sorting

Sprite/mesh/ribbon, alignment and facing mode, size mode and value, sub-UV, sort order, and any
material-binding indirection.

### 7. Material, texture, and mesh dependency graph

The full reference chain from the system down to leaf textures/meshes. Then the audit that matters:
**which of those the recreation actually needs**, and which are unused by the recreated shader.
Unused dependencies are named as unused — that is the record justifying not copying them.

### 8. CkParticles translation

- Behavior ID (and why it is next in the roster)
- Continuous vs burst template — and, if the shared template could not express the cadence, the
  template/configuration change that was made instead of hiding the mismatch
- Seed allocation / layer bands (which `CkParticles_Rand` salts are used, and for what)
- `VisTag` (which renderer draws it)
- Which `FCkParticles_StageOutput` fields the behavior writes: Position, Velocity, Color, Size,
  Scale, Orientation, Rotation, MeshIndex, Dynamic — and which it deliberately leaves at default

### 9. CkUsf translation

- Look name and `.ush` entry point
- `_Parameters` in declaration order (the validator enforces this order against the HLSL signature)
- `ParticleColor` usage
- Dynamic-parameter channel meanings (x/y/z/w), which must match the behavior's `O.Dynamic` writes
- Emissive and opacity behavior, blend mode, shading model

### 10. Copied asset destinations

Source path → plugin-owned destination for every asset copied, plus the reason each was required.
The runtime effect must not reference the source pack; §12 is where that is proven.

### 11. Runtime binding path

How a caller gets this effect: the spawn call, how the behavior resolves its template, and how the
generated CkUsf master reaches the renderer. Binding belongs in centralized metadata — a recipe that
documents a gym-local patch is documenting a bug.

### 12. Exact verification procedure

Split into two parts, because they prove different things:

- **Automated** — the tests that run headless, and precisely what each asserts. Note which lane
  (`-nullrhi` vs `--no-nullrhi`) each requires and why.
- **`[EDITOR-VERIFY]`** — numbered, click-exact steps a human follows, with the comparison criteria
  and the timestamps to compare at. Written so someone who has never seen the effect can execute it.

### 13. Confirmed fidelity differences or intentional deviations

Every known difference, each labeled deliberate or outstanding. An empty section is only honest if
the editor gate in §12 has actually been observed — otherwise it says so.

### 14. Reusable lessons for future effects

What the next recreation should copy or avoid. This is the section that compounds: it is why the
cookbook is worth more than the sum of its recipes.

---

## Recipe index

| Recipe | Source system | Behavior | Status |
|---|---|---|---|
| [NS_Lightning_Range.md](NS_Lightning_Range.md) | Vefects `NS_Lightning_Range` | `LightningRange` (17) | source-verified; visual gate pending |

---

## Authoring order that works

1. **Regenerate the corpus** and read the system `.json` + `.txt` and the material `.json` end to end.
2. **Write sections 1–7 first**, quoting numbers as you read them. Contradictions between sources
   get recorded, not silently resolved.
3. **Decide the template routing** (§4 → §8) before writing any HLSL. Cadence mismatches found after
   the shader is written cost far more.
4. **Author the `.ush` + its exact C++ mirror together**, in one edit. They are one behavior with two
   implementations; letting them drift is the module's top anti-pattern.
5. **Write the CkUsf look** and generate it; fix contract errors before wiring anything downstream.
6. **Bind, add the gym station, extend the tests**, then run both lanes.
7. **Fill in §12–14 last**, from what actually happened — never from what was planned.
