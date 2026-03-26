# CkResourceLoader

Async and synchronous asset loading through ECS requests. Queue single or batch loads, get a callback when assets are ready.

## Key Concepts

- **Async Loading** — Uses Unreal's `StreamableManager` for non-blocking asset loads.
- **Batch Loading** — Load multiple assets atomically; callback fires when all complete.
- **Signals** — `OnObjectLoaded` and `OnObjectBatchLoaded` delegates.

## Example: Loading UI Assets on Demand

```mermaid
flowchart LR
    A["Player enters zone"] -->|"Request_LoadObjectBatch"| B["5 UI assets<br/>loading async"]
    B -->|"OnObjectBatchLoaded"| C["All assets ready<br/>show UI"]
```

## Usage Examples

### Load a single asset

```cpp
UCk_Utils_ResourceLoader_UE::Request_LoadObject(LoaderEntity, LoadRequest, OnLoadedDelegate);
```

### Load a batch

```cpp
UCk_Utils_ResourceLoader_UE::Request_LoadObjectBatch(LoaderEntity, BatchRequest, OnBatchLoadedDelegate);
```

## Tests

No tests found for this module in CkTest.
