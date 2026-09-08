# Gate 02d: declarative widget registry and retained components

## Purpose and boundary

The current CkSlateLayout authoring surface has a deliberately fixed set of nodes:
`row`, `column`, `text`, `button`, `native`, `search`, `image`, and `scroll`.
`FCkUiDocumentParser` recognizes their tags and attributes directly, while
`FCkUiView` separately validates and stages them. That is enough for Gate 02b,
but it cannot support independently authored widget types without modifying both
implementation files.

This gate introduces a typed registry that can be extended by trusted C++ modules.
It does not load native code, execute scripts, or grant markup access to arbitrary
Slate classes. A markup file selects only types present in the immutable registry
snapshot captured when its `FCkUiView` is created.

Existing `FCkUiView::Create`, `FCkUiDocumentParser::TryParse`, built-in tags,
`FDataBindings`, native ports, file polling, diagnostics, and source syntax stay
compatible throughout the migration.

## Final public extension contract

The code shapes below are architectural sketches for the full retained/component
contract. The implemented stateless and retained APIs are documented in
`CkSlateLayout/AUTHORING.md` and the public `CkUiWidgetRegistry.h`: custom schemas,
typed factory arguments, `Register` and `CreateSnapshot`. It uses copied schema
values plus a trusted factory function rather than a virtual factory object.
The retained implementation contract appears at the end of this document. Template
and collection work below remains required; it is not implied by the registry API.

Add public registry types under `CkSlateLayout`:

```cpp
enum class ECkUiPropertyType : uint8
{
    String, Text, Number, Boolean, Color, Length,
    TextBinding, ImageBinding, VisibilityBinding, Action, NativeBinding,
    TableBinding, ComponentTemplate,
};

struct FCkUiPropertySchema
{
    FName Name;
    ECkUiPropertyType Type;
    bool bRequired = false;
};

struct FCkUiWidgetSchema
{
    FName TypeName;
    TArray<FCkUiPropertySchema> Properties;
    int32 MinChildren = 0;
    int32 MaxChildren = 0; // INDEX_NONE means bounded by document node limit.
    TSet<FName> AllowedStyleProperties;
    bool bRetainsState = false;
};

class CKSLATELAYOUT_API ICkUiWidgetFactory
{
public:
    virtual ~ICkUiWidgetFactory() = default;
    virtual const FCkUiWidgetSchema& GetSchema() const = 0;
    virtual FCkUiPrepareResult Prepare(
        const FCkUiResolvedNode& Node,
        FCkUiPrepareContext& Context) const = 0;
};

class CKSLATELAYOUT_API FCkUiWidgetRegistry final
{
public:
    auto Register(TSharedRef<const ICkUiWidgetFactory> Factory) -> FCkUiRegistryResult;
    auto Freeze() const -> TSharedRef<const FCkUiWidgetRegistrySnapshot>;
};
```

`Register` rejects empty names, duplicate type names, invalid schemas, duplicate
property names, unsupported property types, invalid child ranges, and a factory
whose retention declaration has no reconciler. Registration is startup-only.
`Freeze` produces an immutable lookup table. `FCkUiView::Create` accepts an
optional snapshot after the current arguments; null means the built-in snapshot.
The view never observes later registrations, so an open document has deterministic
parse and reload semantics.

The public factory contract is intentionally trusted but constrained:

- `Prepare` may allocate detached Slate widgets and detached component candidates.
- `Prepare` must not add children to a mounted widget, attach an existing retained
  widget, set focus, execute actions, mutate consumer models, start timers, write
  files, register delegates against a strong owner, or publish external state.
- `Prepare` returns failure with diagnostics rather than throwing/ensuring on
  malformed authored input. A failed candidate is discarded without commit.
- Factories receive resolved, typed values only. They never parse XML, CSS, token
  strings, or look up arbitrary binding maps themselves.
- Factories capture consumer state only through weak references or caller-owned
  attributes/delegates. The view, registry snapshot, and staged candidate must not
  form a strong cycle with a consumer.

The trusted-factory rule is an integration boundary, not a sandbox. Third-party
or game modules may register factories only through C++ startup code already trusted
to create Slate widgets. Loose markup cannot nominate a module, class, or callback.

## Typed document and property resolution

Evolve `FCkUiNode` from the current special fields (`Binding`, `Action`, `Text`,
`Placeholder`, `VisibilityBinding`) to a node type plus a map of typed properties.
Keep a compatibility projection for the existing built-ins until all current staging
paths use the generic resolved representation.

The parser performs these steps before any factory runs:

1. Look up the tag in the view snapshot and reject unknown types.
2. Check every attribute against that type's schema, including required attributes,
   duplicates, exact property type, and child count.
3. Resolve CSS tokens and parse allowed style properties into typed style values.
4. Resolve `TextBinding`, `ImageBinding`, `VisibilityBinding`, `Action`,
   `NativeBinding`, and `TableBinding` against the view's typed binding surfaces.
   Required attributes must be set/bound, not merely present in a map.
5. Build a complete `FCkUiResolvedDocument`; validate global ids, region inventory,
   stable retained identities, component-template references, recursion, and limits.

No user-visible widget factory runs until the full resolved document succeeds. This
retains the current rejected-document guarantee: prior tree, revision, focus, native
state, and retained control state remain intact.

## Retained component contract

Stateful controls use a separate adapter from ordinary factories:

```cpp
class ICkUiRetainedComponent
{
public:
    virtual ~ICkUiRetainedComponent() = default;
    virtual auto PrepareReload(
        const FCkUiResolvedNode& Previous,
        const FCkUiResolvedNode& Candidate,
        FCkUiPrepareContext& Context) const -> FCkUiPrepareResult = 0;
    virtual auto Commit(FCkUiPreparedComponent&& Candidate) noexcept -> void = 0;
};
```

`PrepareReload` checks a stable component id, type and declared state key, then
builds only detached ports/configuration. It may reject a state transfer. It must
not move the retained widget. `Commit` is non-failing: all validation, allocations,
and fallible Slate construction finish before it starts. It only swaps ports,
publishes prepared configuration, and releases prior component state after the new
state is installed.

Search and native bindings become the first two internal retained adapters. Their
current id/binding stability behavior remains. Later retained components may be
removed: a valid candidate may omit a prior component id, and commit detaches that
component's port, clears its focus ownership if needed, and releases the retained
instance after all new ports are attached. Removal is rejected only when the type
declares a required counterpart or its consumer reports that removal is unsafe.

Focus restoration is calculated before detachment and applies only when the focused
widget belongs to a retained component that survives reconciliation. A removed
control must not regain focus. A prepared component Commit must not emit edit/action callbacks. The framework
refreshes retained focus through the supported Slate focus-loss/reacquisition cycle;
normal focus/text-commit callbacks may therefore run. Search text keeps its live model
attribute and an existing search is never reset with `SetText` during reload.

## Binding lifetime and typed table data

`FDataBindings` remains source-compatible. Add typed binding tables alongside it,
not a stringly-typed universal map. Every binding entry is a caller-owned attribute,
delegate, or weak provider with documented resource lifetime. Image/table providers
must retain their brushes, Slate resources, and row models for as long as a rendered
widget can dereference them.

Table data has its own typed contract:

```cpp
struct FCkUiTableRowKey { FString Value; };
struct FCkUiTableData
{
    TAttribute<TArray<TSharedPtr<const ICkUiTableRow>>> Rows;
    TFunction<FCkUiTableRowKey(const ICkUiTableRow&)> KeyOf;
};
```

Keys are unique, nonempty, and stable for the same logical row across snapshots.
Table reconciliation preserves selection, expansion, scroll anchor, and row-local
retained state by key; it never preserves identity by array position. Duplicate or
missing keys reject the candidate update before list mutation. Virtualized row
generation remains owned by the typed table component, never by generic markup.

## Component templates

Allow a registry snapshot to contain named component templates after basic widget
registry support is proven. Templates are parsed to resolved node fragments, not
string-expanded XML. Each instance receives a lexical instance prefix; authored ids
become `instance-id/local-id` internally, while the template body cannot reference
or collide with ids outside its instance.

Template invocation has typed parameters with defaults and required checks. Resolved
template depth is capped by the existing document depth limit, total expanded nodes
remain under the existing node limit, and a template call stack rejects direct or
indirect recursion. Templates cannot introduce new factory types or bypass property
schemas. A failed expansion rejects the whole candidate before preparation.

## Incremental implementation slices

### Slice A — built-in registry, no behavior change

**Files:** `CkUiDocument.h/.cpp`, new internal `CkUiWidgetRegistry.h/.cpp`,
`SCkUiSurface.cpp`, parser/view tests.

Move the eight current node schemas into a built-in snapshot and route existing parse
and stage code through registry lookups. Preserve the current public `Create` call
shape and document syntax. Do not expose registration yet.

**Acceptance:** all current authoring tests pass unchanged; unknown tag/attribute
diagnostics stay source-prefixed; a failed parse or validation leaves revision and
mounted tree unchanged.

### Slice B — public immutable snapshots and stateless extension

**Files:** public registry header, registry implementation, CkSlateLayout module
startup, focused registry tests, one test-only external factory module.

Publish registry construction and `Freeze`; add the optional snapshot parameter to
`FCkUiView::Create`. Demonstrate an external stateless `notice` widget with typed
text/color properties. Registration after freeze must not affect an existing view.

**Acceptance:** duplicate registration fails deterministically; unknown/mistyped
factory properties reject before `Prepare`; a prepared external widget is absent from
the mounted tree until successful full commit; old views use their original snapshot.

### Slice C — retained adapters and removal

**Files:** `SCkUiSurface.h/.cpp`, retained component interfaces, search/native
adapters, retention tests.

Replace current search/native bookkeeping with the common retained protocol. Add a
test component whose removal releases a destructor-visible token.

**Acceptance:** stable id/type/key retains input and focus; changed key rejects;
successful removal detaches/releases the old component and does not restore focus;
a late factory preparation failure leaves every old component attached and usable.

#### Slice C integration order

1. Replace the separate native/search port records with one retained record and
   common detach/attach transaction. Native bindings retain their required
   exactly-once contract. A search may be omitted; the accepted commit drops its
   owned widget and hint state. Reusing its id with another kind or binding in the
   same candidate remains an error. Reintroducing an omitted search starts a new
   instance from the current model.
2. Capture focus ownership against the old authored subtree before detach. Refresh
   ancestry only for a surviving retained leaf. Clear focus for removed or replaced
   authored leaves even when another owner retains the old widget/tree. Do not alter
   unrelated focus or override a callback that moves focus elsewhere.
3. Extend that shared mechanism with the public retained custom component prepare
   and non-failing commit protocol. This remains necessary after steps 1 and 2;
   merely retaining built-in searches does not complete Slice C.

The existing production search/native staging in SCkUiSurface is the reference
pattern. Keep initial search callbacks suppressed and bound text live. Tests must
hold an old root deliberately when checking focus removal, and release deliberate
strong references before checking weak-pointer expiration. Rejecting a candidate
must preserve the old root, revision, control state and focus without removals.

### Slice D — typed tables and templates

**Files:** table binding/component API, template resolver, parser tests, production
table adapter tests.

Introduce stable-key reconciliation and lexical template instance ids. Keep the
existing Texture inventory as an adapter until table semantics are fully proven.

**Acceptance:** reorder preserves selection by key; duplicate/missing keys reject
without mutating the list; nested template ids do not collide; recursion and expanded
node/depth limits reject atomically.

## Observable gate acceptance

- One focused CkSlateLayout automation suite verifies all rejection and retention
  cases above using real Slate widgets, not a markup mock.
- One focused Texture Health suite proves its native inventory, splitter, selected
  preview ownership, search input, clear action, and live file reload still operate.
- A capture at narrow and normal dimensions proves direct-text scroll wrapping and
  no clipping; table rows remain virtualized under a large stable-key fixture.
- Build and test through UnrealToolbox only after the implementation slices are
  integrated. A green focused suite supports only the stated focused claim.

## Slice C public retained API implementation contract

The concrete public API extends the existing registration without replacing it:
`Factory` creates stateless widgets; `RetainedFactory` creates an
`ICkUiRetainedWidget`. Exactly one must be supplied. The retained instance exposes
its stable root through `GetWidget` and returns an `ICkUiPreparedWidgetUpdate` from
`PrepareReload`, for both the initial document and later accepted candidates.
A null result or nonempty diagnostic rejects the complete candidate.

`Schema.StateKeyProperty` optionally names a required literal Text property. Its
nonempty resolved text, the authored id and custom tag identify the retained state.
A surviving id cannot change its tag or key. With no key property, id and tag define
identity. Omission releases the retained instance after publishing the accepted
maps and mounted regions. Re-addition constructs a new instance. This uses parsed
typed property values and needs no new XML parser syntax.

All preparation and validation finishes before detaching old ports. The view owns
prepared updates until the entire candidate succeeds. It mounts all regions and
publishes the retained map/revision before calling each non-failing update Commit.
Old instances remain alive through publication. Commit swaps prepared local
configuration only: no edit/action callbacks, focus, view reentry or model/timer
side effects. Normal framework focus callbacks occur afterward. A failed candidate
must invoke zero prepared commits and leave every existing control usable.

This is a trusted C++ extension contract. Const preparation and noexcept commit
do not sandbox a component or undo an implementation that violates the contract.
Stored resources and captures must have explicit lifetime; component widgets must
not strongly capture their owning component and create a cycle.
## Template authoring boundary for Slice D

Reusable layout bodies must be authored in HTML-like resources, not rebuilt as
C++ node/slot trees. Registration may accept a resource/body and typed parameter
schema, but it must compile the body into a validated internal AST. Parameter
substitution operates on typed AST fields, never by rewriting XML strings. An
explicit parameter reference must match the target field's property/binding type.

Instance ids form lexical prefixes for local ids. Expanded nodes/depth count toward
the existing document limits; duplicate ids, unknown parameters/types, missing
required arguments and recursive expansion reject the whole candidate before any
factory or retained Prepare runs. Literal text is never interpreted as executable
markup. The resource inspector must use declarative collection/table adapters as
they land; native table/preview/details islands are not a substitute for the final
app, even if they make a template fixture easier to demonstrate.
## Slice D1 concrete authored template contract

Implemented and focused-tested; current evidence is in PROGRESS.md under Slice D1 final evidence.
Templates are document-local declarations alongside regions in the same resource:

```xml
<ui version="1">
  <template name="metric">
    <param name="title" type="text"/>
    <param name="value" type="text-binding"/>
    <column id="card" class="card">
      <text id="title" text-param="title"/>
      <text id="value" bind-param="value"/>
    </column>
  </template>
  <region name="main">
    <use template="metric" id="cpu" title="CPU" value-bind="cpu"/>
  </region>
</ui>
```

All parameters are required in this first slice. Types are text, number, bool,
color, text-binding, image-binding, number-binding, bool-binding, action,
search-binding and native-binding. Binding arguments use name-bind; other arguments
use name. A body references a parameter with the target attribute plus -param;
literal text content uses text-param. Nested use arguments may forward parameters
with the same convention. Literal and reference forms cannot both be present.
There is no dollar interpolation or XML string substitution. Resolved public nodes
remain concrete; references exist only in the private parsed AST.

Use accepts id, template and declared arguments, with no children or text. Style and
visibility belong to the authored template body, not an implicit extra wrapper.
Body classes use the same document stylesheet. The example emits cpu/card,
cpu/title and cpu/value; nested use ids extend this lexical prefix. Invocation ids
are reserved for collision detection. Renaming an instance intentionally changes
its descendant identities and therefore their retained state.

Parse every definition even when unused. Reject duplicate names/parameters/local
ids, unknown references, missing arguments, exact-type mismatches, cycles and
malformed literals before publishing any candidate. Bound parsed definitions and
expanded output by the existing node/depth limits, including invocation nesting.
Expansion must not recurse or allocate unbounded intermediate trees before checking
those limits. Native binding uniqueness is checked on emitted instances globally.
All factories and retained preparation remain downstream of complete parsing.

Verification uses the production parser and view: nested styled reuse, all typed
field forms, literal escaped markup, rejection preserving the prior output/tree,
zero callbacks on rejected candidates, and expansion/recursion limits. Defaults,
slots and cross-file libraries remain later work; this slice does not claim them.