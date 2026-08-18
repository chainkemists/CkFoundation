# Icon Pipeline Campaign — Phase B Completion Report

2026-08-17. Branches (all local, nothing pushed):

| Repo | Branch | Commits |
|---|---|---|
| CkFoundation | `feature/icon-pipeline` (rebased onto `origin/dev`; backup `backup/icon-pipeline-pre-rebase`) | `436797134` pipeline · `0cd736a1c` TEMP verify window · `72e2b03fa`/`73b7e6d5f` docs · `290aa0944` full manifest · `9858e141a` None sentinel · (+ final docs commit) |
| CkGameplayDebugger | `feature/icon-pipeline` (off `dev`) | `26302a7` typed migration · `a4e75fb` deletion · (+ final docs commit) |
| CkTests | `feature/icon-pipeline` (off `origin/dev`, parked branch `feature/icon-pipeline-base`) | `8a2b84a5` archetype-validation icon path |

**Checkout note for the maintainer:** all three submodules are parked on their icon branches;
the save-load world returns with `git switch feature/save-load-optimization` (CkFoundation,
CkTests) once this campaign is reviewed. The two campaigns share no files except `Source/CLAUDE.md`
(both edit different rows; merges are clean).

## What the migration did (mechanism level)

- **Every icon identity is now the generated `ECk_Icon`** (144 semantics + `None`): toggle actions
  (`FCkDebug_IconToggleAction.IconId`), `SCkDebug_IconToggle` args, all 47 inspector overrides
  (`Get_IconName() -> FName` became `Get_Icon() -> ECk_Icon`), inspector-registry metadata,
  feature visuals, archetype buckets, launcher tool descriptors, save-model rows, the tone axis
  (`Get_ToneIconId -> Get_ToneIcon`), and the seven id-mapper functions.
- **String keys are gone from call sites**; the launcher tab's `FSlateIcon` goes through
  `FCkIconStyle::Get_StyleKey`. Launcher renders at 24px, everything else at 16px — both real
  registered brushes.
- **The archetype `IconSvgPath` contract survives** through the dynamic side-lane
  (`Register_DynamicIcon`/`Get_DynamicBrush`, with generated-semantic-name fallback);
  the hash pool is the generated `ck::icons::Get_GeneratedPool()` (47 decorative glyphs).
- **Deleted**: the 206-file corpus, all three `CreateIconBrushes` scans + `Get_IconBrush`
  accessors + `GeneralIconPool`, both NonUFS icon-staging blocks, the orphaned
  `Gamepad_Master.svg` (zero references), and `Trans_Node_Icon.png` (its one consumer,
  `SCkSmRuntimeGraph.cpp`, now draws `ECk_Icon::Tween`).
- **Spec tests** pin the new registry: tone glyphs resolve at both sizes and stay out of the
  decorative pool; every inspector icon resolves; toolbar rejects `None`; descriptors keep typed
  identities.

## Deviations from the Gate-B1 proposal (each judged at the site)

| Site | Ruling |
|---|---|
| Aggro `▶` active-row marker (`SCkAggroDebuggerWindow.cpp` SetText) | KEPT as text — a live `SetText` target; icon = widget restructure, not migration |
| Gallery `⚠`/`🧪` (`CkGallery_MissionControl.cpp`) | KEPT — `SCkDebug_AlertRow.Glyph` is a deliberate text-glyph API; changing it is a widget redesign (follow-up candidate) |
| GraphPane `◀`/`▶` name-length steppers | KEPT — they are ± steppers (the gate doc's own keep rule), not transport |
| `◀ NOW` timeline badge | KEPT — text badge, sized to font |
| Migrated as planned | picker `⚙` → `Settings`, GOAP `|◀`/`▶|` → `SkipBackward`/`SkipForward`, copy menu `FAppStyle` → `ECk_Icon::Copy` |
| `SGraphNode_SmTransition.cpp:71` engine `FAppStyle` transition icon | KEPT — that node deliberately mimics the engine state-machine look 1:1 (recorded; second follow-up candidate) |
| Icon-name notes | `TreasureMap` (map tab + save empty-state) had no manifest semantic → mapped to `Minimap`; Jolt launcher icon was `Cube` → now `Entity`; other picture-level mappings are one-line manifest amendments whenever wanted |

## Follow-up candidates (recorded, NOT done — each would be a behaviour change)

1. GOAP "RO" text badge → `ECk_Icon::Locked` (the tofu-padlock reason no longer applies).
2. `SCkDebug_AlertRow` gains an `Icon` argument alongside its text `Glyph`.
3. `SGraphNode_SmTransition` off `FAppStyle` if the editor-mimicry stance changes.
4. Register 12/20px brushes if the Style Lab `IconSize` axis should re-rasterize rather than scale.

## Verification state

- Compile: green at Stage 2 (`Result: Succeeded`, 0 errors, 5 error-driven rounds). Stage-3
  deletions compile in the FINAL gate (single `--build --test --test-pattern Debugger` run).
- Machine gate: the debugger spec suite (the surface this campaign touched). Full-suite run was
  deliberately skipped per the maintainer ("only build at the end"; the icon system cannot reach
  gameplay tests — inferred, not proven).
- **[EDITOR-VERIFY] — the human walk (campaign Phase 3, items 2/4/5/6):**
  1. `Ck.Icons.ShowTestWindow` → all 144 icons crisp at 16/24, tint responds (then delete
     `CkIconStyle_TestWindow.cpp` and revert `0cd736a1c`).
  2. Open the launcher: every tool button has its glyph at 24px, tab icon = bug glyph.
  3. Walk EVERY debugger tab (ECS incl. Dashboard/Archetypes/Overview + inspector rail, SM graph
     transition nodes, GOAP timeline transports, Save (toolbar, provenance/tree glyphs, diff
     panel), Optimization (findings, cleanup, profile shelves), Jolt draw toggles + viewport cube,
     Audio, Aggro, AStar, Crowd, Dialog, Eqs, Input, Intent, Insights toolbar, Map, ObjectPooling,
     Scheduler, StyleLab gallery, UI) — no blank icon slots, no black glyphs.
  4. Log stays free of `Unable to find Brush` and of `FCkIconStyle` ensures.
