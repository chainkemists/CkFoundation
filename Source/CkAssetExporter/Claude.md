# CkAssetExporter

**Purpose:** Asset export utilities — exports CkFoundation asset data to external formats (JSON, etc.) for tooling, auditing, or external pipelines.

**Supported asset types:** Behavior Trees, Blueprints, Data Assets, EQS Queries, State Trees, Blueprint structs (`UUserDefinedStruct`), and Blueprint enums (`UUserDefinedEnum`). Each is exposed both as a Content Browser right-click action (`AssetAction/`) and a button in the batch Exporter Tab (`ExporterTab/`). State Trees walk the authored `UStateTreeEditorData` (states, tasks, conditions, transitions, global tasks/evaluators). Blueprint structs export their schema (each field's authored name, internal name, CPP type, category, tooltip) plus default values read from the struct's default instance — reusing `FCk_DataAssetExporter`'s shared property serializer. Blueprint enums export each authored enumerator's display name, internal name, fully-qualified name, numeric value, and tooltip (the auto-generated trailing `_MAX` is skipped via the `NumEnums()-1` idiom).

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
