# Gate 1: Texture Health native Slate prototype

Written: 2026-09-07. Approved by the CTO for implementation and delivery. Moving status: [../PROGRESS.md](../PROGRESS.md).

## Goal

The Texture Debugger's real Texture Health surface can be laid out with reusable Yoga-backed Slate primitives, with visibly coherent spacing and resizing and unchanged native interactions.

## Entry

- Gate 0 cross-module test passes on the current rebased source.
- CTO accepts this prototype and its bounded scope.
- Record the original surface at 1280x720, 800x600, and 640x480 Slate units at application scale 1.0 and 1.5. Record action behavior and existing test results before edits.
- Check current source and tests; retain a native-layout comparison path during the experiment.

## Work items

1. **NEW INFRASTRUCTURE:** propose and implement a runtime `CkSlateLayout` panel with typed slots for row/column, gap, padding, flex grow/shrink, min/max size, and alignment. Root available size is explicit; selected subtrees use Yoga; native leaves remain SWidgets. Fix config policy (web defaults, point-scale handling, direction) before node construction.
   - Verify exact rectangles with finite available sizes, repeated resize, collapsed versus hidden children, insert/remove/reorder, and invalid input rejecting the whole update without callbacks or mutation.
2. **NEW INFRASTRUCTURE:** define constrained measurement for text and opaque native widget leaves, plus dirty propagation. Prove width-dependent text measurement in isolation using the actual Slate shaping/wrapping path. Define native scrolling boundaries and avoid a cyclic desired-size feedback loop.
   - Verify long localized strings at multiple widths settle without frame-to-frame oscillation; changing text, font, scale, and visibility invalidates the necessary nodes.
3. Convert the Texture Health search/count lane and detail card layout; preserve the existing `SSplitter`, `SListView`, `SHeaderRow`, scroll boxes, native input widgets, selection handling, and preview ownership. Retain the existing split ratio/drag semantics. Define narrow-window behavior before changing containers.
   - Extend `CkTextureDebugger_Ux.spec.cpp` via `Set_Snapshot`, retaining actual row models. Verify filter/highlight, clear, selection retention, row context commands, and value-only updates.
4. Add a developer preview with 0, 1, and 1000 rows and deliberately long details. Then exercise the real collected Texture Health data. Compare screenshots and interactions against the original at the entry sizes/scales.
   - Verify live row/widget count stays viewport-bounded, selection and scroll persist across value updates, tab close and PIE end release preview resources, and reopening remains usable.

## Scope boundaries

Do not rewrite snapshot collection, change texture diagnostics, move feature data into foundation, replace native tables with individual Yoga rows, replace native input or text rendering, add a markup parser, or implement CSS Grid. A synthetic preview is a measurement fixture; the real debugger is the acceptance surface.

## Acceptance

- No unintended overlap or inaccessible controls at the agreed sizes. Deliberate clipping/scrolling has visible usable affordances.
- Text wraps using actual Slate measurement and remains readable at both scales.
- Native splitter and header drag, search, clear, focus, context menus, selection, and scroll behavior remain intact.
- An unchanged snapshot causes no widget-tree reconstruction; value changes preserve stable row identity. Removed rows release their state.
- Focused automated regression checks pass through UnrealToolbox; visual and input observations are recorded separately as editor verification.
- Capture layout/paint timing and allocation evidence on identical data and dimensions for old/new paths. Agree a budget from the baseline before accepting the new path; do not substitute vendor benchmark claims.
- Record lines changed and iteration steps for one comparable visual adjustment. The prototype must demonstrate reduced authoring effort as well as correctness.

## Failure branches

If text measurement does not converge, stop conversion and resolve measurement. If native list virtualization breaks, return ownership of row layout to the native table and keep Yoga outside that boundary. If the authoring code remains as cumbersome as the original, revise the primitive API before extending to other debuggers.

## Manual verification instructions (for the implementation phase)

Open the Texture Debugger through the existing Debugger Launcher, select Texture Health, load a representative textured scene, select a row, resize the window and divider, type filter/highlight queries, scroll the list/details, invoke the row context menu, and clear selection. Repeat after PIE end/restart and tab close/reopen. Capture original/prototype screenshots at the agreed sizes and scales. No such UI observations are claimed at Gate 0.

## Prototype measurement budget

Before paired measurements, use this provisional guard: prototype mean CPU DrawWindow submission time must remain within native mean + max(0.5 ms, 25 percent of native mean) for each identical viewport/data case. Record three batches, mean and maximum separately; GPU completion and PNG encoding are outside this CPU metric. This is a host-specific prototype budget, not a portable frame-time guarantee. Yoga node allocation counters must remain unchanged through steady resize/visibility/layout passes; native live rows must stay viewport-bounded for 1000 rows. Retained widget/node counts are not byte-allocation measurements.
