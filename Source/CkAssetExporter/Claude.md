# CkAssetExporter

**Purpose:** Asset export utilities — exports CkFoundation asset data to external formats (JSON, etc.) for tooling, auditing, or external pipelines.

**Supported asset types:** Behavior Trees, Blueprints, Data Assets, EQS Queries, State Trees, Blueprint structs (`UUserDefinedStruct`), Blueprint enums (`UUserDefinedEnum`), **Niagara systems**, **Cascade particle systems**, **materials**, and **textures**. Most are exposed as Content Browser right-click actions (`AssetAction/`) and buttons in the batch Exporter Tab (`ExporterTab/`). State Trees walk the authored `UStateTreeEditorData` (states, tasks, conditions, transitions, global tasks/evaluators). Blueprint structs export their schema (each field's authored name, internal name, CPP type, category, tooltip) plus default values read from the struct's default instance — reusing `FCk_DataAssetExporter`'s shared property serializer. Blueprint enums export each authored enumerator's display name, internal name, fully-qualified name, numeric value, and tooltip (the auto-generated trailing `_MAX` is skipped via the `NumEnums()-1` idiom).

**VFX corpus pipeline (2026-07-12):** `FCk_NiagaraExporter` deconstructs a system into a JSON+text recipe — emitter module stacks with Rapid-Iteration constants, **override-pin harvesting** (dynamic inputs, curve DIs rasterized to keys via `UNiagaraDataInterfaceCurveBase::GetCurveData`, attribute links; nested dynamic-input overrides re-associated via `DoExpandDynamicInputOverrides`, orphans surfaced as `unattachedOverrides` — never silently dropped), enum display names, user-param values, determinism/bounds, renderer detail with material paths. `FCk_CascadeExporter` dumps legacy `UParticleSystem` LOD-0 modules generically via reflection with `UDistribution*` rasterization. `FCk_MaterialExporter` (blend/shading/params + expression histogram + `GetUsedTextures`) and `FCk_TextureExporter` (editor source → PNG capped at 1024 + metadata) chain off renderer material references; `FCk_MeshExporter` (LOD-0 render geometry → Wavefront OBJ + stats JSON with bounds/UV-ranges/sections; UE V-down UV convention noted in the OBJ header) chains off mesh-renderer `Meshes` references into `meshes/` — the carrier geometry (slash sweeps, shells, trails) VFX recreation needs. `FCk_VfxCorpusExporter` orchestrates a batch over asset-registry class sweeps into `Saved/CkVfxCorpus/` (`index.json` manifest; per-asset failures recorded, never skipped; output dirs mirror package folder chains so same-named assets can't collide). Entry points: automation test `Ck.AssetExporter.ExportVfxCorpus` (**gated on env `CK_VFX_CORPUS_EXPORT=1`** — instant skip otherwise; StressFilter is unusable because the automation commandline's "Standard" filter excludes it) and console command `Ck.AssetExporter.ExportVfxCorpus`. Gotchas that cost time once: `UNiagaraNodeParameterMapSet`/`NiagaraNodeParameterMapBase.h` are Private NiagaraEditor headers (detect override nodes by class name on `UEdGraphNode`); `UNiagaraNodeInput`'s accessors are MinimalAPI-unexported (read `DataInterface`/`ObjectAsset` UPROPERTYs via `FindFProperty`); the 5-arg `GetUsedTextures` overload is a UE_DEPRECATED(5.7) final no-op (use the TOptional overload).

**Headless export stack (2026-07-18):** the module is agent-drivable without the editor UI. Entry
point for humans/agents is `Source/CkScripts/Export-CkAssets.ps1` (standalone; resolves the
project's own editor binary; identical on the legacy 5.5 branch) — never hand-build the
commandlet command line. Components:

- `Commandlet/` — `UCkAssetExporterCommandlet` (`-run=CkAssetExporter`): `-Assets=` (object /
  package / disk paths, `;`-separated), `-Dir=` + `-Classes=` registry sweeps, `-List`
  (discovery → `LastList.json`), `-DumpGraph` (dependency graph → `graph.json`), `-SkipFresh` /
  `-Force`, `-ExportServer`. Exit code = manifest-backed Main return
  (`UseCommandletResultAsExitCode` — LaunchEngineLoop otherwise promotes any logged boot Error to
  exit 1). **`-Server` is a reserved engine switch** — never name a commandlet flag that.
- `Dispatch/` — class→exporter routing (most-derived-first; generic `UDataAsset` LAST so
  EQS/StateTree hit their structure-aware exporters), per-asset failure ROWS (never aborts),
  `Saved/CkAssetExporter/LastRun.json` manifest (`entries` array). All externally-consumed paths
  are `ConvertRelativePathToFull` — relative `ProjectSavedDir` forms don't resolve from a
  submitter's cwd.
- `ExportMeta/` — deterministic `_meta` (source .uasset MD5 + per-exporter version constant)
  stamped in every sibling `.json`; powers `-SkipFresh` (hash AND version must match). Bump the
  version constant whenever an exporter's output shape changes. NO timestamps in any export
  output (write-if-changed sweeps + clean diffs depend on it). The json is the ONLY sibling
  carrying `_meta` — `.txt`/`.csv`/WBP paste-artifact siblings carry none of their own, but all
  siblings of an asset are written in the SAME export call, so the json's `_meta` verdict covers
  every sibling. **Freshness banner (2026-07-19):** every human-readable summary `.txt` from the
  seven exporters (DataAsset/Blueprint/BehaviorTree/EQS/StateTree/UserDefinedEnum/
  UserDefinedStruct) now opens with a deterministic 3-line banner pointing at the sibling json's
  `_meta` as the freshness oracle. Deliberately NO version bump for this change — the banner is
  txt-only, txt was never freshness-gated (`-SkipFresh` only ever checked the json's `_meta`),
  and the committed json corpus is byte-unchanged.
- `Server/` — `FCk_AssetExporter_RequestProcessor` (shared core) + the `-ExportServer` loop:
  polls `Saved/CkAssetExporter/Requests/*.json` (`op: export | list | dumpGraph | quit`), writes
  `Results/<basename>.json`, advertises via `server.json` (pid, dirs, `busy`/`currentOp`/
  `lastActivityAt`). Idle self-quit 10min, wall cap 2h. Sub-second-old request files are deferred
  one poll (mid-write settle guard).
- `Bridge/` — `UCkAssetExporter_BridgeSubsystem`: the OPEN editor claims the same protocol
  (zero boots). Never activates under commandlets; defers to a live editor-named owner pid;
  `PreserveExisting` startup so a re-claim can't eat the triggering request.
- `AutoSidecar/` — on-save sidecar refresh for logic-bearing classes (toggle
  `ck.AssetExporter.AutoSidecarOnSave`); inert for commandlets/procedural saves/non-canonical
  save targets (autosaves must not stamp sidecars with unsaved states).
- `DataTableExporter/`, `GraphDump/` — sibling `.csv` + rows-json; full dependency graph with
  hard/soft deps and cascade/redirector hazard flags.
- `BlueprintExporter/CkWidgetPasteArtifacts` — WBP exports also emit `<Base>.hierarchy.copy.txt`
  (byte-identical to the UMG Designer's Ctrl+C clipboard text — paste-ready into another WBP;
  format stable 5.5↔5.7) and per-animation `.t3d.txt` reference dumps (NOT pasteable — the
  Designer has no paste-animation path). Neither carries the freshness banner by design — byte
  identity to the clipboard format forbids any prepended text; the sibling json's `_meta` is
  their freshness oracle instead (same all-siblings-one-export-call rule as `ExportMeta/` above).
  Committed under `Content/BusterBlock` in the legacy superproject (`BusterBlock_5-5`) as the WBP
  porting source of truth; stay machine-local/gitignored in current-repo consumers.

**Depends on:** `CkAi`, `CkCore`, `CkEcs`, `CkLog` (+ engine editor modules incl. UMGEditor).
**Used by:** Agent/pipeline tooling; superprojects via `Export-CkAssets.ps1` + their
`/export-assets` skill.

---

## Key API

- No `_Utils.h`. Drive via `Export-CkAssets.ps1` (preferred) or `-run=CkAssetExporter` directly;
  in-editor via Content Browser right-click / the Exporter Tab.

---

## Anti-patterns

- Don't invoke from game runtime — editor-only.
- Don't add timestamps or machine-relative paths to any export output (breaks determinism /
  external consumers).
- Don't force-kill a `busy` server (`server.json`) — its quit is queued; killing destroys
  another session's in-flight work.
- Don't name commandlet flags after reserved engine switches (`-Server`, `-Client`, ...).

---

## See also

- `CkAngelscriptGenerator/Claude.md` — related code generation tooling.
- Superproject digest `docs/digests/2026-07-18-agent-driven-uasset-export.html` (BusterBlock) —
  architecture, gates, and the porting workflow this stack serves.
