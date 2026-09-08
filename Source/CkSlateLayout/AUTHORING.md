# Native UI authoring

Gate 2 introduces external structure and styles on top of Yoga and native Slate.
This is a deliberately small XML-compatible HTML-like language and CSS subset.
It does not load browser content or execute scripts.

## Edit Texture Health

Open Texture Debugger with `ck.TextureDebugger 1`, then its Texture Health surface.
Edit the installed CkGameplayDebugger plugin's files:

- `Resources/UI/TextureHealth.ui.html`: hierarchy, labels, native binding placement, actions.
- `Resources/UI/TextureHealth.ui.css`: class-based layout and appearance.

Save either file. The open table polls the pair every half second. Successful edits
apply without compiling C++; rejected edits display a diagnostic above the table
and leave the last accepted interface usable. Fix and save to recover. An initial
load failure displays the diagnostic without publishing a partially built layout.

For a visible first edit, change `.search-row { gap: var(--space-s); }` to
`.search-row { gap: 16px; }`, or change `Selected texture` in the markup. Select a
texture and type in the search controls before saving to observe state retention.

## Markup

```html
<ui version="1">
  <region name="main">
    <column id="panel" class="panel">
      <row id="header" class="header">
        <text id="title" class="fill heading">Inventory</text>
        <button id="refresh" action="refresh">Refresh</button>
      </row>
      <native id="inventory" bind="inventory" class="fill" />
    </column>
  </region>
</ui>
```

Each region contains exactly one root node and corresponds to a mount registered
by native code. Every node has a unique nonempty id. Containers are `row` and
`column`; leaves are `text`, `button`, and `native`. A button names a registered
action; a native leaf names an existing Slate widget. Every registered native
binding must appear exactly once. Arbitrary widget classes, expressions, DOM APIs,
inline styles, and script attributes are unsupported.

## Styles

### Resizable panes

`splitter` lays out two or more child roots as resizable panes. Its `direction`
is `horizontal` by default or `vertical`. Each child's `flex-grow` is its initial
weight (the default zero means one here); its `min-width` or `min-height` is the
minimum along the splitter axis. Splitter padding and background are supported;
gap must be zero.

Reload preserves divider coefficients by child ID when the declared weights stay
unchanged, including when panes move or new panes are added. An explicit weight
change resets that pane to the new coefficient. A retained splitter ID cannot
change direction; use a new ID for a replacement. Structural changes end any
active drag owned by that splitter. Splitters are not permitted in table cells.

```html
<splitter id="panes" direction="horizontal">
  <column id="inventory" class="inventory-pane"><text id="inventory-title">Inventory</text></column>
  <column id="detail" class="detail-pane"><text id="detail-title">Details</text></column>
</splitter>
```

```css
.inventory-pane { flex-grow: 0.64; min-width: 160px; }
.detail-pane { flex-grow: 0.36; min-width: 120px; }
```

The C++ view exposes `GetSplitter(id)` for integrations that need the retained
adapter; its `GetSplitter()` exposes the native control. Author pane layout in
the resource files rather than changing native slot membership externally.

### Style values

```css
.panel { gap: var(--space-m); padding: 8px 12px; }
.header { gap: 4px; }
.fill { flex-grow: 1; flex-shrink: 1; }
.heading { font-size: 18; font-weight: bold; color: #E8EDF5; }
```

Only individual `.class` selectors are supported. Multiple classes may be listed
on a node. Rules apply in stylesheet order, including repeated class rules; later
declarations win. This subset has no inheritance, descendant selectors, media queries,
pseudo states, or CSS custom property declarations. `var(--name)` resolves a token
supplied by the native consumer; unresolved or cyclic tokens reject the update.
Padding components may reference tokens independently, for example
`padding: 0 var(--space-s)`. A whole padding token may also contain literal
shorthand lengths. Padding expansion is limited to four source components and
1 MiB of resolved text; unsupported or malformed values reject the document.

| Properties | Supported values |
| --- | --- |
| `gap` | Nonnegative number, optional `px`; row/column/repeat, between items and wrapped lines |
| `flex-wrap` | Row/column/repeat: `nowrap` (default), `wrap`, or `wrap-reverse`; wraps along the main axis at the allocated width/height |
| `padding` | One, two, or four nonnegative lengths in CSS top/right/bottom/left order; containers only |
| `flex-grow`, `flex-shrink` | Nonnegative unitless numbers; both default to zero |
| `width`, `height` | Exact nonnegative lengths, represented as equal minimum/maximum constraints |
| `min-width`, `min-height`, `max-width`, `max-height` | Nonnegative lengths; maximum must be at least minimum |
| `horizontal-align` | `fill`, `left`, `center`, `right` |
| `vertical-align` | `fill`, `top`, `center`, `bottom` |
| `font-size`, `font-weight` | Text/button only; font size 1–512 in Slate units, `normal` or `bold` |
| `text-wrap` | Text and ordinary buttons; `wrap` (default) or `nowrap`. Controls soft wrapping; preserves explicit line breaks |
| `overflow-wrap` | Text and ordinary buttons; `normal` (default) or `anywhere`. Anywhere permits character wrapping for long unbroken identifiers; `text-wrap: nowrap` still disables wrapping |
| `text-overflow` | Text and ordinary buttons; `clip` (default) or `ellipsis`. Ellipsis requires `text-wrap: nowrap`; nowrap text clips to its allocated bounds |
| `color` | Text/button only; `#RRGGBB` or `#RRGGBBAA` in sRGB |
| `background-color` | Container only; same color syntax |

Alignment, growth, and sizing describe a child node in its parent. Region roots
fill their native mounts; putting these slot properties on a region root is rejected.
Put a child container inside the region root when it needs size constraints.
Text uses constrained Slate
measurement. Native controls keep their own internal scrolling, clipping, input,
and virtualization. Appearance inside a native binding remains its responsibility.

## Native integration and ownership

`FCkUiView::Create` accepts named native widgets, actions, tokens, a base font, and optional typed `FDataBindings`.
Install `GetRegion(name)` mounts in the owner, then call `ReloadFiles` or `TryReload`.
The owner schedules `PollFiles` on the game thread; the runtime module has no editor
file-watcher dependency. Texture Health stages the source files as loose runtime
dependencies, although packaged-game execution is a later campaign gate.

The view parses, resolves, validates, and stages all regions before attaching any
existing native widget. It detaches old ports and mounts the candidate in one
synchronous commit. An error preserves the accepted revision and tree. Use weak
owner-bound delegates for actions; callbacks must not strongly capture their owner.

Native widgets retain identity across valid edits: editable contents, list selection,
scroll position, and consumer-owned preview lifetime survive. For a focused native input,
reload restores focus using normal Slate focus loss/reacquisition so the complete
ancestor path is refreshed. Native focus-loss and text-commit callbacks can run;
do not treat reloading an active form as a callback-free operation. If those callbacks
move focus elsewhere, the view respects that choice. Native id/binding
changes are checked to prevent accidental state reassignment. The native splitter
outside the authored regions also survives. Authored containers, labels, and buttons
are recreated on reload; retained authored control state, localization extraction,
controller navigation, and packaged-game proof are outside this gate.

## Declarative controls and data

```html
<search id="filter" bind="filter" placeholder="Filter textures..." />
<text id="count" bind="row-count" />
<image id="preview" bind="selected-image" visible="has-selection" class="preview" />
<scroll id="details-scroll" class="fill">
  <text id="details" bind="selected-details" />
</scroll>
```

The consumer registers values and behavior, not standard Slate widgets:

- `FDataBindings::Text`: named `TAttribute<FText>` values, used by text and search.
- `TextChanged`: named `FOnTextChanged` delegates. A search requires this and a Text binding under the same name.
- `Images`: named `TAttribute<const FSlateBrush*>` values. The owner must retain the brush and its resource for as long as it can be painted; Texture Health keeps its existing selected-texture strong reference.
- `Visibility`: named `TAttribute<bool>` values; `visible="name"` collapses the node when false.

### Scroll and overlay

`scroll` has exactly one child and takes `direction="vertical"` (default) or
`direction="horizontal"`. Both directions accept one authored subtree. Vertical
content is measured at the native scroll panel's actual width, so nested columns
and text wrap without a special direct-text path. Give a nested virtualized table
its own finite `height` or `max-height` to define its viewport; for example,
`.results { height: 180px; }` on the table itself. Put viewport sizing on a child of
the region root, since the native mount sizes the region root.

Scroll id and direction are retained compatibility: a valid reload
with the same pair keeps the native `SScrollBox` and its offset; changing direction
rejects the candidate. Scroll uses Slate's `WhenScrollingPossible` wheel bubbling and
does not define a special Shift-wheel policy. Vertical content shrink also clamps
the retained requested offset so regrowth cannot restore an obsolete position.

`overlay` paints its children in source order. Its first child supplies desired size;
later children are clipped to the overlay bounds and use their authored alignment. Use a
collapsed later layer for an empty state.

```html
<overlay id="inventory-overlay" class="fill">
  <scroll id="inventory-scroll" direction="horizontal" class="fill">
    <table id="inventory" bind="records" class="inventory-min-width">
      <table-column id="name" label="Name"><text id="cell" bind-field="name"/></table-column>
    </table>
  </scroll>
  <column id="empty" visible="table-empty" class="empty-centered">
    <text id="empty-copy" bind="empty-copy"/>
  </column>
</overlay>
```

Set `min-width: 720px` on `inventory-min-width` when a table needs horizontal
overflow, and `horizontal-align: center; vertical-align: center;` on `empty-centered`.
The overlay does not make the empty layer affect table measurement.

A bound text node cannot also contain literal text. Binding names are resolved by the control's required type. Missing or incorrectly typed bindings reject the complete document before controls are changed. Read attributes update with the model without a document reload; edit delegates write through the consumer's existing business logic. Use weak owner-bound getters and delegates and keep reads side-effect-free.

A surviving search id must keep its kind and binding. A valid document may omit a search: commit detaches it and releases the view-owned instance. Adding it again creates a new control from the current bound model. Saving a stylesheet retains a surviving search and does not reset its text. Search placeholders can change on an accepted reload. Normal focus-loss/commit callbacks still apply when the focus path is rebuilt, as described above.

Texture Health now authors its inventory table columns/cells, horizontal scroll, and empty overlay in the external files. Its consumer supplies the typed collection, stable domain keys, selection mapping, colors, tooltips, meter/status fields, and context-menu callback. Its resizable inventory/detail panes are authored through the shared splitter. The bootstrap layout-error chrome remains native host ownership; table context-menu content is still built by the consumer callback. General authored menus remain later work. The splitter migration passed 39 authoring tests and 20 Texture tests with inspected wide/narrow captures; full campaign acceptance remains open.

## Registered custom widgets

Trusted C++ consumers can register custom leaf widgets through the public
`CkSlateLayout/CkUiWidgetRegistry.h`. A view receives an immutable snapshot;
later registrations do not change the types accepted by an existing view.

For example, a module can register a resource title once:

```cpp
auto Registry = FCkUiWidgetRegistry{};
auto Title = FCkUiCustomWidgetRegistration{};
Title.Schema.Tag = TEXT("resource-title");
Title.Schema.Properties = {
    {TEXT("text"), ECkUiCustomPropertyKind::TextBinding, true},
};
Title.Factory = [](const FCkUiCustomWidgetArguments& Args, FString&) -> TSharedPtr<SWidget>
{
    return SNew(STextBlock).Text(Args.TextBindings.FindRef(TEXT("text")));
};
const FCkUiLoadResult Registered = Registry.Register(MoveTemp(Title));
// Handle Registered.Errors if registration failed before constructing the view.
```

Pass `Registry.CreateSnapshot()` as the final argument to `FCkUiView::Create`,
after `FDataBindings`. Supply `resource-name` in `FDataBindings::Text`, then author:

```html
<resource-title id="selection-title" text-bind="resource-name" />
```

No parser or renderer changes are needed for this registration. The executable
cross-module example and rejection tests live in CkTests' `Test_UiRegistry.cpp`.

| Schema kind | Authored attribute for property `value` | Factory arguments |
| --- | --- | --- |
| Text | `value="Selected resource"` | `TextProperties` |
| Number | `value="12.5"` (finite float, no units) | `NumberProperties` |
| Bool | `value="true"` or `value="false"` | `BoolProperties` |
| Color | `value="#112233"` or eight hex digits with alpha | `ColorProperties` |
| TextBinding | `value-bind="name"` | `TextBindings`, from data Text |
| ImageBinding | `value-bind="name"` | `ImageBindings`, from data Images |
| NumberBinding | `value-bind="name"` | `NumberBindings`, from data Number |
| BoolBinding | `value-bind="name"` | `BoolBindings`, from data Visibility |
| Action | `value="action-name"` | `Actions`, from registered actions |

Property names address typed maps in `FCkUiCustomWidgetArguments`. Omitted optional
properties have no entry; they do not silently receive defaults. Bound values stay
live without reparsing the document. The provider owns value validity and resource
lifetime; numeric bindings do not validate future getter results for a custom widget.
Image providers retain their brushes/resources for every widget that can read them.

Registration rejects duplicate or malformed schemas, invalid kinds, reserved tags,
and generated attribute collisions (such as `label-bind` from two properties).
Tags are lowercase identifiers; `ui`, `region` and built-in tags are reserved.
The limits are 512 registered types and 64 properties per type. Each registration
is atomic. Check its result and do not publish a consumer's incomplete required set.

All nodes, properties, and required bindings in a candidate document are validated
before any factory runs. A factory returns a new detached widget; null or a nonempty
failure diagnostic rejects the entire candidate. The runtime checks output hierarchy
identity against mounted/candidate widgets before wrapping and attaching it. The
wrapper owns authored sizing, alignment and visibility; the returned widget keeps its
own visibility and other attributes. Common slot sizing is supported, but custom-leaf
CSS text/background/padding styles are not implicitly applied to its internal content.

Factories are trusted native code, not sandboxed callbacks. They must not mutate
mounted widgets, change focus, execute actions, or publish model/timer side effects
during construction. Returning a previously owned widget violates this contract;
validation cannot undo arbitrary side effects performed inside a callback. Capture
owners weakly and avoid view/registry/owner reference cycles. Snapshot copies preserve
schema values, not the mutability of external state deliberately captured by a factory.

Stateless `Factory` outputs are recreated on valid reload. Use the retained API
below to preserve a custom control's state. Reload clears focus from a replaced
authored control even if another owner holds its old widget; unrelated focus and
focus-callback redirects are respected. Focus restoration snapshots actual and virtual Slate users independently; broader
multi-user navigation remains a game acceptance item. Custom child slots remain later campaign work. The current `<native>`
port is not their final authoring API.

## Stateful custom widgets

A registration may supply `RetainedFactory` instead of `Factory`. Exactly one is
required. A retained factory returns an `ICkUiRetainedWidget`: its `GetWidget()`
exposes the permanent Slate root, and `PrepareReload()` returns a prepared update
for the initial document and each later candidate. The view caches that root once.
It does not reconstruct a surviving component merely because CSS or nearby markup
changed.

An optional state identity is declared through the existing typed schema:

```cpp
Registration.Schema.StateKeyProperty = TEXT("resource");
Registration.Schema.Properties.Add(
    {TEXT("resource"), ECkUiCustomPropertyKind::Text, true});
```

For a registered `resource-editor`, the corresponding markup is:

```html
<resource-editor id="properties" resource="texture-17" />
```

The `resource` property must be required literal text and its value cannot be
empty. Changing `texture-17` while retaining id `properties` rejects the reload,
preventing an unfinished editor from silently becoming a different resource's
editor. Use a distinct component id for a distinct state owner. Without a state-key
property, the authored id and registered tag define identity. Omission releases
the component; re-addition creates a new instance.

`PrepareReload` receives the same resolved typed arguments as a stateless factory.
It must copy or own anything its returned update needs after preparation returns.
Null or a nonempty failure diagnostic rejects the whole document. Prepare must not
mutate mounted widgets, input drafts, focus, actions, models or timers. Previously
prepared updates are discarded if any later node fails.

The returned `ICkUiPreparedWidgetUpdate::Commit() noexcept` publishes prepared local
configuration once, after all mounts and the accepted revision are published.
Perform validation and allocations during preparation. Commit must not dispatch
edit/action callbacks, reenter the view, change focus or mutate consumer models.
For a form, keep editable draft state in the retained control and expose new labels
or presentation values through its configuration attributes; do not reset the
input with SetText during each reload.

The component owns its widget, but the widget must not strongly capture the
component. Use weak references or independently owned attribute state to avoid a
cycle. On successful removal, the view detaches the old port and releases its
component ownership after publishing the accepted document. External references
may deliberately keep a widget alive, but removed controls do not regain focus.
The runtime still uses normal Slate focus callbacks when refreshing a surviving
leaf's ancestry.

These interfaces are a trusted C++ extension point. Const preparation and noexcept
commit cannot undo arbitrary side effects in an implementation that violates the
contract. The production-path examples and rejection/lifetime tests are in
`CkTests/Private/UnitTests/CkYoga/Test_UiRetainedRegistry.cpp`.
### Custom root measurement

Stateless and retained custom ports forward FCkFlexMeasureMetaData attached to the
returned root, including measurement constraints and arranged-size notifications.
SCkFlexText supplies this metadata. Composite custom widgets must expose a suitable
measurement contract on their root; metadata is not discovered recursively.

## Typed tables (initial read-only cells)

Register an `FCkUiCollection` in `FCkUiView::FDataBindings::Collections`.
The collection owns the immutable field schema and stable keyed records; each
table owns its own filtering, sorting and selection. Columns and cells are authored:

```html
<table id="resources" bind="resources" row-height="24"
       filter-bind="query" selection-action="select-resource">
  <table-column id="name" label="Resource" sort-field="name">
    <text id="name-value" bind-field="name"/>
  </table-column>
</table>
```

`filter-bind` names a normal text attribute. `selection-action` names an
`FOnCkUiTableSelectionChanged` in `FDataBindings::TableSelectionChanged`; the callback
receives an optional stable key and Slate's selection reason. Filtering or removing
the selected record clears selection with `Direct`. `GetTable(id)->TrySelectKey`
selects a currently projected key; notification is opt-in for this programmatic API.

`context-menu-action` names an `FOnContextMenuOpening` in
`FDataBindings::TableContextMenus`. The prepared table configuration copies that
delegate only after the whole candidate document has staged successfully; rejected
reloads retain the prior callback. The consumer builds the returned menu content,
which keeps menu authoring outside the current markup language.

After a consumer commits its domain-to-record mapping, it may call
`GetTable(id)->TryRefresh()` to rebuild filtering/sorting/selection projection in the
same interaction. It returns false while the table is preparing, refreshing, or
notifying; leave the normal collection-change/tick path to perform the guarded
fallback rather than retrying reentrantly.

Text cells accept `bind-field`, `color-field`, and `tooltip-field`; image cells
accept `bind-field`; supported cell nodes accept boolean `visible-field`. These
references must match required fields in the collection schema. Global bindings
still work in cells and remain separate from row bindings. Cells support row,
column, text, image, scroll and stateless registered custom widgets. They are
generated only for realized native rows and use the same renderer as ordinary
document nodes. Editable controls and retained custom cells remain future work.

Custom binding properties can read row fields directly:

```html
<meter id="residency" fraction-field="residentFraction"
       tint-field="stateColor" label-field="stateLabel"/>
```

The registered `meter` schema in this example declares NumberBinding, ColorBinding
and TextBinding properties. Factories receive live attributes in NumberBindings,
ColorBindings and TextBindings. ImageBinding and BoolBinding work the same way.
`fraction-bind` reads a global binding, while `fraction-field` reads the current
record; supplying both is an error in either order. Template bodies can use
`tint-field-param` with a `color-binding` parameter. Row bindings are schema-checked
before factories run, including when there are no records. Registered factories
retain their normal detached, side-effect-free construction contract.

Column styles control header text and width: `flex-grow` selects a proportional
column; a fixed authored width selects a fixed column. Sort fields currently support
Text, Number and Bool, with stable key ordering for equal values. The native list,
header, selection key, and scroll offset survive a valid structural table reload;
the new authored header and cells replace only after preparation succeeds. Invalid
schema or cell markup rejects the complete document even when the collection has no
records.

## Document-local authored templates

Templates and their uses live in the same HTML-like resource and reload atomically.
They reuse authored layout rather than constructing another C++ widget tree:

```xml
<ui version="1">
  <template name="metric">
    <param name="title" type="text"/>
    <param name="value" type="text-binding"/>
    <column id="card">
      <text id="title" text-param="title"/>
      <text id="value" bind-param="value"/>
    </column>
  </template>
  <region name="main">
    <column id="metrics">
      <use template="metric" id="cpu" title="CPU" value-bind="cpu"/>
      <use template="metric" id="memory" title="Memory" value-bind="memory"/>
    </column>
  </region>
</ui>
```

All parameters are required. Supported types: text, number, bool, color,
text-binding, image-binding, number-binding, bool-binding, color-binding, action,
search-binding, native-binding. Bindings use argument-name-bind; other arguments use argument-name.
Inside a template body, append -param to the target attribute to reference a typed
parameter. Element text uses text-param. Nested uses forward parameters with the
same syntax. A literal value and parameter reference cannot both supply one field.
There is no string interpolation or markup execution inside parameter values.

Types must match exactly: search bind-param requires search-binding; text requires
text-binding; visible-param requires bool-binding. Native/search/image bindings
still undergo the ordinary view binding validation after expansion. Custom literal
numbers/colors/booleans remain materialized typed values through expansion.

Only template instances introduce ID scopes: cpu/card, cpu/title, cpu/value.
Ordinary row/column nesting does not add a scope. Instance IDs are also reserved;
colliding ordinary or expanded IDs reject. Generated IDs are limited to1024
characters. Changing an instance ID changes descendant identity and retained state.
Styles and visibility belong to the body; use has no implicit wrapper or class.
Forward template declarations work. Unknown or malformed unused templates reject.

The source AST is limited to512 nodes/declarations (including template and parameter
declarations),32 nested nodes,64 parameters per template. Expanded output has a
separate512-node/32-depth limit including uses. Memoized graph analysis rejects
recursive or excessive expansions before materializing instances. Native bindings
count only emitted occurrences, not unused declarations. Defaults, child slots and
cross-file template libraries are not yet implemented.

Template tags template/param/use and custom property suffix -param are reserved.
## Shared typed collection model

`CkUiCollection.h` supplies the game-thread model consumed by authored `<table>`
nodes. Create a schema with `TryCreate`, then publish complete `FCkUiRecordData`
arrays through `TrySetRecords`. Both return `FCkUiLoadResult` and preserve prior
accepted state on failure.

A record has a nonempty opaque string key and typed named fields (Text, Number,
Bool, Color, Image). Use a complete domain key, not a label or hash alone. Fields
must match the immutable schema; required fields must be supplied and unknown
fields reject. Numbers and color channels must be finite. An Image value owns a
shared const brush; null means blank. The consumer still owns any underlying
UObject resource and must preserve its valid rendering lifetime.

Surviving keys keep exactly the same shared FCkUiRecord object when values change.
Record handles are read-only to consumers, but their live values change after an
accepted update. Do not retain a pointer returned by FindField across publication;
retain the const shared record and query its field when needed. Removed records
release when the model and any remaining external readers release them.

Identical order and active field values are a no-op. A changed update installs all
records, key lookup and revision before one OnChanged notification. Reentrant
updates during publication reject; the model stays alive through notifications if
a listener releases the final client owner. Subscribe weakly from widgets.

This is a game-thread API. Limits:1..128 schema fields,100000 records,1000000 total
supplied fields,1024 characters per record key. Incoming order is the table's default
order; filter, sort and selection belong to each table view. Tables virtualize native
rows, so factories run only as rows are realized. Data-only scale tests do not prove
virtualization or UI performance.

## Common debugger widgets

`CkDebuggerCommon` owns optional debugger-specific authored widgets. Foundation does
not depend on that module and does not register these tags itself. A debugger consumer
calls `FCkDebug_UiRegistry::TryCreate`, checks the `FCkUiLoadResult`, and passes the
returned immutable snapshot to `FCkUiView::Create`.

```html
<column id="summary">
  <debug-meter id="budget" fraction-bind="budget-fraction" fill-bind="budget-color"
               width="96" height="4" tooltip-bind="budget-detail"/>
  <debug-status id="state" label-bind="state-label"
                foreground-bind="state-foreground" background-bind="state-background"
                show-dot="true" tooltip-bind="state-detail"/>
</column>
```

`debug-meter` requires Number and Color bindings for `fraction` and `fill`; `width`
and `height` are optional finite positive literals and default to 96 and 4. Its
optional tooltip is a Text binding. `debug-status` requires Text `label` plus Color
`foreground` and `background` bindings; `show-dot` is an optional literal Bool and
its tooltip is optional Text binding. All bindings stay live. The status border uses
the foreground at 0.5 alpha. The underlying `SCkDebug_StatusPill` can also use its
existing tone palette when its optional color attributes are unset.

## Trees

Bind a `FCkUiTreeCollection` through `FDataBindings.Trees`, and a selection delegate through `TreeSelectionChanged`. A tree contains exactly one read-only authored row root:

```html
<tree id="navigation" bind="categories" selection-action="select-category"
      filter-bind="navigation-query" row-height="24" class="fill">
  <row id="category-row"><text id="category-name" bind-field="name"/></row>
</tree>
```

The native STreeView owns indentation, expanders, keyboard navigation and virtualization. Row field bindings use the same types as table cells. Editable/retained custom row controls and multicolumn trees are not yet supported.

`FCkUiTreeNodeData` contains Key, optional ParentKey and Fields. Unset parent means root; an empty supplied parent is invalid. Publish using TrySetNodes: fields, roots, child adjacency and revision change together, then one OnChanged fires. Duplicate keys, absent parents, cycles, depth over256, malformed fields or excess limits reject without publication. Nodes retain shared identity by key. Limits match flat collections except the additional depth limit; roots and siblings preserve input order.

`GetTree(id)` exposes SCkUiTree. TrySelectKey and TrySetExpanded operate on stable keys. User expansion survives filtering and valid reload. Filtering includes text matches plus ancestors and temporarily expands matching paths; clearing the filter restores user expansion. A selected node removed by filtering or model publication clears selection once. GetVisibleNodeCount reports all nodes in the filtered projection, including collapsed descendants; GetLiveRowCount reports realized native rows.

First focused evidence:47 authoring tests pass in UiTree-Editor.log. Resource Inspector navigation, adversarial lazy-factory mutation tests, multicolumn trees and debugger migrations remain pending in the campaign ledger.

Table and tree programmatic selection replaces the native selected-item set, not only the adapter key. Same-key calls retain the current singleton; clearing leaves no native selections. Production regression: Ck.UiAuthoring.Selection.NativeSingleSelection. Wrapping geometry and invalid declarations are covered by Ck.UiAuthoring.Layout.FlexWrapRenderedGeometryAndValidation; the Resource Inspector toolbar opts into flex-wrap in its authored stylesheet.

## Shared retained text input

Register `FCkUiTextInput::Register(Registry)` and pass `Registry.CreateSnapshot()` to
`FCkUiView::Create`. Check the registration result before creating the view. The
control is supplied by CkSlateLayout and uses native Slate editing; consumers do
not construct its layout or input widget.

```html
<text-input id="session-note"
            value-bind="session-note"
            committed="commit-session-note"
            placeholder="Add a note"
            error-bind="session-note-error" />
```

`value-bind` names a live `FDataBindings.Text` attribute. Required `committed`
names a bound `FDataBindings.TextCommitted` delegate (`FOnTextCommitted`). Optional
`changed` names `FDataBindings.TextChanged`. `read-only-bind` and `enabled-bind`
use boolean attributes from `FDataBindings.Visibility`; `error-bind` reads text.
Missing or supplied-but-unbound callbacks reject before widget factories run.

Typing is a local draft. Enter or a focus-loss commit supplies text and the native
commit reason to the consumer; the consumer owns validation and publication. The
control then displays the authoritative value, including normalization or rejection.
Escape discards the draft without committing it. A layout reload preserves the
retained editor and draft; a value-binding name change under the same id rejects.
Use a distinct authored id when the editor's state owner changes. A stable binding
name must not silently begin targeting a different resource while a draft is active.

TextChanged and TextCommitted are also typed custom-registry property kinds and
can be forwarded through template parameters `text-changed`/`text-committed`.
Resolved arguments include source `BindingNames`, the consumer `BaseFont`, and
`CanDispatchEvents`. Event wrappers suppress delivery while the owning view reloads
or after it is released. Editable controls must check CanDispatchEvents before
changing local draft state as well: publication may cause synthetic focus callbacks.
Prepared Commit only swaps configuration; it must not synthesize edits or clear a
draft. Consumers should capture model owners weakly.

Text input does not enable editable virtualized table/tree cells. Undo-history and
text-selection preservation across reload, full navigation/IME/platform coverage,
and the other form control families remain campaign acceptance work. Current
runtime evidence is recorded in CkYoga/PROGRESS.md.


## Shared retained checkbox

Register `FCkUiCheckbox::Register(Registry)` before creating the view's registry
snapshot. As with other registrations, reject the whole setup if registration
fails. The native control is a runtime-safe `SCheckBox`.

```html
<checkbox id="lock-note" value-bind="note-locked"
          changed="set-note-locked" label-bind="lock-note-label" />
<text-input id="note" value-bind="note" committed="save-note"
            read-only-bind="note-locked" />
```

`value-bind` resolves a boolean attribute from `FDataBindings.Visibility` (the
existing boolean binding map). `changed` resolves an `FCkUiOnBoolChanged(bool)`
from `FDataBindings.BoolChanged`. The event carries a proposed value; the native
checked state always reads the model attribute. Ignoring a proposal rejects it
without leaving a temporary local selection. External model updates do not emit
change events.

`label-bind` resolves live text. Optional `enabled-bind` and `read-only-bind`
resolve boolean attributes; either disabled or read-only input suppresses edits.
A compatible reload preserves the native widget and swaps prepared binding
configuration without emitting changes. Retargeting `value-bind` under a surviving
id rejects the document. This is a two-state control; indeterminate checkbox state
is not part of this boolean contract.

`BoolChanged` is also a public custom-property kind. Templates forward these
events through a `bool-changed` parameter and `changed-param` on the custom tag.
View-resolved callbacks suppress dispatch during reload/publication and after
view release. Editable events remain forbidden in readonly table/tree cells.


## Shared retained numeric entry

Register `FCkUiNumberInput::Register(Registry)` and reject setup if registration
fails. The numeric adapter composes `FCkUiTextInput::Create`, so it shares native
text editing, draft preservation, Escape cancellation and validation presentation.

```html
<number-input id="resource-count" value-bind="row-count"
              committed="set-row-count" kind="integer" min="0" max="10000" />
```

`value-bind` reads `FDataBindings.Number` (float). Required `committed` resolves
`FCkUiOnNumberCommitted(float, ETextCommit::Type)` in `NumberCommitted`. Optional
`changed` resolves `FCkUiOnNumberChanged(float)` in `NumberChanged`, delivering
finite raw numeric drafts; use committed alone when partial edits must not update
the model. Templates use `number-changed` and `number-committed` parameter types.

`kind` defaults to `float`; `integer` rounds committed proposals. Optional finite
`min`/`max` constrain commits. Integer intervals must contain an integer, and the
committed result stays inside the bounds even when the bounds are fractional.
Parsing consumes the entire trimmed ASCII decimal/exponent string. Invalid and
nonfinite numbers do not reach consumer callbacks. The model remains authoritative:
a consumer can reject a commit by leaving its value unchanged and can supply
`error-bind` for its own validation feedback.

`placeholder`, `enabled-bind`, `read-only-bind` and `error-bind` follow the shared
text editor contract. Presentation reloads retain drafts and errors and apply a
new placeholder through the prepared child update. Changing value binding or kind
under a surviving id rejects. Model values use round-trip float formatting;
nonfinite model values render empty with validation feedback.

This entry does not yet provide slider/step interaction, double precision, optional
or mixed values, or localized numeric parsing. Those remain explicit campaign
capabilities; do not encode missing values using NaN. Native text undo/selection
retention across focus-path refresh also remains the shared editor's open gate.


## Numeric interaction events for custom controls

A custom schema can declare `ECkUiCustomPropertyKind::NumberInteraction` to receive
an `FCkUiOnNumberInteraction` from `FDataBindings.NumberInteraction`. It is an event
attribute, such as `interaction="preview-gesture"`, and forwards through template
parameters of type `number-interaction`. Resolved arguments expose the callback
in `NumberInteraction` and the authored source name in `BindingNames`.

`FCkUiNumberInteraction` carries `Phase` (Begin, Commit, Cancel), `Source` (Pointer,
Keyboard, Controller), and a finite float `Value`. Pointer includes mouse and touch.
The view rejects invalid enum values and nonfinite payloads before downstream
callbacks. Staging/publication and released views suppress delivery. Readonly
collection cells reject these edit events.

The owning control must enforce phase ordering and decide when native input means
commit versus cancellation. The registry only transports validated payloads. This
API alone does not supply a slider or prove native gesture, capture, controller,
or reload behavior; those controls must also meet their lifecycle tests.

### Retained pointer capture

A retained custom control opts into reload capture repair by overriding
`ICkUiRetainedWidget::GetPointerCaptures()`. Return active
`FCkUiPointerCapture { UserIndex, PointerIndex, Widget }` descriptors with weak widget
references. The view never acquires capture from a report: it checks that Slate
already assigns that exact pointer to the widget, and that the widget is beneath
the reporting component and a mounted region. Default-empty reports preserve
compatibility for controls without an active pointer interaction.

After publication, compatible components receive `BeginPointerCaptureTransfer`
and `EndPointerCaptureTransfer(..., InRestored)` around the public Slate capture
repair. Slate sends a synchronous capture-loss callback during repair. Suppress
local cancellation only for the descriptor inside those paired hooks; successful
repair continues the same gesture and must not emit another Begin. Failed repair
resets the local interaction. Hooks must not dispatch events or mutate models,
focus, capture, or the view. All consumer dispatch remains gated by
`CanDispatchEvents`.

Removal releases an owned pointer without transfer hooks: clear its local draft
and capture state in the normal capture-loss handler. Consumer events remain
suppressed during publication. A preview owner must therefore also scope transient
preview state to the control/view lifetime; removal is not a consumer Cancel event.
Game-thread view destruction also releases verified owned captures, including when a
window still holds the widgets. Stale or foreign reports cannot steal or release
another control's capture.
The contract supports explicit users and pointer indices; a cursor-only report
cannot establish touch or multiplayer input coverage.

### Slider (native interaction validation in progress)

Register `FCkUiSlider::Register(Registry)` to use:

```html
<slider id="row-count-slider" value-bind="row-count"
        min="0" max="10000" step="1000" interaction="set-row-count-slider" />
```

`value` is a float NumberBinding; `interaction` is a required NumberInteraction
event. Optional `changed` emits NumberChanged previews. `enabled-bind` and
`read-only-bind` use Boolean bindings. The defaults are min0, max1 and step0.01;
the range must increase, and step must be positive, no larger than the range,
and representable at the range's float precision. Pointer movement is continuous;
step controls keyboard/controller increments. The native slider uses normalized
0..1 values internally; bindings/events use the authored domain.

The thumb shows a local draft during an interaction. Begin carries the original
model value, Commit proposes the final draft, and Cancel carries the original
value. Consumers may reject a commit by leaving their model unchanged. Resource
Inspector updates its real collection only on Commit. Retained ids cannot retarget
the value binding; changing range/step during an active interaction rejects the
whole reload. The current native fixtures and remaining acceptance limitations are
tracked in CkYoga/PROGRESS.md and Plan/Gate_02h_Slider.md; registration is not a
claim of complete gesture, platform, accessibility or reentrant callback coverage.

Slider consumer callbacks may reload or remove the authored control. Pointer capture
is acquired through the current mounted path after Begin/Changed return, so reload
cannot leave acquisition tied to a detached ancestor. If the callback removes the
control or another widget takes the pointer, acquisition is abandoned. Input events
reentered synchronously into this slider during consumer dispatch or capture-end
processing are unhandled; the next input after the callback returns works normally.
This keeps Slate's post-callback capture release from erasing a nested new gesture.
Interrupted dispatch drains cancellation afterward, and removal during publication
retains the no-consumer-notification teardown contract.

A touch from the owning user during a controller session cancels that session first. Touch only
begins a new interaction after crossing the drag threshold. Escape and gamepad cancel
remain unhandled when the slider has no active or pending interaction.
An interaction belongs to its initiating Slate user. Other users cannot change,
commit, cancel, or take over that preview through keys, navigation, pointer-down,
or focus loss. Disabling or removing the control still ends its interaction.

Sliders accept literal orientation="horizontal" (default) or "vertical". Other
values reject the document. An idle axis change retains the native widget and
invalidates its layout. Axis changes during an active interaction or a touch waiting
for the drag threshold reject the entire reload. Vertical sliders increase toward
the top; Up increases and Down decreases by the authored step. Orthogonal keyboard
navigation escapes, while a locked controller session consumes it without changing
the draft.

### Custom widget CSS contracts

A registered widget can declare optional typed visual properties in
`Schema.StyleProperties`. Custom CSS names are lowercase and start with `-ck-`;
this keeps them distinct from the built-in layout vocabulary. A property name
shared by multiple widgets must have the same kind throughout a registry snapshot.

```cpp
Registration.Schema.StyleProperties.Add(TEXT("-ck-preview-tint"), ECkUiCustomStyleKind::Color);
Registration.Schema.StyleProperties.Add(TEXT("-ck-preview-border-width"), ECkUiCustomStyleKind::Length);
```

```css
.preview { -ck-preview-tint: var(--accent); -ck-preview-border-width: 2px; }
```

Factories and retained reload preparation receive resolved typed values in
`Arguments.Style.CustomProperties`. Absence means the property was not authored;
the component chooses its default. Number is a finite unitless value; Length is
finite and nonnegative, accepting unitless values or px; Color uses the same color
syntax as built-in styles. Tokens and class cascade follow existing CSS behavior.

Unknown names and malformed declarations reject even in unused rules. Applying a
registered visual property to a built-in node or a custom widget that has not opted
in rejects the document. Registration rejects conflicting property kinds atomically.
This contract transports visual configuration; components still own their native
brush/style lifetimes and must apply prepared changes without rebuilding identity
or dispatching consumer events. These declarations do not add browser pseudo-selectors.
Slider visual CSS properties are `-ck-slider-bar-color`, `-ck-slider-bar-hover-color`,
`-ck-slider-bar-disabled-color`, `-ck-slider-thumb-color`,
`-ck-slider-thumb-hover-color`, and `-ck-slider-thumb-disabled-color` (Color);
`-ck-slider-thumb-width`, `-ck-slider-thumb-height`, and
`-ck-slider-bar-thickness` (Length). Omitted values reset to the native Core Slider
style defaults. Thumb dimensions apply to all three states. Native Slate chooses
the hover/disabled brush; the stylesheet does not simulate input state.

Color-only reloads preserve an active draft/capture. A change to thumb dimensions
or bar thickness rejects while an interaction or pending touch exists, just like
an axis change. An idle change retains widget identity and invalidates layout.
Each native slider owns its style storage, including when held after view release.
### Stable-key selection

Register `FCkUiSelect::Register(Registry)` to use the shared native dropdown:

```html
<select id="category-select" value-bind="category"
        options-bind="category-options" changed="select-category-key" />
```

`FDataBindings.String` supplies a live `TAttribute<FString>` key;
`FDataBindings.Collections` supplies an `FCkUiCollection` with a required Text
field named `label`. Record keys identify options; labels may be localized or
updated independently. `FDataBindings.StringChanged` accepts an
`FCkUiOnStringChanged` callback proposing the selected key. The model decides
whether to accept it; rejection restores the model selection without an echo.
Optional `placeholder-bind`, `enabled-bind` and `read-only-bind` use the existing
Text/Bool transports. Missing selected keys display the placeholder without
changing the model.

Custom registry schemas may use `StringBinding`, `StringChanged`, and
`CollectionBinding`; factories receive `StringBindings`, `StringChanged`, and
`Collections`. Template parameter kinds are `string-binding`, `string-changed`,
and `collection-binding`. String values remain separate from display `FText`.
Editing events remain disallowed in read-only table/tree cell templates.

Compatible dropdown reload retains its native widget and open popup in both
native window-hosting modes. Changing its value or options binding under the
same id rejects. Live model and option updates are silent and keep the popup
open; removed keys cannot dispatch from stale rendered rows. Removing the
select closes its popup and clears focus owned by that popup. Native option data uses shared observable storage so a held popup
list cannot borrow a destroyed array. Programmatic selection is silent; the
adapter recognizes the engine's Direct notifications during native keyboard
input separately from its own model synchronization.
Retained custom widgets may provide a read-only GetFocusTransferTarget() ancestor
when focus ancestry must be refreshed without leaving a popup. The view validates
the target against the current focused path, transfers through a focusable
ancestor, and restores the original leaf only if callbacks did not redirect focus.
Other retained widgets use the existing clear/reacquire behavior.

## Tabs

`tabs` is a retained structural container. Its `value-bind` names an FString
attribute and `changed` names a StringChanged delegate. Immediate `tab` children
have unique nonempty keys and either `label` or `label-bind`; optional
`enabled-bind` uses a boolean in the view's Visibility bindings. Tab children are
ordinary authored layout and registered controls.

```html
<tabs id="details" value-bind="detail-tab" changed="select-detail-tab">
  <tab id="overview" key="overview" label="Overview">
    <text id="facts" bind="selected-detail" />
  </tab>
  <tab id="properties" key="properties" label="Properties">
    <text-input id="note" value-bind="note" committed="save-note" />
  </tab>
</tabs>
```

The model owns selection. Activation proposes a key; rejected changes leave the
selected panel unchanged. External changes are silent. Inactive panel hosts are
collapsed, while their child widgets remain owned. Compatible reload retains tabs
and headers by key; changing a surviving tabs id's value binding rejects.
Arrow/Home/End navigation changes header focus; native button activation selects.

Current implementation has shared native selected-header tint. Full authored
state styling, explicit tab accessibility roles, generic descendant popup/capture
cleanup, controller acceptance, and thorough panel draft/scroll/switch lifetime
coverage remain pending. The initial editor tests are not full tabs acceptance.
Tab headers wrap to additional rows using the allotted width. The verified narrow
layout keeps both Overview and Properties labels visible and positions panel
content below the wrapped strip; widening restores a single row.

When an active tab becomes inactive, the view releases pointer captures belonging
to that panel's retained descendants and invokes their optional
`ReleaseTransientInteraction()` hook before hiding the panel. The shared select
uses this hook to close its owned popup. Slider capture loss follows its existing
Cancel policy; it does not commit the drag. Ownership discovery includes collapsed
descendants during teardown. Custom controls that own transient UI should implement
the hook without destroying their persistent model or draft state.

Current tests cover select popups in both hosting modes, slider capture cancellation
and late pointer events on tab switching, plus view destruction during a captured
drag. Nested custom popup stacks, controller interactions, callback-driven panel
replacement and full accessibility remain separate acceptance requirements.
## Authored command menus

Top-level menu declarations describe commands independently of layout. A retained menu-button references one declaration; native behavior remains in the view's action bindings.

```xml
<ui version="1">
  <menu id="actions">
    <menu-item key="reload" label="Reload resources" action="reload-resources" />
    <menu-item key="clear" label="Clear selection" action="clear-selection" enabled-bind="has-selection" />
  </menu>
  <region name="main">
    <column id="root">
      <menu-button id="actions-button" menu="actions" label="Actions" />
    </column>
  </region>
</ui>
```

A menu entry uses label-bind for localized model text, enabled-bind/visible-bind for booleans, and tooltip or tooltip-bind for help text. Separators have stable keys; a submenu references another menu declaration. Missing references, cycles and expansion beyond the document limits reject before publication. View validation checks all declared action and value bindings before any widget factories run.

An accepted reload dismisses the owned menu and retains its button. A rejected reload preserves the open menu and current view. Removing the owner or hiding its tab closes owned transient UI. This first menu-button increment does not yet supply authored table/tree context-menu targeting; that follows with a stable row-key action contract. Native menu styling, controller/accessibility and packaged acceptance require their own evidence. See the campaign PROGRESS.md for verified versus pending behavior.
### Row context menus

Tables and trees can reference the same menu declarations used by menu buttons:

```html
<menu id="resource-context">
  <menu-item key="properties" label="Show properties" action="show-properties" />
</menu>
<table id="resources" bind="resources" context-menu="resource-context">
  <table-column id="name-column" label="Name">
    <text id="name-cell" bind-field="name" />
  </table-column>
</table>
```

Bind these commands in `FCkUiView::FDataBindings::ContextActions` using `FOnCkUiContextAction`, whose argument is the opening row's stable `FString` key. Ordinary menu-button commands still use `FActions`; an action bound only in the wrong map rejects the document before publication. Consumer code resolves current feature data from the supplied key and performs the behavior; it does not build an `FMenuBuilder` or inspect mutable selection to discover the target.

Right-click uses the clicked row even if a selection callback selects another row. Shift+F10 opens at the selected row or view; the dedicated Context/Menu key is not exposed by this engine's InputCore mapping. Empty-space clicks do not reuse an old target. Removed or replaced record identity and accepted document revisions invalidate an open context session. Accepted reload, hidden ancestor, and owner release close owned context popups; rejected reload preserves them. `context-menu` cannot be combined with legacy `context-menu-action`.
## Tables without selection

Use the literal boolean attribute `selectable="false"` on a table to present rows without selection. The default is true (single selection). Templates may forward it with `selectable-param` from a bool parameter. This does not disable scrolling, filtering, or context commands.

An accepted reload that disables selection keeps the native table/list identity and clears selection without a selection callback. Nonempty `TrySelectKey` requests are rejected while selection is disabled. Malformed boolean values reject the document before replacing accepted state.

## Localized headers and search hints

Use `label-bind` on `table-column` and `placeholder-bind` on `search` to read live `FText` values from the view's `Data.Text` bindings. These preserve localization identity; changing the bound value updates the native text without replacing the table or search control.

```html
<search id="filter" bind="filter-text" placeholder-bind="filter-hint" />
<table id="items" bind="items">
  <table-column id="name" label-bind="name-header">
    <text id="name-value" bind-field="name" />
  </table-column>
</table>
```

A column requires exactly one of `label` or `label-bind`. A search may omit its hint, or use exactly one of `placeholder` or `placeholder-bind`. Missing/unset text bindings reject the document atomically. Templates forward text-binding parameters with `label-bind-param` and `placeholder-bind-param`. Native attributes stop reading view bindings when their view is released.

## Inherited event eligibility

Native owners may set `FCkUiView::FDataBindings::CanDispatchEvents` to a live boolean
attribute. Unset preserves ordinary dispatch. False suppresses outbound actions,
search changes, form edits, selection callbacks and context commands; passive data
bindings remain live. Custom widget arguments receive the same eligibility and
wrapped callbacks. Controls may also reflect eligibility in native enabled state;
the gate does not define a uniform visual style for the subtree.

Use a weak owner in the attribute and return false when that owner or the item's
record identity expires. Keep the getter side-effect-free. Reload guards and
existing revision checks still apply. This primitive does not itself implement
repeat-item ownership or make child reloads atomic; those are the repeater's job.

## Reloading several independent views

Use `FCkUiView::TryReloadBatch` with `FReloadRequest` entries when several independent
mounted views must change together. Each request contains its view, markup,
stylesheet and diagnostic source name. The batch snapshots the requests; all views
must be distinct, live and on the game thread, with no reload already in progress.

All documents validate before factories run. A parse, binding, factory or ownership
failure preserves all existing mounts and revisions. Accepted batches publish every
view before reconciling focus/capture. Consumer actions and reentrant reloads stay
suppressed through the transaction, including temporary-state cleanup. A widget
cannot belong to two participants. Single-view `TryReload` uses the same machinery.

This API covers independent views. Repeated child-view ownership and nested
transaction composition are separate capabilities under implementation.

## Repeated authored items (in validation)

A repeat uses a typed collection and one item subtree. Item IDs are local to each
record; record order determines visual order. For example, a collection with required
Text `title` and Bool `expanded` fields can render independently collapsible cards:

```html
<repeat id="cards" bind="materials">
  <column id="card">
    <button id="heading" bind-field="title" item-action="toggle-material"/>
    <column id="details" visible-field="expanded">
      <text id="name" bind-field="title"/>
    </column>
  </column>
</repeat>
```

Register `materials` in `FDataBindings.Collections` and `toggle-material` in
`FDataBindings.ItemActions`. The item action receives the record key; the consumer
publishes the updated expansion field through the collection API. The renderer
validates field references against the schema even when the collection is empty.
Custom widget typed properties use the same `property-field="field"` syntax.

Value changes read through live attributes. Structural changes retain child views
for surviving record identities. Removing and reinserting a key creates a new
interaction scope, so externally held old controls cannot act on the new record.
Rejected preparation preserves the previous mounted layout. Inspect
`SCkUiRepeat::GetLastFailure()` for collection-driven preparation errors.

This increment rejects nested repeat/table/tree elements and native ports inside
items. It is not virtualized. Use tables or trees for large fixed-row collections;
repeat supports variable-height authored cards. Nested collections and broader
interaction/geometry acceptance remain campaign requirements.
Ordinary button labels use SCkFlexText with explicit Yoga width constraints. Native
SButton still owns clicks, focus, enabled state and normal/pressed padding; the
measurement adapter includes that padding and subtracts it from the label width.
Tabs expose constrained header-plus-selected-body measurement so vertical scroll
containers include wrapped content in their scroll extent. Measurement does not
select tabs or dispatch events. Focused evidence: Ck.UiAuthoring.Button.WrappingGeometryAndLifetime
and Ck.UiAuthoring.Tabs.ConstrainedScrollContent.
# Shared debugger inspector

Repeated button strips can use `<repeat direction="horizontal">` with
`flex-wrap: wrap`; the default direction remains vertical. Buttons accept
`color-bind`, `tooltip`, `tooltip-bind`, and repeat-scoped `color-field` and
`tooltip-field`, using the same typed field validation as text metadata.

Views using `FCkDebug_UiRegistry` can compose the existing collapsible inspector
with an authored body. The title binding supplies localized `FText`; compatible
reload preserves the user's expanded/collapsed state and the body slot mount.

```html
<debug-inspector id="profiles" title-bind="profiles-title">
  <slot name="body">
    <column id="profile-details">
      <text id="profile-hint" bind="profiles-hint" />
    </column>
  </slot>
</debug-inspector>
```

The debugger registry belongs to CkDebuggerCommon. Generic CkSlateLayout users
register their own components; they do not need a debugger module dependency.

## Retained local dialog

Register `FCkUiDialog::Register` and provide an explicit nonnegative host
`FDataBindings::SlateUserIndex`. The `dialog` tag requires `open-bind` (a bool
binding), `dismiss` (an action), and exactly one authored root in each required
`content` and `body` slot. The model owns open state; dismiss and authored body
buttons request model changes. The background content is disabled while open.
Focus, Back dismissal and navigation are scoped to the host user after mounting.
Owner, open binding and slot mounts are stable for a retained dialog ID.

Retained getters accept explicit `container-id/slot-name/child-id` paths for
custom-slot content. Exact local IDs (including template-expanded IDs) take
precedence; there is no implicit search through unrelated child scopes. These
lookups return the requested retained control, not a mutable child view.

The retained implementation uses the owning Slate user for focus, Back dismissal
and navigation, disables the background content while open, and releases only the
dialog owner's transient focus/capture state when it closes, is removed or is
destroyed. Accepted reloads retain compatible slot mounts and retained child identity; rejected
reloads preserve the existing dialog state. Resource Inspector exercises this
contract for its clear-pinned-snapshots confirmation through the qualified
`inspector-dialog/content` paths. The shared authoring gate passes 114/114 (R8) and Resource Inspector passes 10/10 (R5). These establish the focused native contract, not whole-campaign or packaged/controller-session acceptance.

Custom adapters receive `ReleaseSlotPointerCaptures(slotName)` for runtime or teardown use. It releases only the explicit host user's current captures beneath that slot, using routed pointer indices and retained capture reports. Do not call it from a factory, PrepareReload, or Commit. Closing releases body capture; opening releases background capture. Capture-loss callbacks are reentrant and may redirect ownership.
