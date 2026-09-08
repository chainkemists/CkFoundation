# Texture Health vertical prototype

Delivered implementation for review, 2026-09-07. This is Gate 1 of the campaign, not completion of the full web-like authoring feature. No commits, pushes, or merges were made.

## What is available

Open the existing debugger with `ck.TextureDebugger 1`, then select **Texture Health**. Its default layout now uses the runtime `CkSlateLayout` module. The 64/36 splitter, six-column virtualized native list, header, search/highlight controls, selection, Clear, context command, and texture preview ownership remain native Slate.

- `SCkFlexBox`: row/column, container gap/padding, slot grow/shrink/min/max/alignment, retained Yoga nodes, dynamic insertion/removal, explicit input rejection, and guarded structural callbacks.
- `SCkFlexText`: actual Slate text shaping under a supplied width; automatic constrained measurement through weak metadata when placed directly in a flex panel. Unset font arguments preserve the supplied Slate style.
- Narrow tables scroll horizontally below a 720-unit desired content width. The wide view fits all six columns. Empty selection no longer reserves the preview image's height. Long selected facts wrap within the native details scroll box.
- `.UseYogaLayout(false)` retains the original container path for paired comparison. Collection and row reconciliation are shared, not duplicated.

## Verified evidence

Final Toolbox invocation: Win64 Development incremental build succeeded (14.85 seconds; 7.43 seconds compilation/link execution), **20/20 tests passed**, zero failed/skipped/contaminated, 34 seconds test duration, editor exit 0. Fresh editor log has no Unreal error, ensure, fatal, or AngelScript error/warning matches. Existing startup warnings and Chromium USB diagnostics remain.

Tests cover exact rectangles, resize/DPI, collapsed/hidden children, insertion/removal and stable child identity, live padding updates, constrained/nested text and font changes, invalid declarations/measure inputs, rejected reentrant mutation, native list selection/filter/highlight/Clear, snapshot pointer/scroll retention, and preview GC/release/reopening. These are focused tests, not a repository-wide regression claim or physical mouse/keyboard verification.

Actual Slate/RHI captures: 0/1/1000 rows, 1280x720 / 800x600 / 640x480 logical units, scales 1.0 / 1.5, both Native and Prototype: **36 PNGs**. The selected fixture deliberately has long names and facts. Its white preview is a transient test texture, not representative scene artwork.

Host evidence:

- `Saved/Logs/BuildTest-TextureLayout-Compact.log`
- `Saved/Logs/TextureLayout-Compact-Editor.log`
- `Saved/Automation/TextureDebugger/LayoutCapture/Compact/Native/`
- `Saved/Automation/TextureDebugger/LayoutCapture/Compact/Prototype/`
- `Saved/Automation/TextureDebugger/LayoutCapture/Compact/Comparison.csv`

## Timing and resource observations

Each capture records three batches of 30 timed `DrawWindow` submissions after warm-up. GPU completion/readback and PNG saving are outside the CPU draw statistic. All 18 paired means satisfy the budget declared before measurement: native mean + max(0.5 ms, 25 percent of native mean).

For 1000 rows, prototype means range from approximately 0.50 to 0.87 ms; 18/23/29 native row widgets are retained for the three viewport heights. Maximum samples are recorded separately in the CSV; ambient spikes make a general speedup claim inappropriate. This is one host/editor run with three sampling batches, not three independent cold benchmark runs.

The panel allocation regression proves resize/visibility/DPI/layout passes do not create additional Yoga nodes. Widget/node counts do not constitute allocator-byte profiling.

## Authoring assessment and remaining acceptance

The first slot-based adapter still required about 169 constructor lines versus 165 for the retained native path, so it did not demonstrate easier authoring. The final prototype therefore adds compact `ck::slate::Row`, `Column`, `Content`, and `Fill` composition. Its four principal layout containers now occupy four declarations, while native widget construction remains separate. The prototype constructor is about 99 lines including narrow scrolling and constrained empty text. This source-size comparison includes formatting and extraction improvements; it is not a measured model-generation-time claim.

For the actual detail header, the composition is `Row(CkStyle::SpaceS, {Fill(Heading), Content(Clear)})`. Adjusting spacing edits one container value; appending a sibling adds one item descriptor without another slot/bracket block or per-edge padding. Typed slots remain available for advanced constraints. A focused test confirms compact construction allocates the complete three-node example once and preserves the exact typed-slot rectangle. Live editable declarations remain Gate 2; this is a smaller native C++ authoring surface, not the finished web-like workflow.

The inspected captures demonstrate the bounded layout changes. At scale 1.5, the offscreen captures show square search glyphs in both native and prototype paths; native capture also loses some heading glyphs. Treat this shared capture/render limitation separately from layout, and inspect the live editor before claiming complete visual acceptance at that scale.

CTO live review remains appropriate for splitter/header dragging, keyboard focus, context menu interaction, representative collected scene textures, and PIE end/restart. The automated fixture exercises `Set_Snapshot` and native callbacks; it is not evidence that a human exercised those physical input paths. Packaged game, controller navigation, RTL, Mac/Linux, Blueprint/AngelScript authoring, and live reload remain later campaign work.

All four Ck plugin branches were fetched again after the green gate. Each remains 0 ahead / 0 behind its current `origin/dev` baseline. Campaign source remains uncommitted on `feature/yoga-slate-layout`; unrelated dirty work is preserved.
