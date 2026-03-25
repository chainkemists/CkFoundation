# StateTree Feature for ECS Framework

## Overview

Create a StateTree feature for the Venus ECS framework that allows entities to execute Unreal Engine StateTree assets without requiring Actors.

## Requirements

### Core Functionality
- Execute StateTree assets on entities without Actor ownership
- Use `FStateTreeInstanceData` + `FStateTreeExecutionContext` directly (no UStateTreeComponent wrapper)
- Processor-driven ticking (not component tick)
- One StateTree per entity
- Good defaults: user provides StateTree asset, everything else "just works"

### API Surface (Utils)
Expose via `UCk_Utils_StateTree_UE`:

**Lifecycle Control:**
- `Request_StartLogic` - Start executing the StateTree
- `Request_RestartLogic` - Stop and restart from beginning
- `Request_StopLogic` - Stop execution
- `Request_PauseLogic` - Pause execution
- `Request_ResumeLogic` - Resume paused execution

**State Queries:**
- `Get_IsRunning` - Is the StateTree currently executing?
- `Get_IsPaused` - Is execution paused?
- `Get_RunStatus` - Get current run status (Running/Stopped/Paused)

### Schema
Create `UCk_StateTree_Schema_Entity` that provides:
- Entity handle as context
- World reference
- Basic external data access pattern

StateTree assets using this schema can be executed on entities.

### NOT Included
- Replication support (explicitly excluded)
- Multiple StateTrees per entity
- Actor-based context data

## Technical Approach

### Direct StateTree Execution (Option B)
Skip `UStateTreeComponent` entirely. Instead:
1. Store `FStateTreeInstanceData` in fragment
2. Create temporary `FStateTreeExecutionContext` each tick
3. Processor handles Start/Tick/Stop lifecycle
4. Custom schema provides entity context

### Fragment Structure
- **Params**: StateTree asset reference, auto-start behavior
- **Current**: Instance data, execution state, owner object
- **Requests**: Variant of request types for lifecycle control

### Owner Object Pattern
`FStateTreeExecutionContext` requires a UObject owner for:
- Creating transient UObjects during execution
- Logging context

We'll create a minimal `UCk_StateTree_Owner` UObject per entity that:
- Has lifetime tied to the entity
- Provides world access
- Holds the instance data

## Reference Implementations
- IsmProxy feature: `CkIsmRenderer/Proxy/` - component-less UE integration
- Probe feature: Request struct patterns, signal binding

## Success Criteria
1. Can create StateTree asset with `UCk_StateTree_Schema_Entity`
2. Can add StateTree feature to entity with just asset reference
3. StateTree executes automatically on spawn (if configured)
4. Can control lifecycle via Request_ functions
5. Can query state via Get_ functions
6. StateTree tasks can access entity handle through context
