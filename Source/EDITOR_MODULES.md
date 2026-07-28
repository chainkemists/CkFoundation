# CkFoundation Editor Modules

Reference for all `*Editor` modules and editor infrastructure modules. These modules load only in the editor. They extend the editor's Details panels, asset graphs, toolbars, and style system. **Do not depend on these from runtime (non-editor) modules.**

Each module has a stub `Claude.md` in its own folder that points here.

---

## Editor infrastructure (shared by all *Editor modules)

### `CkEditorGraph`
**Depends on:** `CkCore`, `CkLog`. **Used by:** every `*Editor` module.

Base graph/schema infrastructure for CkFoundation editor graphs. Provides node base classes and connection validation that `CkAttributeEditor`, `CkCueEditor`, `CkDynamicEditor`, etc. build on. If you're implementing a new CkFoundation asset type with a graph editor, inherit from the base classes here.

### `CkEditorStyle`
**Depends on:** `CkCore`, `CkSettings`. **Used by:** every `*Editor` module.

Shared icon/color/font style for all CkFoundation editor UIs. Reference the style via `FCk_EditorStyle::GetStyleSet()`. Add new icons here when building an editor module — don't embed raw brushes in individual editor modules.

### `CkEditorToolbar`
**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkResourceLoader`, `CkSettings`, `CkUI`.

The main CkFoundation editor toolbar (registered with UE's menu extension system). Buttons for: run code generator, open data viewer, trigger Insights capture. Extend by adding a toolbar entry via `CkEditorToolbar`'s extension point — don't add Ck-specific tools to UE's main menu directly.

---

## Editor modules paired with runtime modules

Each entry follows the pattern: **purpose, runtime twin, unique editor additions.**

### `CkCoreEditor`
**Runtime twin:** `CkCore`. **Depends on:** `CkCore`, `CkLog`.

Details panel customizations for core CkFoundation types (`FCk_LogCategory` picker, `FCk_CVarRef` picker). Automatically loaded by the editor; no action needed.

### `CkLogEditor`
**Runtime twin:** `CkLog`. **Depends on:** `CkCore`, `CkLog`.

Log category management UI — lists all registered `FCk_LogCategory` instances, lets you enable/disable categories at runtime in editor. Also registers the log output redirector to the Message Log.

### `CkCVarEditor`
**Runtime twin:** `CkCVar`. **Depends on:** `CkCVar`, `CkCore`, `CkEditorGraph`, `CkLog`.

Details customization for `FCk_CVarRef` — replaces the raw name field with a searchable CVar picker dropdown populated from all registered `IConsoleVariable*` instances.

### `CkEcsEditor`
**Runtime twin:** `CkEcs`. **Depends on:** `CkCore`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`.

ECS debugging panels — entity browser, processor timeline, handle inspector. Available in the editor's "CkFoundation" menu. Use to inspect live entity state during PIE.

### `CkEcsExtEditor`
**Runtime twin:** `CkEcsExt`. **Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkEditorGraph`, `CkLog`.

Additional editor support for EcsExt concepts (SceneNode hierarchy display, EntityHolder visualizer).

### `CkAttributeEditor`
**Runtime twin:** `CkAttribute`. **Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`, `CkRecord`.

Details panels for attribute asset types — modifier stacks, provider pickers, min/max clamping visualization. Essential for designers authoring attribute configurations.

### `CkCueEditor`
**Runtime twin:** `CkCue`. **Depends on:** `CkCore`, `CkCue`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`, `CkUI`.

Base cue graph editor — the shared editing infrastructure that `CkAudioEditor`, `CkVfxEditor`, and `CkObjectiveEditor` build on. Provides the node graph canvas, connection types, and cue-asset compilation.

### `CkAudioEditor`
**Runtime twin:** `CkAudio`. **Depends on:** `CkAudio`, `CkCore`, `CkCue`, `CkCueEditor`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`.

Audio cue asset graph editor. Authors create audio cues by connecting nodes (SoundBase, conditions, modifiers) in a visual graph. Compiles to `FCk_Fragment_AudioTrack_ParamsData`.

### `CkVfxEditor`
**Runtime twin:** `CkVfx`. **Depends on:** `CkCore`, `CkCue`, `CkCueEditor`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`, `CkVfx`.

VFX cue asset graph editor. Authors connect Niagara system nodes, transform offsets, and trigger conditions in a visual graph.

### `CkObjectiveEditor`
**Runtime twin:** `CkObjective`. **Depends on:** `CkCore`, `CkCue`, `CkCueEditor`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`, `CkObjective`.

Objective asset graph editor — stages, conditions, completion rewards authored visually.

### `CkDynamicEditor`
**Runtime twin:** `CkDynamic`. **Depends on:** `CkCore`, `CkDynamic`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkLog`, `CkUI`.

Dynamic behavior editor — shows the behavior stack on a selected entity, lets designers swap behaviors at edit time.

### `CkInventoryEditor`
**Runtime twin:** `CkInventory`. **Depends on:** `CkCore`, `CkDynamic`, `CkEcs`, `CkEditorGraph`, `CkEditorStyle`, `CkInventory`, `CkLog`.

Inventory grid editor — visualizes item slot layout, lets designers configure grid sizes and item constraints.

### `CkJoltEditor`
**Runtime twin:** `CkJolt`. **Depends on:** `CkCore`, `CkEcs`, `CkJolt`, `CkLog`, `CkThirdParty` (+ UnrealEd, EditorSubsystem, AssetRegistry, ToolMenus, Landscape). Type: Editor.

Jolt static-world cooker — `FCk_Jolt_WorldCooker` (shared with nothing else; the runtime uses the same `ck::jolt::bake` extraction), `UCk_JoltCook_EditorSubsystem_UE` (Cook/Validate current map, Tools-menu entry), `UCk_JoltCook_Commandlet` (`-run=CkJoltCook`).

### `CkOverlapBodyEditor`
**Runtime twin:** `CkOverlapBody`. **Depends on:** `CkCore`, `CkLog`.

Shape visualization overlays for overlap body entities in the editor viewport.

### `CkPathNetworkEditor`
**Runtime twin:** `CkPathNetwork`. **Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkPathNetwork` (+ UnrealEd, NavigationSystem, PropertyEditor, Slate). Type: UncookedOnly.

Path-network authoring tools for `ACk_PathNetwork_UE`. The Details customization supports
detector bake, generated-ribbon promotion/clearing, authored-ribbon creation, and navmesh
validation. Reusable editor automation and Editor Utility Blueprints should call
`UCk_Utils_PathNetworkEditor_UE::Bake_DetectorToActor` and
`Validate_RibbonPointProjectability` instead of reproducing detector/vectorization logic.
Generated ribbons whose tips extend beyond navigable space may be normalized with
`Trim_UnprojectableGeneratedRibbonEndpoints`.

`Bake_DetectorToActor` owns an undo transaction, preserves authored ribbons, replaces generated
ribbons, and treats an empty detector mask as a successful authored-only bake. It never saves a
package. `Validate_RibbonPointProjectability` is read-only and requires the caller to supply the
projection extent used by its runtime navigation policy; it proves projectability within that
extent, not that source points already lie on the navmesh. It fails if the world has no navigation
system or default Recast nav data. Game-side automation remains responsible for map loading,
detector configuration, package saving, and deciding whether zero generated ribbons is acceptable.
`Trim_UnprojectableGeneratedRibbonEndpoints` uses the same caller-supplied projection extent, preserves
authored ribbons exactly, removes only generated unprojectable prefixes and suffixes, drops generated
ribbons reduced below two points, and fails atomically if any remaining generated point is unprojectable.

### `CkResourceLoaderEditor`
**Runtime twin:** `CkResourceLoader`. **Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkResourceLoader`, `CkSettings`.

Asset dependency viewer — shows which assets are loaded/unloaded by `CkResourceLoader` at any point during PIE.

---

## Non-editor-variant modules in this group

### `CkPieLayoutEditor`
**Depends on:** `CkCore`, `CkLog`, `CkSettings`. **No runtime twin** (`Type: Editor`).

Auto-arranges Play-In-Editor (PIE) multiplayer windows into a monitor-aware grid after Play starts. Settings (`UCk_PieLayout_Settings_UE`) appear under Editor Preferences → CkFoundation → "PIE Play Grid" and persist to `EditorPerProjectUserSettings.ini`. The `UCk_PieLayout_Subsystem_UE` editor subsystem hooks `FEditorDelegates::PostPIEStarted`/`EndPIE`, runs an initial arrange pass after a configurable delay, then keeps a short retry window alive (driven by `FCk_Chrono` over an `FTSTicker`) so late-spawning client windows are also placed without re-moving already-arranged ones. The Tools → "Kirosho PlayGrid" menu adds "Open Preferences" and "Arrange PIE Windows Now". Supports four layout modes (Auto Balanced, Equal Grid, Host Focus, Compact) and four monitor targets (Primary, Editor, Mouse Cursor, Specific index).

### `CkBuildConfig`
**Depends on:** nothing. **Used by:** `CkCore`, `CkLog`.

Contains only build-configuration `.h` files that set compile-time flags (`CK_DISABLE_ENSURE_CHECKS`, `CK_DISABLE_ENSURE_DEBUGGING`, etc.). No C++ classes. Referenced by build rules and the Ensure system.

### `CkEcsTemplate`
**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Runtime systems that spawn entities from template data assets.

Entity template data assets and the associated spawn infrastructure. See `CkTemplate/Claude.md` for the runtime pattern — `CkEcsTemplate` is the complementary data-asset-authoring layer for templates that need the full `CkEcsExt`/`CkProvider` feature set.

---

## Rules for editor modules

1. **Never reference `*Editor` modules from runtime modules.** Build rules enforce this but the linker will catch violations. Runtime modules must be usable in packaged builds.
2. **All editor UI lives in `*Editor` modules.** Don't put `WITH_EDITOR` blocks with substantial logic in runtime modules — move them to the paired `*Editor` module.
3. **Share editor style** — use `CkEditorStyle` for icons, colors, and fonts. Don't embed raw `FSlateIcon` literals in individual editor modules.
4. **Graph editors all build on `CkCueEditor` or `CkEditorGraph`.** Don't create a new graph canvas from scratch; extend the existing infrastructure.

---

## See also
- `CkEcs/Claude.md` — the ECS debugging features surfaced by `CkEcsEditor`.
- `CkCore/EditorOnly/README.md` — editor-only runtime utilities (distinct from editor modules).
- Root `/Source/CLAUDE.md` — architecture overview.
