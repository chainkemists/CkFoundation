# Gate 2 authoring result

2026-09-07. File authoring is implemented and functionally verified for CTO review.
The campaign remains unmerged. Performance acceptance has one open observation below.

## Delivered

- CkSlateLayout parses XML-compatible HTML-like markup and a strict class-based CSS
  subset into a detached typed document. Unknown syntax, invalid dimensions, unresolved
  tokens, duplicate ids, missing bindings and actions reject the whole candidate.
- FCkUiView stages all named regions before moving native widgets. Texture Health's
  native search, count, inventory, preview and details survive reload; its native
  splitter retains ownership of the pane proportions. Labels, button actions,
  hierarchy, spacing and appearance now live in Resources/UI/TextureHealth.ui.html
  and TextureHealth.ui.css in the CkGameplayDebugger checkout (plugin id CkDebugger).
- The open table polls the file pair every half second. Invalid saves retain the
  last accepted tree and revision and show a local error. Corrected saves recover.
- Public authoring guide: ../CkSlateLayout/AUTHORING.md.

## Final functional evidence

Host: E:/Repos/CkPlugins_Other, Win64 Development, incremental UnrealToolbox build.

| Gate | Result | Host log |
| --- | --- | --- |
| Final build and Ck.UiAuthoring | Build succeeded, 7/7 passed, 0 failed/skipped/contaminated, 35s, editor exit0 | Saved/Logs/BuildTest-UiAuthoring-FocusFinal.log |
| Final Ck.TextureDebugger | 20/20 passed, 0 failed/skipped/contaminated, 42s, editor exit0 | Saved/Logs/Test-TextureLayout-Gate2Final.log |

Final build took13.16s (10.41s compile/link executor). Both preserved fresh editor
logs have zero Error:, ensure, fatal, or AngelScript-warning matches:
UiAuthoring-FocusFinal-Editor.log and TextureLayout-Gate2Final-Editor.log.
Existing startup warnings are not claimed absent.

Tests exercise actual changed flex geometry, file edits and recovery, unchanged-file
suppression, cross-region atomic rejection, native aliases/parent ownership, action
dispatch, lifetime release, exact focused input and updated focus ancestry. Production
Texture Health tests retain selected row identity, filter/highlight, scroll, native
search/list/splitter identity, Clear action and texture GC behavior across reload.

36 actual Slate/RHI PNGs are under
Saved/Automation/TextureDebugger/LayoutCapture/Gate2Final/{Native,Prototype}:
0/1/1000 rows, 1280x720/800x600/640x480 logical viewports, scales1.0/1.5.
Final wide populated and narrow high-DPI captures were opened and reviewed; the
preceding empty-state capture was also reviewed. The synthetic texture preview is
white. The shared high-DPI search-icon capture limitation from Gate1 remains.

## Performance observation still open

Comparison.csv next to the final captures records17/18 passing provisional paired
mean CPU DrawWindow budgets. For1000 rows at1280x720 scale1.0, native0.831ms and
authored1.450ms exceed the1.331ms allowed mean by0.119ms. Authored batches were
1.016/0.887/2.446ms; maximum7.486ms. The preceding capture run passed18/18, but that
does not erase the final miss or establish its cause. No broad speedup or complete
performance-acceptance claim. Investigate with controlled profiling in the performance
gate; keep this observation in campaign tracking. Live row counts remain18/23/29
for the three heights, rather than materializing1000 rows.

## Focus and scope boundaries

Slate's public SetUserFocus returns early for an identical focused leaf and leaves
its old ancestor path. Reload therefore uses normal focus clear/reacquire after
the atomic mount commit, while retaining the old path and view through callbacks.
Text/focus are restored unless a callback deliberately moves focus elsewhere.
Normal focus-loss/received and possible text-commit callbacks can run. This is
explicitly not a callback-free reparenting guarantee.

Authored text/buttons/containers are recreated; native bindings retain identity.
Region roots are sized by their native mount; child-slot sizing applies below them.
Full HTML/CSS, localized authoring extraction, controller navigation, packaged-game
proof and a reusable high-level component catalogue are not delivered by this gate.
The game vertical remains Gate3; no full-feature-complete claim.

## Corrections and repository state

Two compile-only failures preceded runtime: Slate/CString API mismatches and then
a new focus fixture's include path. The first consumer run was17/20 because the
plugin lookup used the folder name instead of CkDebugger; corrected and20/20.
Final source audit caught stale focus ancestry despite a passing leaf-only assertion;
the public ancestor assertion now passes on the corrected implementation.

All four Ck origins fetched again at exit, unchanged and0 ahead/behind:
Foundation9398637a4, GameplayDebugger1824d7d, Tests658ae567, Application7ca8b58.
All remain feature/yoga-slate-layout. No rebase was needed. No commits, pushes,
merges, repository copies or new worktrees. Unrelated generated comment changes,
scratch files and P36500 content remain untouched. Yoga's75 source/license hashes
remain unchanged. Focused diff whitespace checks passed.
