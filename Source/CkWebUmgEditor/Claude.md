# CkWebUmgEditor

**Purpose:** Editor-side of the HTML/CSS→UMG toolkit (webumg campaign, Gate 4 emission):
imports a `*.ckui.json` bundle (extracted by `Tools/ckwebumg-extract`) into a
`UCk_WebUmg_PageAsset_UE` content package. Nothing here ships in a cooked build.

**Type:** Editor. **Depends on:** CkCore, CkLog, CkWebUmg + UnrealEd/Projects.

## Key API

- `ck::webumg::editor::ImportPageAsset(JsonPath, PackageFolder, SaveToDisk)`
  (`CkWebUmg_Importer.h`) — the emission entry point:
  - MD5 source-hash stamp: re-importing an unchanged source is a **same-object no-op**
    (DECISION 3 idempotence, proven by `CkTests.UnitTests.CkWebUmg.ImportIdempotence`).
  - Changed sources overwrite wholesale through the atomic converter — a duplicate
    `data-ck-name` hard-fails (ensure) and leaves the existing asset untouched.
  - `ckui-assets/` textures import via `ImportFileAsTexture2D`, get `SRGB=true`, and are
    OUTERED to the asset so one package carries the whole page.
  - `SaveToDisk=false` imports in-memory only — tests never write Content.

## Anti-patterns

- Don't hand-edit imported assets — DECISION 3 makes generated output read-only; regeneration
  overwrites wholesale. Hand-authoring belongs in consumers of the asset.
- Don't bypass the converter to build assets field-by-field — emission is atomic by contract.
