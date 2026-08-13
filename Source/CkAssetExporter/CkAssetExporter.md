# CkAssetExporter

Editor tool that exports Unreal assets (Behavior Trees, Blueprints, Data Assets, EQS queries) to JSON and plain-text files. Provides content browser right-click actions and a dedicated editor tab for batch exporting.

## Key Concepts

- **Dual Format** — Every export produces both a `.json` (structured, for tools) and a `.txt` (human-readable, indented tree view).
- **Supported Asset Types** — Behavior Trees, Blueprints, Data Assets, Environment Queries (EQS), State Trees, Blueprint structs (`UUserDefinedStruct`), Blueprint enums (`UUserDefinedEnum`), and Materials / Material Instances (`UMaterialInterface` — json-only). Niagara/Cascade particle systems export too (via the VFX-corpus tooling).
- **Content Browser Actions** — Right-click any supported asset to export it directly.
- **Exporter Tab** — Editor window (`Window > Asset Exporter`) with buttons per asset type, status bar, and results list.

## Material graph export (`version::Material` 3)

A material sidecar carries the authored node graph under `"graph"`, not just the parameter values:

- `graph.nodes[]` — one entry per expression: `id` (its index in the package's expression array, and the key
  every reference below uses), `class` (the `MaterialExpression` prefix stripped), `name`, optional `desc`
  (the node's authored comment).
- `nodes[].inputs[]` — **connected pins only**: `index`, `name`, the source `node` + `output` index, and a
  `mask` channel string when the pin is component-masked. An absent input is an unconnected pin, whose value
  comes from the node's own constant in `props`.
- `nodes[].outputs[]` — output pin names, emitted when a node has more than one, so an input's `output`
  index reads as a pin rather than an ordinal.
- `nodes[].props` — properties that differ from the class default, which is what keeps this to the fields the
  author actually touched (`Constant.R`, `ScalarParameter.DefaultValue`, `Custom.Code`, `ComponentMask.R/G/B/A`).
  A property pointing at another expression — `NamedRerouteUsage.Declaration` above all — resolves to
  `{ "node": <id> }`, because that edge never appears in the input iterator.
- `graph.outputs[]` — which node feeds each material output pin (`MP_EmissiveColor`, …). The older
  `connectedOutputs` array is still written: it answers "is Emissive wired", this answers "wired to what".

Structs are exported only from an allow-list of value types (`LinearColor`, `Vector`, …). That is deliberate:
`FExpressionInput` is a struct too, and walking one re-descends the whole graph through its `Expression`
pointer — connectivity has its own representation and must not also arrive as nested property soup.

The graph is `WITH_EDITOR`, a stricter gate than the `WITH_EDITORONLY_DATA` guarding the rest of the block:
`GetExpressionInputForProperty` and `FExpressionInputIterator` are both editor-only.

### Material functions (`version::MaterialFunction` 1)

`UMaterialFunction` is **not** a `UMaterialInterface` — no parameters, no blend mode, no material output
pins — so it routes to `ExportMaterialFunction` rather than a branch inside `ExportMaterial`. It emits the
**same `graph.nodes` shape**, which is the point: a `MaterialFunctionCall` node in some other material's
graph names its function's asset path in `props.MaterialFunction`, and that path now resolves to a sidecar
with the function's own graph instead of being a dead end.

Two differences from a material export:

- **No `graph.outputs`.** A function's outputs are `FunctionOutput` *nodes* in `nodes`, not fixed pins, and
  its inputs are `FunctionInput` nodes — so the signature is already in the node list, with `InputName` /
  `OutputName` / `SortPriority` arriving as ordinary non-default `props`. `Build_Graph` takes a null
  outputs-owner to express this.
- **No render-capability gate.** `ExportMaterial` refuses in a render-incapable process because
  `GetUsedTextures` walks compiled `FMaterialResource`s that don't exist there. A function has no compiled
  resource to walk and no `usedTextures` field, so the refusal doesn't apply — functions export fine from a
  commandlet. Textures a function samples still surface as object paths in its nodes' `props`.

## Example: Exporting a Behavior Tree for Review

```mermaid
flowchart LR
    A["Right-click BT asset<br/>in Content Browser"] -->|"Export Behavior Tree"| B["Serializes tree<br/>hierarchy + properties"]
    B --> C["BT_Enemy.json<br/>BT_Enemy.txt<br/>saved to disk"]
```

## Usage Examples

### Export a single behavior tree

```cpp
auto Result = FCk_BehaviorTreeExporter::ExportBehaviorTree(MyBehaviorTree);
// Result.bSuccess, Result.JsonFilePath, Result.TextFilePath
```

### Batch export blueprints

```cpp
auto Results = FCk_BlueprintExporter::ExportBlueprints(BlueprintArray);
```

### Export a data asset

```cpp
auto Result = FCk_DataAssetExporter::ExportDataAsset(MyDataAsset);
```

### Export an EQS query

```cpp
auto Result = FCk_EQSExporter::ExportEQS(MyEnvQuery);
```

## Tests

No tests found for this module in CkTest.
