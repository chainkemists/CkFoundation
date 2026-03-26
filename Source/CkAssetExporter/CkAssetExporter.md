# CkAssetExporter

Editor tool that exports Unreal assets (Behavior Trees, Blueprints, Data Assets, EQS queries) to JSON and plain-text files. Provides content browser right-click actions and a dedicated editor tab for batch exporting.

## Key Concepts

- **Dual Format** — Every export produces both a `.json` (structured, for tools) and a `.txt` (human-readable, indented tree view).
- **Supported Asset Types** — Behavior Trees, Blueprints, Data Assets, Environment Queries (EQS).
- **Content Browser Actions** — Right-click any supported asset to export it directly.
- **Exporter Tab** — Editor window (`Window > Asset Exporter`) with buttons per asset type, status bar, and results list.

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
