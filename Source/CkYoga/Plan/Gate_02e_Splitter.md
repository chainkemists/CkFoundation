# Authored splitter and Texture pane migration

Moving status and evidence belong in ../PROGRESS.md.

## Contract

Add a retained `splitter` using native SSplitter. Two or more authored child roots
are panes. Direction is horizontal by default or vertical; an existing retained
ID cannot change direction. Pane IDs define identity across reorder/add/remove.
Child flex-grow supplies the initial coefficient, with the style default zero
interpreted as one. Child min-width/min-height supplies the corresponding native
minimum. Splitter padding/background are supported; gap must remain zero.

Keep coefficient units consistent: preserve each surviving pane's live native
coefficient when its declared weight is unchanged; changed/new declarations use
the raw authored coefficient. A collapsed surviving pane may have zero weight.
Validate the complete prospective sum before publication. Do not normalize only
some panes or silently reset resized panes.

SCkUiSplitter::Prepare validates a detached candidate. Commit updates existing
slots in place for unchanged order. Structural changes rebuild slots and release
only capture owned by this splitter. Duplicate content, self/descendant cycles,
invalid numbers or IDs must reject atomically. FCkUiView stages all descendant
ports and updates before publishing. A retained splitter ancestor does not retain
a removed stateless focused child.

## Consumer

TextureHealth.ui.html becomes one main region containing the authored splitter
and its existing inventory/detail subtrees. The consumer keeps its bootstrap
diagnostic host for an initially invalid file. Context-menu content and common
diagnostic-host authoring remain future work; this slice does not complete the
full campaign.

## Evidence required

- Real native divider drag changes arranged sizes and obeys authored minima.
- Same-shape reload retains native identity, divider values and retained search.
- Reorder/add/remove preserves matching pane values; explicit weight changes work.
- Late invalid binding and invalid parser/API inputs preserve accepted state.
- Fresh authoring and Texture tests, plus wide/narrow rendered Texture captures.
- Freeze edits before incremental UnrealToolbox gates and refresh all CK dev refs
  at the completed boundary. No dev publication.
