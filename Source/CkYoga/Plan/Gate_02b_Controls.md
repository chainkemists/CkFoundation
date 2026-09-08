# Gate 2b: Declarative controls and typed data bindings

## Accepted scope

CTO requested the next layer after observing that Construct_Yoga still creates standard Slate controls. The shared CkSlateLayout authoring runtime will construct searches, dynamic text/counts, images, and scrolling from markup. Texture Health C++ supplies typed attributes/actions plus its specialized virtualized inventory component. Native splitter mounts and reload diagnostics remain host chrome; no browser or JavaScript engine is introduced.

## Design and references

- Extend existing FCkUiDocumentParser strict schema and FCkUiView staged all-region transaction, following its native-port identity and focus handling.
- FDataBindings groups Text, TextChanged, Images and Visibility maps. No type-erased reflection or debugger dependency enters CkSlateLayout.
- Search requires both text getter and edit delegate. Search identity, entered text and focus survive accepted reloads with stable id/kind/binding. Invalid candidates invoke no edit callbacks and publish no partial tree.
- Dynamic text/image/visibility use lazy Slate attributes. Text declarations may be literal or bound. A scroll container has exactly one authored child.
- Standard control construction disappears from Texture Health's native binding setup. Keep its specialized inventory native, including virtualization, stable row keys, context menu, selection and empty-state overlay.
- Search/header/preview/detail geometry moves to the real TextureHealth.ui.html and stylesheet. Existing comparison-only native constructor remains.

## Verification

1. Parser and generic view coverage: type errors, missing binding, malformed structure, live getter updates, editing callbacks, identity/focus retention, failed transaction isolation.
2. Texture Health production UI tests: search/filter/highlight, selection across snapshots/reload, clear action and preview lifetime. Retain held-mouse regression.
3. Incremental Toolbox build, fresh Ck.UiAuthoring and Ck.TextureDebugger gates, real RHI table captures and fresh error/ensure/script-warning scans. Plan two focused test invocations; explain extra runs if failures require them.
4. Review consumer reduction as actual removed standard-control construction, not code merely moved to another feature helper. Authoring documentation must identify remaining native boundary and supported syntax.

## Git boundary

All changes stay on feature/yoga-slate-layout, no publishing or merge into dev. Entry fetch found Foundation 2 commits and GameplayDebugger 3 commits ahead of the prior baseline, all in Insights files disjoint from this campaign. Integrate those updates in place at a coordinated source-edit pause before final build, preserving dirty work. CkTests and CkApplication are current.