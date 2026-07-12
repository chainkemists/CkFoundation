# CkAssetExporter

**Purpose:** Asset export utilities — exports CkFoundation asset data to external formats (JSON, etc.) for tooling, auditing, or external pipelines.

**Supported asset types:** Behavior Trees, Blueprints, Data Assets, EQS Queries, State Trees, Blueprint structs (`UUserDefinedStruct`), Blueprint enums (`UUserDefinedEnum`), **Niagara systems**, **Cascade particle systems**, **materials**, and **textures**. Most are exposed as Content Browser right-click actions (`AssetAction/`) and buttons in the batch Exporter Tab (`ExporterTab/`). State Trees walk the authored `UStateTreeEditorData` (states, tasks, conditions, transitions, global tasks/evaluators). Blueprint structs export their schema (each field's authored name, internal name, CPP type, category, tooltip) plus default values read from the struct's default instance — reusing `FCk_DataAssetExporter`'s shared property serializer. Blueprint enums export each authored enumerator's display name, internal name, fully-qualified name, numeric value, and tooltip (the auto-generated trailing `_MAX` is skipped via the `NumEnums()-1` idiom).

**VFX corpus pipeline (2026-07-12):** `FCk_NiagaraExporter` deconstructs a system into a JSON+text recipe — emitter module stacks with Rapid-Iteration constants, **override-pin harvesting** (dynamic inputs, curve DIs rasterized to keys via `UNiagaraDataInterfaceCurveBase::GetCurveData`, attribute links; nested dynamic-input overrides re-associated via `DoExpandDynamicInputOverrides`, orphans surfaced as `unattachedOverrides` — never silently dropped), enum display names, user-param values, determinism/bounds, renderer detail with material paths. `FCk_CascadeExporter` dumps legacy `UParticleSystem` LOD-0 modules generically via reflection with `UDistribution*` rasterization. `FCk_MaterialExporter` (blend/shading/params + expression histogram + `GetUsedTextures`) and `FCk_TextureExporter` (editor source → PNG capped at 1024 + metadata) chain off renderer material references; `FCk_MeshExporter` (LOD-0 render geometry → Wavefront OBJ + stats JSON with bounds/UV-ranges/sections; UE V-down UV convention noted in the OBJ header) chains off mesh-renderer `Meshes` references into `meshes/` — the carrier geometry (slash sweeps, shells, trails) VFX recreation needs. `FCk_VfxCorpusExporter` orchestrates a batch over asset-registry class sweeps into `Saved/CkVfxCorpus/` (`index.json` manifest; per-asset failures recorded, never skipped; output dirs mirror package folder chains so same-named assets can't collide). Entry points: automation test `Ck.AssetExporter.ExportVfxCorpus` (**gated on env `CK_VFX_CORPUS_EXPORT=1`** — instant skip otherwise; StressFilter is unusable because the automation commandline's "Standard" filter excludes it) and console command `Ck.AssetExporter.ExportVfxCorpus`. Gotchas that cost time once: `UNiagaraNodeParameterMapSet`/`NiagaraNodeParameterMapBase.h` are Private NiagaraEditor headers (detect override nodes by class name on `UEdGraphNode`); `UNiagaraNodeInput`'s accessors are MinimalAPI-unexported (read `DataInterface`/`ObjectAsset` UPROPERTYs via `FindFProperty`); the 5-arg `GetUsedTextures` overload is a UE_DEPRECATED(5.7) final no-op (use the TOptional overload).

**Depends on:** `CkAi`, `CkCore`, `CkEcs`, `CkLog`.
**Used by:** Pipeline tooling only.

---

## Key API

- No `_Utils.h`. Invoked via editor commands.

---

## Pattern

Run via editor utility or commandlet.

---

## Anti-patterns

Don't invoke from game runtime — editor-only.

---

## See also

- `CkAngelscriptGenerator/Claude.md` — related code generation tooling.
