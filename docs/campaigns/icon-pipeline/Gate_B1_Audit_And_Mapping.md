# Icon Pipeline Campaign — Phase B, Gate 1: Audit + Mapping Table

Authored 2026-08-17, read-only (no CkGameplayDebugger edits). Builds on Phase A's
`Gate_1_Discovery_Report.md` §1.5. Appendices in this folder:

- **`phaseB-icon-ids.txt`** — every icon id referenced by name in debugger source, with every
  `path:line`; plus the 71 corpus files with zero code references.
- **`phaseB-unicode.txt`** — all 72 icon-like unicode markers in display strings, with `path:line`.

Method: the 206 vendored SVG basenames were cross-referenced against every string literal in
`CkGameplayDebugger/Source` (all contexts — `Get_IconBrush`, `.IconId(...)`, aggregate-init
toggle actions, mapper returns, `Get_IconName` overrides). Numbers below are from that sweep.

## 1. The complete visual-element inventory

### 1.1 The icon corpus and its three registries (the system being replaced)

| Element | Evidence |
|---|---|
| 206 first-party SVGs (58 semantic root + 148 `General/` pool), 16×16 white-stroke | `CkGameplayDebugger/Resources/Icons/**` |
| Registry 1: `CkDebugger.Icon.<Base>` @16 + `GeneralIconPool` | `CkDebuggerCommon/.../CkDebuggerStyle.cpp:173-203`, accessor `:216`, pool `:208` |
| Registry 2: `CkCommon.Icon.<Base>` @16 (same files re-registered) | `CkDebuggerCommon/.../CkDebuggerCommonStyle.cpp:181-196` |
| Registry 3: `CkDebuggerLauncher.Icon.<Base>` @24 (same files, third time) | `CkDebuggerLauncher/.../CkDebuggerLauncherStyle.cpp:113-131` |
| NonUFS staging of the SVGs (×2) | `CkDebuggerCommon.Build.cs:66-75`, `CkDebuggerLauncher.Build.cs:28-34` |

**Usage totals:** 134 of 206 icons are referenced by name in code (57 root + 77 pool); 1 is
spec-test-only (`Skull` — the doctrine example); **71 are never named in code** and are live only
through the General-pool hash-pick. Named references span ~70 `Get_IconBrush` sites, ~35
`.IconId(...)`/toggle-action sites, 47 inspector `Get_IconName` overrides, and 7 id-mapper
functions.

### 1.2 Identity carriers (how ids flow — these all become `ECk_Icon` plumbing)

| Carrier | Evidence | Migration shape |
|---|---|---|
| Inspector contract `Get_IconName() -> FName` (47 overrides) | `CkEcsDebugger/.../CkDebuggerInspector_Base.h:36`; registry copies it `CkDebuggerInspectorRegistry.cpp:81,94` | virtual becomes `Get_Icon() -> ECk_Icon` (default = entity fallback) |
| Feature-visuals metadata `FCkDebuggerFeatureVisual::IconName` | `CkEcsDebugger_FeatureVisuals.h:15` | field becomes `ECk_Icon` |
| Archetype buckets `IconName` + `Cube` fallback + pool hash-pick | `CkEcsDebugger_ArchetypeAggregation.cpp:84-126` | typed field; hash-pick over a GENERATED pool array |
| Toggle actions `FCkDebug_IconToggleAction::IconId` (~20 aggregate-init sites) + `SCkDebug_IconToggle` | `SCkDebug_IconToggle.cpp:59,101,244` | field becomes `ECk_Icon` |
| Launcher tool descriptor `_IconId` | `CkDebuggerToolRegistry.h:55`; consumed `SCkDebuggerLauncher.cpp:204` | typed; registrants updated |
| Save-debugger model rows `IconId` + mappers | `CkSaveDebugger_Model.h:71,471`; `.cpp:233,241,1027-1052` | mappers return `ECk_Icon` |
| Tone axis `ck::debug_axes::Get_ToneIconId(ECk_Tone)` | `CkDebuggerAxes.cpp:1427-1445` | returns `TOptional<ECk_Icon>` (Neutral/Accent draw nothing — keep that contract) |
| Optimization mappers (category/severity/cleanup/profile-group) | `SCkOptimizationDebuggerWindow.cpp:133-170,442-460,2085` | return `ECk_Icon` |
| Window-local `Get_IconBrush` wrappers | e.g. `SCkSaveDebuggerWindow.cpp:119-123`, `SCkOptimizationDebuggerWindow.cpp:111-115` | become `FCkIconStyle::Get_Brush` calls |
| Launcher tab icon — the one `FSlateIcon` with a real key | `CkDebuggerLauncher_Module.cpp:53-55` | needs a generated style-key accessor (`FCkIconStyle::Get_StyleKey`) so no hand-written string survives |

### 1.3 The external contract that CANNOT become a closed enum — decision needed

`FCk_ArchetypeDescriptor::_IconSvgPath` (`CkFoundation/Source/CkEcs/Public/CkEcs/Archetype/CkArchetype_Data.h:50`)
lets **game plugins** name an SVG path for their archetypes (doc example `"Icons/Shelf.svg"`,
`CkArchetype_Data.h:91`); the debugger resolves its basename against the registry
(`CkEcsDebugger_ArchetypeAggregation.cpp:86-88`). A generated enum is closed — a game cannot add
entries to it. Options:

- **(a) RECOMMENDED — keep a dynamic side-lane**: `FCkIconStyle` gains
  `Register_DynamicIcon(FName Id, FString SvgPath)` + `Get_DynamicBrush(FName)`, used ONLY by the
  archetype-descriptor path. First-party code stays 100% typed; game-supplied icons keep working;
  the lane is one function pair, not a parallel system.
- (b) Change the descriptor field to `ECk_Icon` — games lose bespoke icons (breaking change to a
  reflected CkEcs struct + any game assets/AS setting it; workspace search required at Gate 2).
- (c) Leave archetype icons on the old registry — forbidden by "no parallel systems left standing".

### 1.4 Non-corpus visuals

| Element | Evidence | Verdict proposed |
|---|---|---|
| 5 GraphEditor PNGs (arrow, dashed border, panel bg, colorspill, node icon) | `CkDebuggerStyle.cpp:117-131` | **Keep 4** (they are graph plumbing textures — tiling backgrounds, 9-slice borders — not icons); `Trans_Node_Icon` (25×25 PNG) → `ECk_Icon` (`transition`), delete the PNG |
| 2 glow PNGs (9-slice halos) | `CkDebuggerCommonStyle.cpp:140-146` | Keep — not icons |
| `Devices/Gamepad_Master.svg` | `CkDebuggerCommon/Resources/` | Migrate: `Input` semantic already covers it → verify its consumer (input debugger device pane) at migration |
| Engine icon via `FAppStyle` (`GenericCommands.Copy`) | `CkDebug_CopyMenu_Utils.cpp:22-24` | Migrate to `ECk_Icon::Copy` (`content-copy`) — the one FAppStyle icon; it also violates the packaged-builds rule their own doc states (`CkDebuggerCommon/CLAUDE.md:73-76`) |
| `WhiteBrush`/`NoBorder`/`GenericWhiteBox` engine brushes (~30 sites) | e.g. `SCkDebug_Sparkline.cpp:29` | **Not icons** — utility fills; out of scope |
| Colour-only indicators (`SCkDebug_CategoryDot`, status pills, tone chips) | e.g. `SCkInsightsAnalyzerTab.cpp:142` ("a marker, not a glyph — stays an SImage") | **Do not migrate** — deliberate design (severity-reads-from-shape doctrine covers the glyph cases; dots are dots) |
| Empty `FSlateIcon()` in menus (~30 sites) | e.g. `CkDebuggerWidget_EntityTree.cpp:2091` | Out of scope (no visual exists; adding icons = behavioural change the campaign forbids smuggling in) |

### 1.5 Unicode markers in display strings (72 hits — appendix `phaseB-unicode.txt`)

Proposed split, per the campaign's "flag any element where an icon is the wrong answer":

- **Migrate to icons (~12 sites)** — markers acting as standalone glyphs in UI chrome:
  `▶` active-row marker (`SCkAggroDebuggerWindow.cpp:614` → `play`), `⚠`/`🧪` gallery badges
  (`CkGallery_MissionControl.cpp:221,229` → `alert`, `flask-outline`), `⚙` picker settings
  (`SCkDebug_ViewportPickerControls.cpp:164` → `cog-outline`), `|◀`/`▶|` timeline transport
  (`SCkGoapDebuggerWindow.cpp:447,449` → `skip-backward`/`skip-forward`), `◀`/`▶` graph-pane
  nav (`SCkGoapDebugger_GraphPane.cpp:824,856`), `◀ NOW` marker (`SCkGoapDebugger_AgentColumn.cpp:717`).
- **Keep as text (~60 sites)** — an icon is the wrong answer:
  - `→` inside transition/history strings (`%s → %s` — SM/GOAP/Audio/Overlay): these strings are
    **copyable payloads** (`SCkDebug_CopyableContainer`/`SCkDebug_HistoryRow` copy them to the
    clipboard); replacing text with widgets breaks copy semantics.
  - Disclosure carets `▸▾▼›‹` (row expanders — Slate-conventional, sized-to-font).
  - Legend swatches `■●◆` (GOAP node-kind/pool legends, scheduler cards — they are colour swatches
    keyed to graph paint, not glyphs; the graph draws the same shapes via geometry).
  - Math/typography `∞ ≤ ≥ − ±` and `→` in tooltips/log lines.
  - `•` bullet lists in gallery demo copy.

## 2. Proposed mapping table (`current id → ECk_Icon semantic → MDI @ v7.4.47`)

Every MDI name below was validated against the pinned `mdi-names.txt`. Sites per id:
appendix `phaseB-icon-ids.txt`. `(?)` = low-confidence pick, please amend.

### 2.1 Feature identities (inspector glyphs, window identities, entity tree)

| Current id | ECk_Icon | MDI | Note |
|---|---|---|---|
| Cube (entity fallback) | `Entity` | cube-outline | Phase A seed |
| ActorBridge | `ActorBridge` | bridge | |
| Aggro | `Aggro` | target-account | |
| AStar | `AStar` | vector-polyline | |
| Attribute | `Attribute` | chart-bar | current glyph is also a bar chart |
| AttributeByte | `AttributeByte` | alpha-b-box-outline | |
| AttributeFloat | `AttributeFloat` | decimal | |
| AttributeInteger | `AttributeInteger` | numeric | |
| AttributeRotator | `AttributeRotator` | rotate-3d-variant | |
| AttributeVector | `AttributeVector` | axis-arrow | |
| Audio | `Audio` | waveform | |
| Camera | `Camera` | video-outline | |
| EntityCollection | `EntityCollection` | select-group | |
| EntityInfo | `EntityInfo` | card-bulleted-outline | |
| Eqs | `Eqs` | magnify-scan | |
| FogOfWar | `FogOfWar` | weather-fog | |
| FrameActor | `FrameActor` | vector-square | |
| Goap | `Goap` | graph-outline | |
| Grid | `Grid` | grid | |
| Input | `Input` | gamepad-variant-outline | also covers Gamepad_Master.svg |
| Interaction | `Interaction` | gesture-tap | |
| Inventory | `Inventory` | briefcase-outline | |
| IsmRenderer | `IsmRenderer` | checkbox-multiple-blank-outline | |
| Jolt | `Jolt` | lightning-bolt-outline | |
| Label | `Label` | label-outline | |
| Minimap | `Minimap` | map-outline | |
| Network | `Network` | lan | |
| Objective | `Objective` | flag-outline | merges pool `Flag` |
| OverlapBody | `OverlapBody` | set-center | |
| PathNetwork | `PathNetwork` | road-variant | |
| PathNetworkFollower | `PathNetworkFollower` | navigation-variant-outline | |
| Physics | `Physics` | atom | |
| Pin | `Pin` | pin-outline | Phase A seed; also Poi inspector |
| Probe | `Probe` | radar | |
| Relationships | `Relationships` | account-group-outline | |
| Resolver | `Resolver` | call-merge | |
| SceneNode | `SceneNode` | file-tree | |
| SelectInViewport | `SelectInViewport` | cursor-default-click-outline | |
| Shapes | `Shapes` | shape-outline | |
| StateMachine | `StateMachine` | state-machine | MDI has the exact concept |
| Timer | `Timer` | timer-outline | Phase A seed |
| Transform | `Transform` | arrow-all | |
| Tween | `Tween` | transition | also replaces Trans_Node_Icon PNG |
| Variables | `Variables` | variable | |
| Vfx | `Vfx` | creation | |
| World | `World` | earth | shared: opt "world", save EngineOwned |
| ViewFront/Back/Left/Right/Top/Bottom/Perspective (7) | — | — | **FLAGGED: icon-may-be-wrong-answer.** 3D-viewport orientation buttons; MDI has no face-of-cube set. Options: (i) keep these 7 bespoke SVGs as manifest entries pointing at retained house assets (pipeline supports any vendored SVG), (ii) approximate with `rotate-orbit`/arrows (poor), (iii) text buttons. Recommend (i). |

### 2.2 Severity (shape-distinct per `CkDebuggerCommon/CLAUDE.md:70-72`)

| Current id | ECk_Icon | MDI | Shape |
|---|---|---|---|
| Severity_Info | `Info` | information-outline | circle-i |
| Severity_Success | `Success` | check-circle-outline | circle-check |
| Severity_Warning | `Warning` | alert-outline | triangle |
| Severity_Error | `Error` | alert-circle-outline | circle-! |

Tone → glyph stays one rule (`Get_ToneIconId` returns the typed id; Neutral/Accent stay glyphless).

### 2.3 Save-debugger domain

| Current id | ECk_Icon | MDI |
|---|---|---|
| Cassette | `SaveSlot` | cassette |
| Package | `Payload` | package-variant-closed |
| Lock | `Locked` | lock-outline |
| Key | `SaveKey` | key-outline |
| Gear | `Settings` | tune |
| Scale | `Size` | scale |
| Bug | `Diagnostics` | bug-outline |
| Ghost | `Orphaned` | ghost-outline |
| Trap | `CycleRoot` | sync-alert |
| Rocket | `RuntimeSpawned` | rocket-launch-outline |
| Factory | `DefinitionBuilt` | factory |
| Anchor | `Anchored` | anchor |

### 2.4 Optimization-debugger domain

| Current id | ECk_Icon | MDI |
|---|---|---|
| Palette | `TextureAsset` | palette-outline |
| Brush | `MaterialAsset` | brush-outline |
| Bulb | `Lighting` | lightbulb-outline |
| Person | `Actor` | account-outline |
| Puzzle | `Fragment` | puzzle-outline (Phase A seed; also Blueprint category + DynamicFragments inspector) |
| Crate | `Duplicates` | package-variant |
| Scissors | `Redirectors` | content-cut |
| Bucket | `DirtyPackages` | bucket-outline |
| Broom | `Cleanup` | broom |
| Wrench | `Fix` | wrench-outline |
| Flame | `HotPath` | fire |
| Chip | `Hardware` | chip |
| Stopwatch | `ProfileTiming` | av-timer |
| Snowflake | `Freeze` | snowflake |
| Moon | `Dormant` | sleep |
| Hourglass | `Waiting` | timer-sand |

### 2.5 Remaining named pool glyphs (semantic per dominant use — amend freely)

| Current id | ECk_Icon | MDI | | Current id | ECk_Icon | MDI |
|---|---|---|---|---|---|---|
| Antenna | `Broadcast` | antenna | | Note | `Note` | note-outline |
| ArrowProjectile | `Projectile` | ray-start-arrow | | Pencil | `Edit` | pencil-outline |
| Backpack (?) | `Loadout` | bag-personal-outline | | People | `Crowd` | account-multiple-outline |
| Bolt (?) | `Power` | flash-outline | | Plug | `Connection` | power-plug-outline |
| Bomb (?) | `Destructive` | bomb | | Radio | `Channel` | radio |
| Book | `Catalog` | book-open-outline | | Rail | `Rail` | train-car |
| Calendar | `Schedule` | calendar-outline | | Ring (?) | `Ring` | ring |
| Camcorder | `Recording` | video-vintage | | Scroll | `Log` | script-outline |
| Chest (?) | `Storage` | treasure-chest | | Shield | `Protection` | shield |
| Clipboard | `Report` | clipboard-outline | | Speech | `Dialog` | forum-outline |
| Coin | `Currency` | currency-usd | | SpeakerBox | `Speaker` | speaker |
| Compass | `Compass` | compass-outline | | Sun | `DayTime` | white-balance-sunny |
| Crosshair | `Aim` | crosshairs | | Sword (?) | `Combat` | sword |
| Disc (?) | `Media` | disc | | Target | `Target` | target |
| Door | `Door` | door | | Ticket | `Ticket` | ticket-outline |
| FilmReel | `Cinematic` | movie-open-outline | | Tree | `Foliage` | pine-tree |
| Footprint | `BodyActivity` | shoe-print | | Tv | `Screen` | television |
| Gate | `Gate` | gate | | Wave | `Signal` | waves |
| Gem (?) | `Rarity` | diamond-stone | | Web | `Web` | spider-web |
| Hand | `Grab` | hand-back-right-outline | | Well (?) | `Well` | water-well |
| Mic | `Microphone` | microphone-outline | | Wheel (?) | `Vehicle` | ferris-wheel |
| Monitor | `Display` | monitor | | Wind | `Wind` | weather-windy |
| Needle (?) | `Precision` | needle | | Window | `UIWindow` | window-closed-variant |
| Net (?) | `Capture` | web | | | | |

Plus from §1.4-1.5: `Copy`/content-copy, `Play`/play, `SkipForward`/skip-forward,
`SkipBackward`/skip-backward, `Experiment`/flask-outline (gallery 🧪), `Alert` reuses `Warning`.

### 2.6 The hash-pool (71 unreferenced files + the pool mechanism)

The pool exists purely for **stable visual variety** (archetypes without a bespoke glyph hash into
it, `CkEcsDebugger_ArchetypeAggregation.cpp:122-124`). Proposal: do NOT map the 71 unreferenced
files 1:1. Instead the manifest gains a `"pool": true` flag; the generator emits
`ck::icons::Get_GeneratedPool() -> TConstArrayView<ECk_Icon>`; the pool = the ~50 §2.5 glyphs plus
any extras you want for variety. Hash stays `FCrc::StrCrc32(Key) % Pool.Num()` — picks will
reshuffle once (pool size changes); that is cosmetic and inevitable under any migration.

## 3. Deletion candidates (Gate 2 will verify zero external references before ANY deletion)

- All 206 SVGs under `CkGameplayDebugger/Resources/Icons/**` + `Devices/Gamepad_Master.svg` +
  `GraphEditor/Persona/StateMachineEditor/Trans_Node_Icon.png`
- `FCkDebuggerStyle::CreateIconBrushes/Get_IconBrush/Get_GeneralIconPool/GeneralIconPool`
  (`CkDebuggerStyle.cpp:173-220`, `.h:35-46,81`)
- `FCkDebuggerCommonStyle::CreateIconBrushes/Get_IconBrush` (`CkDebuggerCommonStyle.cpp:113-127,181-196`)
- `FCkDebuggerLauncherStyle::CreateIconBrushes/Get_IconBrush` (`CkDebuggerLauncherStyle.cpp:47-54,113-131`)
- The two icon-staging `foreach` blocks (`CkDebuggerCommon.Build.cs:66-75`, `CkDebuggerLauncher.Build.cs:28-34`)
- Spec tests are REWRITTEN, not deleted: `CkDebuggerToneIcons.spec.cpp`,
  `CkDebuggerStyle.spec.cpp`, `CkEcsDebugger_InspectorIcons.spec.cpp`,
  `CkDebugIconToolbar.spec.cpp`, `CkSaveDebugger_Registration.spec.cpp`,
  `CkDebuggerLauncherCatalog.spec.cpp` — they pin the new registry instead
- Known external-contract risks for Gate 2: `_IconSvgPath` values in game plugins/assets
  (`CkTests/Script/CkEcs/CkArchetype_AuthoringValidation.as` already found); BusterBlock archetype
  descriptors; anything referencing `CkDebugger.Icon.*` keys as strings

## 4. Should-NOT-migrate list (explicit, with reasons)

1. Copyable transition/history strings keep `→` (clipboard semantics — §1.5).
2. Disclosure carets, legend swatches, math glyphs stay text (§1.5).
3. Colour-only dots/pills stay colour-only (deliberate, documented design).
4. Graph plumbing PNGs except `Trans_Node_Icon` (textures, not icons).
5. Glow PNGs (9-slice halos).
6. Empty `FSlateIcon()` menu entries (adding icons = smuggled behavioural change).
7. The 7 `View*` orientation glyphs — recommend keeping the bespoke SVGs as vendored manifest
   entries rather than forcing MDI approximations (§2.1 flag).

## 5. Adjacent observations (recorded, not acted on)

- The corpus is registered **three times** (~412 redundant brush objects + duplicate
  rasterizations); migration collapses to one registry with per-size entries — memory win for free.
- `CkEditorTools`'s division-of-labour comment (`CkDebuggerStyle.h:14-17`, "CkStyle:: … no Slate
  objects") needs updating once the debugger consumes `FCkIconStyle` (Phase A already put the icon
  registry there with the maintainer's approval).
- 16 is the only size the debugger uses today outside the launcher (24); the Style Lab's `IconSize`
  axis speaks of 12/16/20 glyph sizes (`CkStyleLab_AxisMetadata.cpp:26`) but sizes the *widget*,
  not the brush — worth confirming whether 12/20 brushes should be registered too during migration.

## 6. Decisions needed at this gate

1. **Approve/amend the mapping tables** (§2.1-2.5) — especially the `(?)` rows and every semantic rename.
2. **View-cube glyphs**: keep bespoke SVGs via manifest (recommended) or force MDI approximations?
3. **`IconSvgPath` contract**: dynamic side-lane (recommended §1.3a) or breaking enum change (§1.3b)?
4. **Pool strategy**: curated generated pool (§2.6) or 1:1 mapping of all 148 pool files?
5. **Unicode split** (§1.5): approve the migrate/keep classification?

**STOPPED — awaiting approval before any code is written.**

## 7. Early Gate-2 sweep (2026-08-17 — re-run at the real Gate 2 before deleting anything)

Workspace swept: `D:/Repositories/CkRepos/*` (BusterBlock, BusterBlock_5.5, CkPlugins,
CkPlugins_Other), excluding Intermediate/Binaries/Saved/binary assets and the sibling
CkGameplayDebugger checkouts themselves (same repo on other branches, not consumers).

- **Debugger style keys/classes** (`CkDebugger.Icon.*`, `CkCommon.Icon.*`,
  `CkDebuggerLauncher.Icon.*`, `FCkDebuggerStyle`/`FCkDebuggerCommonStyle`/
  `FCkDebuggerLauncherStyle`, `Get_GeneralIconPool`) outside the plugin: **zero code references**
  — only this campaign's own docs. BusterBlock game code never touches the debugger styles.
- **`IconSvgPath` setters**: exactly ONE live external reference —
  `CkTests/Script/CkEcs/CkArchetype_AuthoringValidation.as:15` (`IconSvgPath = "Icons/Cube.svg"`,
  which names a deletion-candidate file). Phase B must update this CkTests test in step with the
  corpus deletion (a CkTests submodule touch — plan the cross-repo commit pairing).
  BusterBlock itself sets `IconSvgPath` nowhere (no game archetype uses a bespoke icon today).
- **`Resources/Icons` path strings** elsewhere: BB doc `Script/Npc/AI/PROGRESS_NpcCombatAggroRefactor.md:152`
  (doc mention of icon id `Aggro` — update the doc line at migration); unrelated third-party
  plugin `EditorScriptingTools` has its own same-named folder (no relation).
