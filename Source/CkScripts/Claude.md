# CkScripts

**Purpose:** Standalone maintenance and tooling scripts that ship with the plugin. **Not a module**
— there is no `Build.cs` here and nothing in this directory is compiled or loaded by the engine.
Each script is self-contained and invoked by a human or an agent from a shell.

(For AngelScript sources and the script host, see `Script/CLAUDE.md` at the plugin root and
`CkEcs/Claude.md` — neither lives here.)

---

## Contents

| Script | What it does |
|---|---|
| `Export-CkAssets.ps1` | Headless `.uasset` → sibling `.ckexport` exporter wrapper. Resolves the project's own editor binary and routes through an open editor's bridge, a kept-alive export server, or a one-shot boot. Canonical doc: the consuming project's `/export-assets` skill + `CkAssetExporter/CLAUDE.md`. |
| `Show-CkAssetExport.ps1` | Renders a compact human-readable view of a sidecar on demand (`-Outline` for a terse skeleton, `-Stats` for the byte ratio). Reads `.ckexport` and the legacy 5.5 projects' `.json` sidecars alike; prints to stdout and writes nothing under `Content/`. |
| `Generate-CkIcons.ps1` | Deterministic generator for the typed icon pipeline: validates `Resources/Icons/CkIcons_Manifest.json` against the pinned MDI index + vendored SVGs (fails loudly on any typo), emits `CkEditorTools/Style/CkIcons_Generated.{h,cpp}`. `-Check` verifies committed output matches the manifest (exit 3 on drift). |
| `Import-MdiIcon.ps1` | Vendors one MDI icon at the pinned version into `Resources/Icons/Mdi/` (validates name, downloads, applies the white-fill recolour). Manifest entry + regeneration stay manual. |
| `CkLfsLocks.py` / `CkLfsLocks.exe` | Git LFS lock inspection and management. |
| `CkEcsTemplateReplacer.ps1` / `.py` | **STALE.** Scaffolds a module from the CkEcsTemplate copy, which was deleted (`ad045415b`). Scaffold by copying the smallest complete feature quartet (`CkTimer`) instead. |
| `CONTINUATION_PROMPT_DerivedTxtView.md` | Task handoff doc; slated for deletion. |

---

## Anti-patterns

- Don't add a `Build.cs` or engine-loaded code here — a script that needs to run inside the editor
  belongs in the owning module (`CkAssetExporter/Commandlet`, `CkAssetExporter/Server`).
- Don't hand-build the export commandlet command line; call `Export-CkAssets.ps1`.
- Don't extend `CkEcsTemplateReplacer` — it targets a scaffold that no longer exists.
