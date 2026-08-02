# CkAssetExporter

**Purpose:** Asset export utilities — exports CkFoundation asset data to external formats (JSON, etc.) for tooling, auditing, or external pipelines.

## The sidecar extension is `.ckexport`, and it is JSON (2026-08-01)

`<Asset>.ckexport` sits next to `<Asset>.uasset` and is **plain UTF-8 JSON** — Read/Grep it directly.
Every sidecar's `_meta.format` says so in its own first lines, so a reader who opens one cold needs
no doc.

**Why not `.json`.** `FAutoReimportManager::SetUpDirectoryMonitors` stamps every watched content
directory with `SetApplicableExtensions(GetAllFactoryExtensions())`, and json IS a registered import
format (`UReimportDataTableFactory`). A committed sidecar corpus therefore accumulates into the
monitor's pending set, parks its state machine on the *"N changes to source content files"* prompt,
and — while parked — re-stats **the entire outstanding set every frame**. Measured on BusterBlock's
1,178-file corpus: **18.8 fps, editor-wide, on every map**, with the game thread at 99.7% of a core
looking compute-bound while actually blocked in `FileExists`/`GetTimeStamp` through the NTFS +
Defender filter drivers. No UE-side profiler can see it (Insights attributes it to `Tick_Engine`
self-time; `SimpleTickObjects` → `FAutoReimportManager` carries no stat scope) — it took `xperf`
stack sampling to find. `FMatchRules::IsFileApplicable` rejects an unclaimed extension **before any
wildcard rule**, so an extension no factory claims is invisible to the monitor in every consuming
project with zero configuration. That is the whole mechanism.

Two constraints that make it work, both enforced by tests:
- **Terminal extension only.** `MatchExtensionString` reads the FINAL extension (`Strrchr`), so
  `Foo.export.json` still parses as json. Any replacement must be the last extension.
- **Must stay unclaimed.** `Ck.AssetExporter.Dispatch.SidecarExtensionUnclaimed` asserts no import
  factory claims it, with a control assertion that json *is* claimed so the test cannot pass
  vacuously. Changing the extension to something a factory claims silently reintroduces the 18 fps
  bug — that test is the only thing standing in the way.

Extensions live in one place: `ck::asset_exporter::extension::{Sidecar, SummaryText, Csv}`
(`ExportMeta/CkAssetExporter_ExportMeta.h`). Corpus-mode output (non-empty `InOutputDir` →
`Saved/CkVfxCorpus/`) deliberately keeps `.json`: it is outside `Content/`, never watched, never
committed.

**Startup guard.** `Validation/CkAssetExporter_AutoReimportGuard` runs at `OnPostEngineInit` and
warns — naming the exact `AutoReimportDirectorySettings` line — if any extension this module writes
under `Content/` is factory-claimed AND not excluded by the effective monitor. It rebuilds the
engine's own parse + same-or-parent dedup and asks `FMatchRules::IsFileApplicable`, rather than
approximating. `.csv` is deliberately NOT probed: a DataTable's csv sidecar is a legitimate
re-import source for that DataTable, so a monitor picking it up is correct behaviour.

**Every path is JSON-only.** `ExportAssets`/`SweepDirectory` and the seven per-type exporters take
`ECk_AssetExporter_SidecarFormats`, and every one of them defaults to `JsonOnly` — on-save hook,
right-click, Exporter Tab, commandlet, server/bridge alike. The sibling `.ckexport` is the single
artifact an export produces. The enum and the `ShouldWrite_SummaryText` gate remain so a caller can
opt a `.txt` back in programmatically. Rationale: the `.txt` summary was lossy AND gitignored in
consuming projects, so it was never shared and drifted — BB's committed `StoreDriver_BB_BP.txt`
still listed variables renamed months earlier. A human-readable view is rendered on demand from the
committed JSON instead, by `Source/CkScripts/Show-CkAssetExport.ps1` (full view ≈ what the `.txt`
carried, `-Outline` for a terse skeleton); it prints to stdout and writes nothing under `Content/`.
DataTable `.csv` and the WBP paste artifacts still emit on every path: they are consumed data
artifacts, not the drift-prone summary.

**Supported asset types:** Behavior Trees, Blueprints, Data Assets, EQS Queries, State Trees, Blueprint structs (`UUserDefinedStruct`), Blueprint enums (`UUserDefinedEnum`), **Niagara systems**, **Cascade particle systems**, **materials / material instances** (`UMaterialInterface` — json-only, dispatch-routed 2026-07-20), and **textures** (corpus-chained only — no standalone right-click/dispatch entry). Most are exposed as Content Browser right-click actions (`AssetAction/`) and buttons in the batch Exporter Tab (`ExporterTab/`). State Trees walk the authored `UStateTreeEditorData` (states, tasks, conditions, transitions, global tasks/evaluators). Blueprint structs export their schema (each field's authored name, internal name, CPP type, category, tooltip) plus default values read from the struct's default instance — reusing `FCk_DataAssetExporter`'s shared property serializer. Blueprint enums export each authored enumerator's display name, internal name, fully-qualified name, numeric value, and tooltip (the auto-generated trailing `_MAX` is skipped via the `NumEnums()-1` idiom).

**VFX corpus pipeline (2026-07-12):** `FCk_NiagaraExporter` deconstructs a system into a JSON recipe — emitter module stacks with Rapid-Iteration constants, **override-pin harvesting** (dynamic inputs, curve DIs rasterized to keys via `UNiagaraDataInterfaceCurveBase::GetCurveData`, attribute links; nested dynamic-input overrides re-associated via `DoExpandDynamicInputOverrides`, orphans surfaced as `unattachedOverrides` — never silently dropped), enum display names, user-param values, determinism/bounds, renderer detail with material paths. **Sidecar shape v3 (`version::Niagara = 3`, the first Niagara `_meta`):** a top-level `systemState` (the System Spawn/Update stacks plus a resolved `systemStateModule` — the emitter-level Loop rows are INERT while an emitter's Life Cycle Mode is System, so the authored cadence is readable ONLY here), a per-emitter `eventHandlers` array (source emitter/event, execution mode, spawn counts, and the handler's own module stack; **always emitted, empty when there are none**), and a per-emitter `lifetimeResolved` naming which source actually drives Initialize Particle's Lifetime pin — `source` is `direct` (Lifetime Mode = Direct Set), `minmax` (= Random), `override` (a stack override binds the selected pin), `NO_MODULE`, or `AMBIGUOUS` when the Lifetime Mode static switch is unreadable and the stores therefore cannot disambiguate. `inertValues`/`inertOverrides` carry the lifetime constants the selected mode does NOT read — the store keeps every value ever authored, so presence is not evidence. `FCk_CascadeExporter` dumps legacy `UParticleSystem` LOD-0 modules generically via reflection with `UDistribution*` rasterization. `FCk_MaterialExporter` (blend/shading/params + expression histogram + `GetUsedTextures`) and `FCk_TextureExporter` (editor source → PNG capped at 1024 + metadata) chain off renderer material references; `FCk_MaterialExporter` is **dual-mode** (2026-07-20): a set `InOutputDir` writes into the corpus tree (as before), an empty default writes the sibling `<Asset>.json` next to the `.uasset` and stamps `_meta` (`version::Material`), so it is now a first-class dispatch/right-click/tab type. It is json-only — no `.txt` sibling, so it carries no freshness banner; the sibling json's `_meta` is its freshness oracle. `FCk_MeshExporter` (LOD-0 render geometry → Wavefront OBJ + stats JSON with bounds/UV-ranges/sections; UE V-down UV convention noted in the OBJ header) chains off mesh-renderer `Meshes` references into `meshes/` — the carrier geometry (slash sweeps, shells, trails) VFX recreation needs. `FCk_VfxCorpusExporter` orchestrates a batch over asset-registry class sweeps into `Saved/CkVfxCorpus/` (`index.json` manifest; per-asset failures recorded, never skipped; output dirs mirror package folder chains so same-named assets can't collide). Entry points: automation test `Ck.AssetExporter.ExportVfxCorpus` (**gated on env `CK_VFX_CORPUS_EXPORT=1`** — instant skip otherwise; StressFilter is unusable because the automation commandline's "Standard" filter excludes it) and console command `Ck.AssetExporter.ExportVfxCorpus`. Gotchas that cost time once: `UNiagaraNodeParameterMapSet`/`NiagaraNodeParameterMapBase.h` are Private NiagaraEditor headers (detect override nodes by class name on `UEdGraphNode`); `UNiagaraNodeInput`'s accessors are MinimalAPI-unexported (read `DataInterface`/`ObjectAsset` UPROPERTYs via `FindFProperty`); the 5-arg `GetUsedTextures` overload is a UE_DEPRECATED(5.7) final no-op (use the TOptional overload).

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
- `ExportMeta/` — deterministic `_meta` (`format` self-description + source .uasset MD5 +
  per-exporter version constant)
  stamped in every sibling `.ckexport`; powers `-SkipFresh` (hash AND version must match). Bump the
  version constant whenever an exporter's output shape changes (`version::Blueprint = 2` — bumped
  for the WBP paste artifacts: `hierarchy.copy.txt` + per-animation t3d dumps). Only the
  freshness-gated exporters skip on a match — Niagara stamps `version::Niagara` but is deliberately
  NOT gated (its dispatch branch never asks `Is_FreshAndSkip`), and Cascade stamps no `_meta` at
  all, so both always re-export. NO timestamps in any export
  output (write-if-changed sweeps + clean diffs depend on it). The json is the ONLY sibling
  carrying `_meta` — `.csv`/WBP paste-artifact siblings carry none of their own, but all
  siblings of an asset are written in the SAME export call, so the json's `_meta` verdict covers
  every sibling. It also covers the rendered text view, which is derived from the json per
  invocation and therefore cannot be staler than it. **Freshness banner:** the summary `.txt` from
  the seven exporters (DataAsset/Blueprint/BehaviorTree/EQS/StateTree/UserDefinedEnum/
  UserDefinedStruct) — written only when a caller explicitly asks for `JsonAndText` — opens with a
  deterministic 3-line banner pointing at the sibling json's `_meta` as the freshness oracle. The
  banner carries no version bump: it is txt-only, txt was never freshness-gated (`-SkipFresh` only
  ever checked the json's `_meta`), and the committed json corpus is byte-unchanged.
- `Server/` — `FCk_AssetExporter_RequestProcessor` (shared core) + the `-ExportServer` loop:
  polls `Saved/CkAssetExporter/Requests/*.json` (`op: export | list | dumpGraph | quit`), writes
  `Results/<basename>.json`, advertises via `server.json` (pid, dirs, `busy`/`currentOp`/
  `lastActivityAt`). Idle self-quit 10min, wall cap 2h. Sub-second-old request files are deferred
  one poll (mid-write settle guard).
- `Bridge/` — `UCkAssetExporter_BridgeSubsystem`: the OPEN editor claims the same protocol
  (zero boots). Never activates under commandlets; defers to a live editor-named owner pid;
  `PreserveExisting` startup so a re-claim can't eat the triggering request. A `quit` op (e.g.
  `-StopServer`) releases the claim WITHOUT exiting the editor, and the bridge re-claims only once
  new requests arrive — clean handoff, no re-grab in the idle gap. Unlike the `-ExportServer` loop
  it has NO idle / wall-clock watchdogs: the editor's lifetime is the user's business.
- `AutoSidecar/` — on-save sidecar refresh for logic-bearing classes (toggle
  `ck.AssetExporter.AutoSidecarOnSave`); inert for commandlets/procedural saves/non-canonical
  save targets (autosaves must not stamp sidecars with unsaved states). This is what keeps
  committed sidecars truthful MECHANICALLY rather than by convention: every editor save of a
  logic-bearing asset re-exports its sibling `.ckexport` in place (JSON only — see the extension
  section above), and because output is deterministic + write-if-changed, saving an unchanged asset
  produces zero diff noise.
- `DataTableExporter/`, `GraphDump/` — sibling `.csv` + rows-json; full dependency graph with
  hard/soft deps and cascade/redirector hazard flags. `GraphDump/` enumerates with NO class filter
  (migration-closure planning must see art and redirectors too) and records dependencies pointing
  OUTSIDE the root — those are the closure frontier. Output is deterministic: no timestamps, assets
  sorted by object path, dep arrays sorted lexically.
- `BlueprintExporter/CkWidgetPasteArtifacts` — WBP exports also emit `<Base>.hierarchy.copy.txt`
  (byte-identical to the UMG Designer's Ctrl+C clipboard text — paste-ready into another WBP;
  format stable 5.5↔5.7) and per-animation `.t3d.txt` reference dumps (NOT pasteable — the
  Designer has no paste-animation path). Neither carries the freshness banner by design — byte
  identity to the clipboard format forbids any prepended text; the sibling json's `_meta` is
  their freshness oracle instead (same all-siblings-one-export-call rule as `ExportMeta/` above).
  Stale `<Base>.animation.*.t3d.txt` files from renamed or removed animations are deleted before the
  current set is written, so the sibling set always mirrors the asset.
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

## Implementation notes

- **Niagara "recipe numbers" live in the Rapid Iteration Parameter store**, not the graph: sizes,
  colors, counts, lifetimes sit in each stage script's RI store keyed
  `Constants.[Emitter].[Module].[Input]`. The module graph pins expose only the static-switch
  selectors — reading pins alone loses every authored constant.
- **The exported material recipe** is base properties (domain, blend mode, shading model, two-sided,
  connected outputs) + effective parameter values resolved through the instance chain + an
  expression histogram of the base graph. The histogram IS the "material tricks" list (DepthFade,
  Fresnel, Panner, DynamicParameter, SubUV) a faithful recreation needs.
- **VFX corpus layout** (`CkVfxCorpusExporter.h`): `<CorpusRoot>/index.json` manifest;
  `systems/<Pack>/<Name>.json`; `materials/<Pack>/<Name>.json`; `textures/<Pack>/<Name>.png|json`;
  `meshes/<Pack>/<Name>.obj|json` (the VFX carrier meshes referenced by mesh renderers). Discovery is
  by asset class via the asset registry — pack naming conventions are unreliable. Referenced
  materials/textures/meshes are exported once, deduplicated across systems.
- `FCk_NiagaraExporter::ExportNiagaraSystem` / `FCk_MeshExporter::ExportStaticMesh` take an optional
  `InOutputDir` (same dual-mode shape as `FCk_MaterialExporter`): empty — the default — writes the
  siblings next to the `.uasset`; set writes them into that directory, the mode the corpus
  orchestrator drives.

---

## See also

- `CkAngelscriptGenerator/Claude.md` — related code generation tooling.
- Superproject digest `docs/digests/2026-07-18-agent-driven-uasset-export.html` (BusterBlock) —
  architecture, gates, and the porting workflow this stack serves.