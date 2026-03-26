# CkTemplate

Another copy-paste starter template for new ECS modules (similar to CkEcsTemplate). Follows the standard three-processor pattern.

## Key Concepts

- **Template Module** — Not a runtime feature. Copy and rename when creating a new ECS subsystem.

## Example: Copy to Create a New Module

```mermaid
flowchart LR
    A["Copy CkTemplate<br/>folder"] -->|"rename types"| B["New module ready"]
```

## Usage Examples

### Add feature to entity

```cpp
UCk_Utils_Template_UE::Add(Entity, Params);
```

## Tests

No tests found for this module in CkTest.
