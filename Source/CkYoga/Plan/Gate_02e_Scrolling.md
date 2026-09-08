# Gate 02e: authored scrolling and overlays

## Implemented first slice; focused evidence recorded below

`scroll` is now a one-child retained `SScrollBox` with a `vertical|horizontal`
direction, parsed in `CkUiDocument.cpp` and staged in `SCkUiSurface.cpp`. Direction
is part of retained-scroll compatibility; an accepted same-direction reload keeps
the object and offset, and a direction change rejects. Native wheel consumption is
`WhenScrollingPossible` with no special Shift-wheel policy.

- `Source/CkSlateLayout/Private/CkUiWidgetRegistry.cpp`: scroll permits padding/background and requires zero gap.
- `Source/CkSlateLayout/Private/CkUiDocument.cpp`: vertical retains the direct-text restriction; horizontal accepts one authored child.
- `Source/CkSlateLayout/Private/SCkUiSurface.cpp`: scroll stages orientation, clipping and retained offset behavior.

`SCkUiTable` is a retained native island. Its prepared configuration is staged through `FCkUiView::StageNode` and committed through `FCkUiView::Commit`; accepted document reloads must preserve the table instance, selected stable key, and scroll state.

`overlay` is also implemented as an ordered clipped layer container. Its first child
supplies desired size; later layers honor their own alignment/padding. The shared
fixture has passed the focused verification summarized below.

TextureHealth can now author:

```html
<overlay id="inventory-overlay" class="fill">
  <scroll id="inventory-scroll" direction="horizontal" class="fill">
    <table id="inventory" class="inventory-min-width" ... />
  </scroll>
  <text id="empty" visible="table-empty" ... />
</overlay>
```

The consumer supplies distinct empty-world and filter-empty text. `table-empty` must mean zero visible rows, not zero collection rows.

## Subsequent vertical subtree slice

The direct-text vertical restriction is temporary. A later slice must allow arbitrary vertical authored subtrees, including columns, tables, and custom leaves. Before widening it, add constrained-measurement coverage for wrapped text and nested flex children so a vertical scroll child cannot create desired-size feedback or unbounded height. Keep exactly one direct scroll child and preserve clipping.

## Retention, input, and verification

Checkpoint: BuildTest-UiScrollOverlay-R5.log passed32/32 authoring tests; Test-TextureScrollOverlay.log
passed20/20 compatibility tests. See PROGRESS.md for archived logs and the exact limits of this evidence.
The shared fixture proves horizontal table geometry/virtualization, offsets/selection retention,
wheel behavior and overlay visibility/desired-size isolation. Actual Texture empty-state migration,
general vertical subtrees and full visual acceptance remain open.

- Accepted reloads must retain `SScrollBox` and `SCkUiTable` identity when id/kind/compatibility are unchanged. Preserve horizontal and vertical offsets, native table selection, and focus where its leaf survives.
- Rejected reloads must publish no new scroll/overlay/tree state.
- Verify clipping with a child wider than the viewport and native wheel bubbling. The explicit policy is `WhenScrollingPossible`; Shift-wheel has no special handling.
- Run real-window finite-height table tests: 320px and wide host widths, a 720px table minimum, offscreen scroll-to-key, and bounded generated row counts at 1,000 and 10,000 records.
- Test both empty overlays, filter selection clearing once, and a reload preserving table pointer, selection key, and both offsets.

## Arbitrary vertical content: implementation direction after splitter gate

Source inspection confirms why removing the parser restriction alone is insufficient:
SScrollPanel consumes child desired size, while SCkFlexBox::ComputeDesiredSize uses
undefined constraints. A nested text column therefore reports intrinsic unwrapped
height before the scroll viewport supplies its width. The existing vertical direct
STextBlock special case bypasses this mismatch.

Implement a shared measured-content bridge with the retained native scroll control.
Reuse FCkFlexMeasureMetaData: constrain the cross axis and leave the content's scroll
axis undefined, keeping viewport height separate from intrinsic content height.
Prefer feeding the bridge during constrained measurement before arrangement; avoid
making the previous solved content height the next desired viewport height. Native
hosts without a Yoga parent also require an arrangement path supplying viewport
constraints. Account for real scrollbar/content padding and visibility; do not assume
outer width equals content width. Confirm actual engine layout and same-pass behavior
before selecting a scrollbar reservation implementation.

Do not blanket-reject nested tables or same-axis scroll controls. Explicit finite
inner viewports are valid and must retain native virtualization. Trace how authored
height/max-height reach the native control through flex slots and style wrappers.
Static ancestor presence alone is insufficient evidence that an inner viewport is
bounded; test actual arrangement. Decide the unbounded-inner-viewport diagnostic
from that evidence, rather than silently realizing a whole collection.

Extend Test_UiScrollOverlay.cpp using its window/tick/FindTagged/Records helpers:
- Nested column and wrapped text at narrow/wide widths; finite viewport, changing
  intrinsic content height, explicit newlines, clipping and wheel behavior.
- Nested retained search/native control across content/style reload; preserve
  identity, value, focus and scroll offsets; rejection publishes nothing.
- Explicitly bounded nested table and inner scroll at1k/10k rows; real offscreen
  RequestScrollIntoView and bounded live rows, with offsets clamped after shrink.
- First arrangement, scrollbar threshold changes and repeated stable layout must
  not require a stale prior solved height or show desired-size feedback.

Source/API/test investigation is complete; adapter implementation and these gates
are pending. Keep the existing direct-text behavior covered during its migration.
## Vertical subtree checkpoint

The subsequent vertical subtree implementation is now present in SCkUiScrollBox and FCkUiView. The earlier direct-text restriction above describes the historical first slice. Focused final evidence is43 authoring tests and20 Texture tests, with inspected OverflowWrap captures; see PROGRESS.md for exact logs and remaining coverage. The adapter resolves native panel geometry before content arrangement and clamps retained requested offset after native Tick. Authored overflow-wrap:anywhere restores long-identifier wrapping without a special direct-text renderer. Broader composition/performance/platform acceptance remains open.
