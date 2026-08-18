# Icon Pipeline Campaign — PROGRESS

## Phase A — pipeline into CkFoundation

- **Gate 1 (2026-08-17): PASSED** — see `Gate_1_Discovery_Report.md`. Maintainer decisions:
  **MDI as specified** (over the recommended house-corpus option — the visual-language and
  licensing tradeoffs were surfaced and accepted), **hosted in CkEditorTools**. Sizes corrected
  to 16/24 (constraint said 16/20; 24 is what the launcher actually uses), vendored via pinned
  GitHub tag (no npm in this repo).
- **Phase 1 (2026-08-17): BUILT** — vendored MDI v7.4.47 subset (15 icons, recoloured
  `fill="#FFFFFF"`, LICENSE + NOTICE.md + VERSION.json + mdi-names.txt oracle), manifest,
  deterministic PowerShell generator (+`-Check` drift mode, + `Import-MdiIcon.ps1`), generated
  `ECk_Icon` + table, `FCkIconStyle` registry in CkEditorTools (16/24 brushes, startup/shutdown
  symmetric, NonUFS staging).
- **Phase 2 (2026-08-17): verified except the human visual check** —
  - full build Succeeded ×2 (fresh 573-action + incremental);
  - generator deterministic (byte-identical across delete+regenerate ×2);
  - bogus manifest entry fails loudly (exit 1, names the entry and the oracle);
  - fresh headless editor boot: 0 "Unable to find Brush/Style", 0 ensures, 0 AS errors, clean
    shutdown, dependent test (`Ck.DebuggerCommon.Axes.ToneIcons`) 1/1 passed.
  - **PENDING [EDITOR-VERIFY]**: run `Ck.Icons.ShowTestWindow` in-editor, confirm all 15 icons
    render crisply at 16/24 and tint (black glyph = recolour failure; blank = missing SVG).
    Then DELETE `Source/CkEditorTools/Public/CkEditorTools/Style/CkIconStyle_TestWindow.cpp`.
- **COMMITTED 2026-08-17 on `feature/icon-pipeline` (branched off `dev`), unpushed**:
  `516826365` (pipeline) + `fcf192733` (TEMPORARY test window — revert after visual check) +
  `68c5358fc` (CkScripts doc). Nothing in CkGameplayDebugger touched. The CkFoundation checkout
  is parked on this branch for the visual check; `git switch feature/save-load-optimization`
  restores the save-load world.

## Phase B — migrate CkGameplayDebugger, delete old mechanism

- **Phase 0 audit DONE (2026-08-17), Gate 1 DELIVERED — awaiting maintainer approval.**
  `Gate_B1_Audit_And_Mapping.md` + appendices (`phaseB-icon-ids.txt`, `phaseB-unicode.txt`).
  Headline numbers: 206 corpus icons, 134 referenced by name (57 root + 77 pool), 71 pool-only;
  47 inspector overrides; 7 id-mapper functions; 72 unicode markers (≈12 migrate, rest keep);
  full MDI mapping proposed and validated against the pinned index.
- Open Gate-1 decisions: mapping amendments, View* glyphs, `IconSvgPath` dynamic lane vs enum,
  pool strategy, unicode split. NO CODE until approved.
- **Phase 1 MIGRATED + Phase 2 DELETED (2026-08-17)** — proceeding on the maintainer's repeated
  "continue" (recommended Gate-1 design; every glyph choice remains a one-line manifest amendment).
  CkFoundation `feature/icon-pipeline` REBASED onto origin/dev (backup: `backup/icon-pipeline-pre-rebase`;
  stale local `dev` lacked CkDebugScene). CkGameplayDebugger `feature/icon-pipeline` off dev:
  `26302a7` (full typed migration — 5 compile-driven rounds to zero errors) + `a4e75fb` (deletion:
  206 SVGs + 3 registries + staging + orphans Gamepad_Master.svg/Trans_Node_Icon.png; docs).
  CkTests `feature/icon-pipeline` off origin/dev: `8a2b84a5` (archetype-validation icon path).
  Deviations from the Gate-1 unicode list, each judged at the site: Aggro `▶` kept (live SetText
  target), gallery `⚠`/`🧪` kept (SCkDebug_AlertRow's deliberate text-Glyph API), GraphPane `◀▶`
  kept (they are ± steppers), `◀ NOW` kept (text badge); migrated: picker gear, GOAP timeline
  `|◀`/`▶|`, copy menu (FAppStyle → typed). Follow-up candidates recorded: GOAP "RO" lock badge
  could become ECk_Icon::Locked; SGraphNode_SmTransition still mimics engine art via FAppStyle.
- **Phase 3 gate IN FLIGHT**: full build + full test suite. Then the human visual walk
  ([EDITOR-VERIFY]) across every debugger tab.
