# Resource inspector reference

`ResourceInspector.html` is a self-contained browser design reference. Open it
locally in a browser to inspect the intended resource-inspector interactions.
It uses ordinary browser HTML, CSS and JavaScript, not the native `.ui.html`
language. There are no external assets, packages or network requests.

The final native test app must load its own authored document through the actual
CkSlateLayout parser, registry and FCkUiView. Translate the visual and interaction
requirements into supported declarations; do not embed this page, introduce a
second renderer, or recreate the whole interface in bespoke Slate.

## Manual reference checks

1. Select different resource rows and verify the details follow the selection.
2. Filter to one resource and then no resources. Clear the filter and confirm that
   no unrelated row replaces the selected key.
3. Sort name and memory in both directions; selection stays with the resource.
4. Expand/collapse navigation groups and filter by category.
5. Switch Overview, Properties and Native graph tabs. Stage a local property edit
   and verify it affects only that mock resource. Invalid LOD bias is rejected.
6. Change dataset size and theme. Empty data must clear resource-specific content.
7. Resize the details pane and inspect narrow/wide windows and text wrapping.

`ResourceInspector.scenarios.json` records cross-renderer acceptance requirements.
It includes native-only reload, registry, lifecycle and packaged-game scenarios
that this browser reference cannot prove. Scenario presence is not test evidence.

## Translation targets

| Browser reference | Production pipeline requirement |
| --- | --- |
| CSS grid shell / pane resizing | Authored splitters, constraints and retained pane ratios |
| HTML table / generated rows | Shared virtualized table, typed records and cell templates |
| DOM event handlers | Typed value bindings and weak owner-bound actions |
| Nested navigation lists | Shared tree adapter with stable keys and retained expansion |
| Form / tabs | Retained generic controls, validation and tab selection |
| CSS themes / status chips | Shared tokens, state styles and reusable components |
| SVG graph | Registered native graph widget with typed properties |
| Browser resources array | Shared deterministic test model, isolated from debugger collection |

Current evidence and unresolved verification live in ../PROGRESS.md. In particular,
browser automation of the local file was rejected by the browser URL policy on
2026-09-07; no alternate route was attempted and no visual pass is claimed.
