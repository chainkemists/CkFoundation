# VALIDATION — acceptance protocol (executed in Phase 6; definition of done)

Routed through `ck-change-control`. Every line gets a verdict in PROGRESS.md: GREEN / RED /
HUMAN-QUEUED. "Done" = all machine lines GREEN + human queue populated with exact steps; the
campaign is not "shipped" until the maintainer clears the human queue and runs `/ck-ship-dev`.

## A. Headless / machine-checkable

| # | Check | Command | Expected |
|---|---|---|---|
| A1 | Fraction exactness unit tests | `--test --test-pattern PixelArtRender --discover-fresh` | all green, incl. the 8×4 viewport/target sweep |
| A2 | Snap math property tests (idempotence, ≤½-texel bound, reconstruction, forward invariance, margin fold) | same lane | all green |
| A3 | Camera ortho AutoTest (compose → ViewInfo ortho fields → layer modifier blend → reset) | `--test --test-pattern Camera --discover-fresh` | green; test present in fresh discovery (zero-match = stale-green, re-check) |
| A4 | Subsystem contract AutoTest (default-off, settings roundtrip, loud precondition failure, reset) | `--test --test-pattern PixelArt --discover-fresh` | green |
| A5 | Look contract test (params positional match, domain, blendable location, scene textures) | `--test --test-pattern Usf` (or the specific name) | green |
| A6 | Look generation validator | `Ck_Usf_GenerateLooks` (editor console, or the save-hook) | zero errors; `M_CkUsf_Look_PixelArt` exists |
| A7 | **Full suite, gate of record** | `--test --no-live` | delta-zero vs the Phase-0 baseline: same counts AND same failing-test NAMES |
| A8 | Zero-residue greps | `rg -n "PixelArt.Spike" Source` ; `rg -in "pixelart" Source/CkCamera` | 0 hits each |
| A9 | Shipping-config compile | `--build` with the shipping config per `/build-test` (if the toolbox lane exists; else record UNRUN) | succeeds |

## B. Three-environment surface (non-negotiable #4)

| # | Check | How |
|---|---|---|
| B1 | C++ | The AutoTests above call the utils/subsystem from C++/AS harness paths |
| B2 | Blueprint | A throwaway BP in the gym level calls `Get_PixelArtSubsystem` → `Apply_Preset` → `Request_SetEnabled`; nodes exist with `[Ck][PixelArt]` display names `[EDITOR-VERIFY]` |
| B3 | AngelScript | `Script/Generated/` contains the camera requests + subsystem surface after regen (grep names into PROGRESS.md); the gym stations themselves drive the subsystem from AS |

## C. Human verification queue (`[EDITOR-VERIFY]` — maintainer, standalone unless noted)

Exact setup for all: gym map, standalone (`-game`), `Request_Apply_RecommendedCVars` via the
gym's setup station, 1440p output, internal 360.

| # | Observation | Pass looks like |
|---|---|---|
| C1 | Auto-pan station, snap ON + comp ON | No pixel creep on static geometry AND smooth (non-stepped) scroll. The two A/B stations prove both failure modes exist when their fix is disabled (comp OFF → stepping; snap OFF → creep) |
| C2 | Same, at a second output res with non-integer texel ratio (e.g. 1080p window at internal 360 → 3.0×, then resize to ~1152p) | Box filter degrades gracefully — no shimmer bands, no blur |
| C3 | Margin | At max pan speed, no edge smear/garbage at any screen border |
| C4 | Outline rubric (Phase 5) | 1-texel silhouettes darkened, creases brightened, no doubled lines, no ground staircase at grazing angle; band-shift mode picks palette-adjacent colors |
| C5 | UMG overlay | Text crisp at native res above chunky scene |
| C6 | Toggle residue | Enable/disable ×10 from the gym: visuals fully restore; `r.ScreenPercentage` and AA cvars back at priors |
| C7 | PIE placard | In PIE at >100% DPI, the gym shows the "preview approximate" placard |
| C8 | Perf table sanity | The three recorded GPU times (OFF / ON@360 / ON@180) are consistent with what the maintainer sees live |
| C9 | Screenshot set for the campaign record | `Shot` captures of C1/C4 states committed to the campaign folder (small PNGs) or paths recorded |

## D. Definition of done (change-control class)

This campaign is a **Class 3+ framework change** (new modules, renderer hook, public API):
- All A + B lines GREEN, C queue handed to maintainer with this document.
- Both module `Claude.md`s + tier rows landed; comment audit done.
- Everything committed on `feature/pixel-art-renderer` (CkFoundation) + the CkTests branch;
  **nothing pushed**; no submodule pointer bumps. Publishing = maintainer's `/ck-ship-dev`.
- PROGRESS.md final entry: commit SHAs per repo, human queue, known limitations
  (= the feature matrix), and the Phase-7 backlog list untouched.
