# Gate 03: capability gallery and reusable tool templates
Status: native scope complete, 2026-09-11. Browser-reference narrow restacking is CTO-deferred and Resource Inspector navigation `+` is excluded pending a future product contract. Remaining campaign work is tracked by the conventional migration and acceptance gates; this gallery no longer remains open solely for those two rows.

## Entry evidence
Published host `8e60f187`; Foundation `8f3daa93`; Debugger `bb5fd18f`; Tests `92587fa3`. Fresh serial real-RHI `Ck.UiAuthoring` passes 151/151 in `Saved/Logs/Test-UiAuthoring-Closeout-R222-RealRHI.log`; the earlier NullRHI attempt is not acceptance evidence. Historical menu reopen remains unexplained.

## Decisions
[G3-D1] Independent FCkCapabilityGalleryModel in CkTests; reuse the existing per-player ResourceInspector host with exclusive mode and explicit owner. No native recreation of gallery layout, new framework globals, or debugger module dependency.
[G3-D2] Installed CapabilityGallery.ui.html/.css, six tabs: layout, forms, data, collections, commands, composition. GalleryCoverage.md is the exhaustive capability/state ledger; six page shells alone do not close it.
[G3-D3] Reuse generic registry controls, templates and dialog slots. Missing controls remain explicit backlog until their shared adapter is implemented. Do not substitute browser-only visuals.

[G3-D4] The large table dataset and fixed three-card repeat demo are independent collections. Dataset controls exercise 0/1/12/1000/10000 rows, filtering and stable-key mutation through the virtualized table; they must not allocate an equivalent number of nonvirtual repeat subtrees. Each independent publication reports failure and updates only its own associated metadata on success.

## Steps and verification
1. Enumerate live schema capabilities and map examples/tests -> verify each declared supported capability has a ledger row, distinct from existing unit-only coverage.
2. Implement gallery model/resources and exclusive host mode -> verify installed resource loading, explicit-owner rejection, public open/close paths and unchanged ResourceInspector lifecycle.
3. Exercise each page through native input/data mutation -> verify model outcomes, enabled/read-only/error states, stable keys, menus/dialogs, accepted/rejected reload and ownership cleanup.
4. Capture representative wide/narrow/scaled pages -> inspect clipping, wrapping, scroll reachability and text. Maintain explicit pending cells; screenshots alone do not prove state behavior.
5. Complete reusable examples/templates and reference parity -> verify concrete examples for every supported capability, including long labels and state variants. Browser-reference policy block remains explicit, never bypassed.

## Exit criteria
Supported native GalleryCoverage rows 1–12 have inspectable production examples or production-consumer evidence and current focused gates; gallery is reachable from the native host; malformed reload preserves accepted UI; focus/capture/user ownership and removal behave correctly; and resource staging is declared. Browser-reference narrow restacking is deferred rather than accepted. Resource Inspector navigation `+` is outside this gate. The gallery gate is closed at its declared native scope; controller, performance, outer-shell/inspector migration and all-debugger teardown remain in the full campaign, while package, keyboard-accessibility and localization gates are deferred.

## Ownership
Terra: model/host. Terra: authored resource pair. Luna: schema/state ledger and static inventory. Lead: decisions, Build.cs staging, integration, final gates and evidence review. No Git publication in this implementation increment unless requested again.
[G3-D5] Nested navigation lab: the model owns selection. Native SCkUiTabs hides a disabled requested panel but does not publish a fallback key. The gallery disable-Details action explicitly selects Overview when Details was active; native reconciliation then handles presentation and owned focus. Arrow/Home/End move header focus; Space activates. Do not attribute model fallback to a native callback.

## Current exit audit (2026-09-11)
The installed gallery, copyable GalleryStarter resource pair, NonUFS staging declarations, public per-player open/close path, exclusive Resource Inspector handoff, explicit Slate-user rejection, and focused owner-context evidence are verified by the current source and archived focused gates. Representative wide, narrow, and scaled native captures establish the stated samples; they do not imply every state at every scale.

Gate 03 native scope is **COMPLETE**. Native GalleryCoverage rows 1–12 have current focused evidence, including GalleryStyledSlider, GalleryContextMenus-R2, and the current-binary `ResourceInspector-Row12-R1` gate:10/10 in42s, one editor start, exit0 and zero relevant diagnostics. Row 12 verifies installed Resource Inspector loading/error/pins/dialog evidence plus exact `NSLOCTEXT` long search-hint and Name-header identity/default restoration, retained selected/query/note state, and separate narrow horizontal access; four fresh current-binary captures were inspected. Nested repeats remain an explicitly unsupported shared-runtime boundary; the historical nested-reopen uncertainty remains unresolved and needs no new gallery lab solely from history. CTO-deferred browser restacking is not claimed, and the excluded Resource Inspector `+` contract is not invented. Remaining controller, performance, full-shell/inspector migration and all-debugger teardown obligations are tracked outside Gate 03.
