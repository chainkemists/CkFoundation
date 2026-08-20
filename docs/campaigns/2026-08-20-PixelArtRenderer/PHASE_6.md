# PHASE 6 — Gym, full test pass, docs, perf measurement, close-out

> Entry: Phases 0–5 done. Load `ck-tests-authoring-and-running` (gym framework) and
> `ck-performance-and-analysis` (for the measurement — no perf claims without numbers).
> The acceptance protocol is [VALIDATION.md](VALIDATION.md) — this phase EXECUTES it.

## Steps

1. **PixelArt gym** in CkTests (`Script/CkUsf/`-adjacent home: `Script/CkPixelArt/` — follow the
   Stylize gyms' registration shape, `CkUsfStylize*Gym_*.as`, and the gym creation spec):
   - Reference scene: engine basic shapes — ground plane, cube stack, sphere, 30° ramp, one
     thin-feature object (railing-like) — plus a directional light with VSM configured. No
     content-heavy assets.
   - Stations/keys: toggle renderer, cycle internal height {180, 360, 540}, toggle snap, toggle
     sub-texel comp (the stutter A/B), cycle presets, toggle look, auto-pan camera at
     0.2 texel/frame (the creep verdict station), UMG test overlay (native-res UI criterion).
   - The gym prints a placard when it detects PIE + non-100 DPI-derived secondary fraction
     ("PIE preview approximate — verdicts in standalone", per PROMPT risk 1).
2. **Full suite gate of record**: `--test --no-live` (fresh boot). Diff counts AND names vs the
   Phase-0 baseline. Any new red: isolate own-change vs pre-existing (A/B stash if needed)
   before touching anything.
3. **Perf measurement** (success criterion 8): in the gym scene, standalone, 1440p output —
   `stat unit` / `stat gpu` (or Insights per `ck-performance-and-analysis`) for: renderer OFF
   (native), renderer ON at 360 internal, renderer ON at 180. Record the three GPU frame times
   in PROGRESS.md. Expected: ON ≤ OFF (the scene renders at ~6% of the pixels; the upscale pass
   is one fullscreen PS). If ON > OFF → STOP, profile before claiming anything.
4. **Docs** (the campaign's permanent residue):
   - `Source/CkPixelArtRender/Claude.md` + `Source/CkPixelArt/Claude.md` — purpose, key API,
     the supported-feature matrix (AA None/FXAA only; VSM-not-CSM; SSAO off; no light functions
     (status per Phase 0 7d); no split-screen/captures/stereo; PIE preview approximate; renderer-
     private include = engine-bump cost), anti-patterns (the Fences from these phase docs).
   - `Source/CLAUDE.md` — tier-table rows for both modules (+ CkUsf row note if deps changed).
   - Fix the two stale docs found in research (RESEARCH_Codebase §2): the `CkEcs/CLAUDE.md`
     processor-group roster (add Transform_Derived/LateResolve) and the `CkGraphics/Claude.md`
     "render target management" claim.
   - `Source/CkUsf/Claude.md` — one line under the Stylize section pointing at the PixelArt
     look + the scoped-D1 note (the SVE ruling stands for Stylize-class effects; the pixel-art
     renderer is the sanctioned exception, link this campaign).
5. **Comment audit** (mandatory closing step, root CLAUDE.md): re-read the full campaign diff;
   delete every what-comment and campaign/phase breadcrumb.
6. **Execute [VALIDATION.md](VALIDATION.md)** — record every line's verdict in PROGRESS.md.
   The `[EDITOR-VERIFY]` items go into PROGRESS.md's "Human verification queue" with exact
   steps; they are the maintainer's, not yours.
7. Final commits. **Do not push anything** — end-of-campaign publishing is the maintainer's
   `/ck-ship-dev` decision.

## Exit criteria

- VALIDATION.md executed; every machine-checkable line green; human queue populated with exact
  repro steps and expected observations.
- Full suite delta-zero vs Phase-0 baseline (names).
- Perf table recorded (three real numbers, no estimates).
- Docs landed; comment audit done; PROGRESS.md final state written (what's committed where,
  what remains human-gated).

## Fences

- The perf claim ships as measured numbers from THIS machine + scene, never extrapolated.
- Gym runs its lane with `--no-nullrhi` if any station needs real rendering; keep stations
  skip-with-trace under nullrhi (harness escalates Warnings).
- No BB-side (superproject) edits in this campaign — CkFoundation + CkTests only. BB adoption
  is a follow-up.
