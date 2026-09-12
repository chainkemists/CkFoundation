**Latest checkpoint (2026-09-11):** Input, Save, StyleLab, Dialog, and Texture authored migrations are accepted as bounded slices. R72 passes the serialized authored cohort 12/12; Dialog proves timed-record/repeat-item identity across real time advancement, while Texture proves real-window six-tab physical routing, active-page state, and Refresh selection retention with inspected wide/narrow captures. Texture PIE, outer-shell migration, checker Apply/Restore, responsive browser parity, packaged/controller/accessibility/localization/performance remain unclaimed; full campaign remains open and unpublished.

**Previous checkpoint (2026-09-08):** Shared intrinsic-region fix is verified by126existing authoring cases plus focused RegionIntrinsic R2 1/1. Corrected cached Dialog R5 passed1/1 in34s with one editor start, exit0 and zero failed/skipped/contaminated tests. Fresh960x640 and520x620 captures were inspected and accepted: all native Chrome controls are readable with no overlap, and timed/Forever rows plus authored panes remain coherent. Scoped Dialog migration accepted; full campaign remains open and unpublished.

# Gate 05: Dialog Debugger conventional migration

**Status:** Accepted for the scoped authored Dialog migration. R5 verifies the real registry/cooldown production path, native input/actions, retained Forever identity, real timed progress, reload/teardown rejection and fresh wide/narrow presentation. The shared native-region desired-size repair is separately verified by the composite126+1 authoring evidence in `CkYoga/PROGRESS.md`. Live style axes, broader environment acceptance and the full campaign remain open.

## Current implementation checkpoint

The authored resource pair mounts four named regions (`cooldown-controls`, `runtime-commands`, `search`, and `main`) through the native window chrome’s single view. The shared lifecycle is synchronous and clear; preference controls remain generation-gated, while Save/Load actions use the live-world gate. Real registry/filter projections, command routing, reload, and cleanup have progressed, while native controls, Chrome geometry, forever-row identity, stale Save disabling, and TextScale/RowDensity remain unresolved acceptance gates.

## Scope

Migrate the conventional static and cooldown presentation of `SCkDialogDebuggerWindow` to an authored CkSlateLayout document while retaining the native window, collector, command routing, filtering semantics, and owner/lifetime gates. The target is the whole window surface: runtime commands, cooldown controls, search/highlight controls, grouped cooldown rows, empty state, and diagnostic text. Painted or specialized overlays are outside this slice.

## Current window contract

`SCkDialogDebuggerWindow::Construct` owns two runtime command buttons (`Save`, `Load`), an `Active cooldowns only` toggle, a dual search bar, a scrollable cooldown host, and `_ContentText`. `DoExecCommand` routes `Ck_Save`/`Ck_Load` through the active PIE player controller with an engine fallback. Search writes `_FilterString` and `_HighlightString`; changing the active-only toggle resets `_LastCooldownSignature`.

`DoBuild_CooldownSignature` and `DoRebuildCooldowns_Structure` currently rebuild structure only when the visible emitter/cooldown identity set changes. `DoUpdateCooldowns_LiveValues` updates retained row values each refresh. `DoRebuildContent` produces the diagnostic text with the same filter semantics. Preserve `DoPassesFilter`: filter matches line ID for diagnostic lines, and emitter debug name or simple emitter tags for emitter sections. Active-only keeps forever or positive-remaining cooldowns.

## Authored projection

Use one flat vertical `repeat` collection. CkSlateLayout already supports `visible-field` inside repeat items, typed `bind-field`, `color-field`, and the registered `debug-meter` with `fraction-field` and `fill-field`. A repeat item may not contain nested repeat/table/tree/native nodes.

Project records from `FCkDialogDebugger_EmitterInfo` and `FCkDialogDebugger_CooldownInfo`:

- Emitter identity is the collected `FCk_Handle_DialogEmitter`; use the handle identity together with the current world/session generation and entity version in the key, not display name alone. The exact handle encoding remains an implementation detail to verify. Record key shape: `emitter:<generation>:<entity-version>:<handle-identity>`.
- Cooldown identity is the owning emitter identity plus `FCkDialogDebugger_CooldownInfo::LineID` (`FName`) within that same generation/version scope. Record key shape: `cooldown:<generation>:<entity-version>:<handle-identity>:<LineID>`.
- Emitter records provide `is-emitter`, heading/debug name, and any tag summary needed by the authored row.
- Cooldown records provide `is-cooldown`, `LineID`, remaining text, numeric `fraction`, and `meter-color`.

The row template uses mutually exclusive `visible-field="is-emitter"` and `visible-field="is-cooldown"` subtrees. Cooldown rows contain line text, `debug-meter`, and remaining text. Outer bindings provide count, empty state, and diagnostic content. Do not add a new adapter or custom tag until the existing repeat/table field path proves insufficient.

## Ownership and refresh

C++ remains responsible for world collection, complete record projection, filtering, active-only admission, command execution, and weak-owner dispatch. Update the retained collection atomically: construct the complete next record set, then publish it once; rejected or unavailable refreshes must leave the prior authored view intact. Preserve stable record keys and mutate live remaining/fraction/color fields in place when membership is unchanged. Rebuild only for membership/order/filter/active-only changes. Invalidate the view generation before detaching or replacing authored widgets so held search, toggle, and command callbacks become inert.

The retained view belongs to the Dialog debugger window and must survive ordinary same-owner refreshes and compatible resource reloads. World/session loss must invalidate the generation, disable the surface, and clear stale records through the normal owner tick path; an old-world snapshot must never remain runnable. No callback may capture owning dialog handles across teardown. Accepted reload preserves collection identity, active drafts, focus, and live callbacks; a rejected reload preserves the previous document and callbacks only within the same valid generation.

## Required implementation files

- `Plugins/CkGameplayDebugger/Source/CkDialogDebugger/Public/CkDialogDebugger/Window/SCkDialogDebuggerWindow.h/.cpp`: retained authored view, bindings/actions, generation gate, record projection, refresh and reload polling.
- `Plugins/CkGameplayDebugger/Resources/UI/DialogDebugger.ui.html/.css`: complete authored document and responsive presentation.
- `Plugins/CkGameplayDebugger/Source/CkDialogDebugger/CkDialogDebugger.Build.cs`: CkSlateLayout dependency and explicit NonUFS resource staging if absent.
- A focused Dialog authored-control test in the module’s existing test location; use real collector snapshots and installed resources rather than fabricated partially initialized rows.

## Acceptance requirements

1. Mount the complete window and drive Save, Load, active-only, filter, and highlight controls through authored bindings/actions; verify exact existing routing and filter semantics.
2. Verify emitter and cooldown record keys, grouped ordering, active-only/empty behavior, line IDs, remaining text, meter fraction/color, count, and diagnostic text against the native collector snapshot.
3. Change remaining time and forever state across refreshes; prove stable row widgets/keys are retained for unchanged membership and only membership changes rebuild the collection.
4. Exercise emitter/cooldown add/remove, filter changes, active-only changes, unavailable PIE/world state, and owner expiry. Assert atomic refresh and inert stale callbacks.
5. Exercise compatible reload with focus/search draft and collection identity retained, plus rejected markup/styles preserving the prior view.
6. Capture narrow and wide responsive layouts with a real cooldown row and reachable command/search controls. Inspect a fresh startup/test log for ensures, parser errors, and script errors.

## Deferred and non-claims

This queued plan does not claim runtime acceptance or add a new meter/repeat adapter. Browser-reference parity, painted overlays, and unrelated debugger consumers remain outside the slice. No R5 Visual LOD result is implied by this plan.

## Source basis

- `Plugins/CkGameplayDebugger/Source/CkDialogDebugger/Public/CkDialogDebugger/Window/SCkDialogDebuggerWindow.cpp`: `Construct`, `DoExecCommand`, `DoPassesFilter`, `DoRebuildContent`, `DoGet_CooldownFraction`, `DoBuild_CooldownSignature`, `DoMake_CooldownRow`, `DoRebuildCooldowns_Structure`, and `DoUpdateCooldowns_LiveValues`.
- `Plugins/CkGameplayDebugger/Source/CkDialogDebugger/Public/CkDialogDebugger/Data/CkDialogDebugger_Types.h`: `FCkDialogDebugger_CooldownInfo`, `FCkDialogDebugger_EmitterInfo`, `FCkDialogDebugger_RegistrySnapshot`, `FName LineID`, and typed dialog handles.
- `Plugins/CkGameplayDebugger/Resources/UI/TextureHealth.ui.html`: existing `debug-meter` table cell.
- `Plugins/CkGameplayDebugger/Resources/UI/SurfaceLighting.ui.html`: existing repeat `visible-field` projection.
- `Plugins/CkFoundation/Source/CkSlateLayout/Private/CkUiDocument.cpp` and `SCkUiSurface.cpp`: repeat field scope, visibility, typed fields, and no nested repeat/table/tree/native rule.
