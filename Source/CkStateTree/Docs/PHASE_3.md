# Phase 3: Processor Implementation

## Objectives
1. Implement `FProcessor_StateTree_Setup` - Create owner, initialize instance data
2. Implement `FProcessor_StateTree_Tick` - Execute StateTree each frame
3. Implement `FProcessor_StateTree_HandleRequests` - Process lifecycle requests
4. Implement `FProcessor_StateTree_EndPlay` - Cleanup

## Dependencies
- Phase 1 (Schema, Owner)
- Phase 2 (Fragment structures)

## Tasks

### 3.1 FProcessor_StateTree_Setup
**File:** `CkStateTree_Processor.cpp`

Setup processor creates owner and initializes instance data:

```cpp
auto FProcessor_StateTree_Setup::ForEachEntity(
    TimeType InDeltaT,
    HandleType InHandle,
    const FFragment_StateTree_Params& InParams,
    FFragment_StateTree_Current& InCurrent) const -> void
{
    const auto& StateTree = InParams.Get_Params().Get_StateTree();
    
    CK_ENSURE_IF_NOT(ck::IsValid(StateTree), 
        TEXT("StateTree Entity [{}] has no StateTree asset assigned"), InHandle)
    { return; }
    
    // Create owner object
    auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    InCurrent._Owner = TStrongObjectPtr<UCk_StateTree_Owner>(
        UCk_StateTree_Owner::Create(World, InHandle));
    
    // Initialize instance data
    InCurrent._InstanceData.Init(*InCurrent._Owner.Get(), *StateTree, EStateTreeStorage::Internal);
    
    // Remove setup tag
    InHandle.Remove<FTag_StateTree_RequiresSetup>();
    
    // Auto-start if configured
    if (InParams.Get_Params().Get_AutoStart() == ECk_StateTree_AutoStart::OnSetup)
    {
        UCk_Utils_StateTree_UE::Request_StartLogic(InHandle, FCk_Request_StateTree_StartLogic{});
    }
}
```

### 3.2 FProcessor_StateTree_Tick (NEW)
**File:** `CkStateTree_Processor.h` / `CkStateTree_Processor.cpp`

New processor that ticks running StateTrees:

```cpp
// Header
class CKSTATETREE_API FProcessor_StateTree_Tick : public ck_exp::TProcessor<
    FProcessor_StateTree_Tick,
    FCk_Handle_StateTree,
    FFragment_StateTree_Params,
    FFragment_StateTree_Current,
    TExclude<FTag_StateTree_RequiresSetup>,
    CK_IGNORE_PENDING_KILL>
{
public:
    using TProcessor::TProcessor;

public:
    auto DoTick(TimeType InDeltaT) -> void;

    auto ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_StateTree_Params& InParams,
        FFragment_StateTree_Current& InCurrent) const -> void;
};
```

Implementation:

```cpp
auto FProcessor_StateTree_Tick::ForEachEntity(
    TimeType InDeltaT,
    HandleType InHandle,
    const FFragment_StateTree_Params& InParams,
    FFragment_StateTree_Current& InCurrent) const -> void
{
    // Only tick if running
    if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Running)
    { return; }
    
    const auto& StateTree = InParams.Get_Params().Get_StateTree();
    auto Owner = InCurrent._Owner.Get();
    
    CK_ENSURE_IF_NOT(ck::IsValid(Owner) && ck::IsValid(StateTree),
        TEXT("StateTree Entity [{}] has invalid owner or StateTree"), InHandle)
    { return; }
    
    // Create temporary execution context
    FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);
    
    if (NOT Owner->SetContextRequirements(Context))
    { return; }
    
    // Tick the state tree
    Context.Tick(InDeltaT);
}
```

### 3.3 FProcessor_StateTree_HandleRequests
**File:** `CkStateTree_Processor.cpp`

Process lifecycle control requests:

```cpp
auto FProcessor_StateTree_HandleRequests::ForEachEntity(...) const -> void
{
    InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_StateTree_Requests& InRequests)
    {
        algo::ForEachRequest(InRequests._Requests, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            
            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});
    });
}

// StartLogic handler
auto FProcessor_StateTree_HandleRequests::DoHandleRequest(
    HandleType InHandle,
    const FFragment_StateTree_Params& InParams,
    FFragment_StateTree_Current& InCurrent,
    const FCk_Request_StateTree_StartLogic& InRequest) -> void
{
    if (InCurrent.Get_RunStatus() == ECk_StateTree_RunStatus::Running)
    { return; } // Already running
    
    const auto& StateTree = InParams.Get_Params().Get_StateTree();
    auto Owner = InCurrent._Owner.Get();
    
    FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);
    
    if (NOT Owner->SetContextRequirements(Context))
    { return; }
    
    Context.Start();
    InCurrent._RunStatus = ECk_StateTree_RunStatus::Running;
}

// StopLogic handler
auto FProcessor_StateTree_HandleRequests::DoHandleRequest(
    HandleType InHandle,
    const FFragment_StateTree_Params& InParams,
    FFragment_StateTree_Current& InCurrent,
    const FCk_Request_StateTree_StopLogic& InRequest) -> void
{
    if (InCurrent.Get_RunStatus() == ECk_StateTree_RunStatus::Stopped)
    { return; }
    
    const auto& StateTree = InParams.Get_Params().Get_StateTree();
    auto Owner = InCurrent._Owner.Get();
    
    FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);
    
    if (Owner->SetContextRequirements(Context))
    { Context.Stop(); }
    
    InCurrent._RunStatus = ECk_StateTree_RunStatus::Stopped;
}

// RestartLogic = Stop + Start
// PauseLogic = set status to Paused
// ResumeLogic = set status to Running (if was Paused)
```

### 3.4 FProcessor_StateTree_EndPlay
**File:** `CkStateTree_Processor.cpp`

Cleanup on entity destruction:

```cpp
auto FProcessor_StateTree_EndPlay::ForEachEntity(
    TimeType InDeltaT,
    HandleType InHandle,
    const FFragment_StateTree_Params& InParams,
    FFragment_StateTree_Current& InCurrent) const -> void
{
    // Stop if running
    if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Stopped)
    {
        const auto& StateTree = InParams.Get_Params().Get_StateTree();
        if (auto Owner = InCurrent._Owner.Get(); ck::IsValid(Owner) && ck::IsValid(StateTree))
        {
            FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);
            if (Owner->SetContextRequirements(Context))
            { Context.Stop(); }
        }
    }
    
    // Clear owner reference (TStrongObjectPtr will release)
    InCurrent._Owner.Reset();
}
```

### 3.5 Register Tick Processor
**File:** `CkStateTree_ProcessorInjector.cpp`

Add tick processor to injector:

```cpp
// Ensure FProcessor_StateTree_Tick is registered with appropriate phase
```

## Success Criteria
1. Setup creates owner and initializes instance data
2. Tick processor executes StateTree each frame when running
3. All lifecycle requests work correctly
4. EndPlay properly cleans up
5. No memory leaks from Owner object

## Files to Modify
- **MODIFY:** `Public/CkStateTree/CkStateTree_Processor.h` - Add FProcessor_StateTree_Tick
- **MODIFY:** `Public/CkStateTree/CkStateTree_Processor.cpp` - Implement all processors
- **MODIFY:** `ProcessorInjector/CkStateTree_ProcessorInjector.cpp` - Register tick processor

## Notes
- `FStateTreeExecutionContext` is temporary - created fresh each tick
- Pause is implemented by simply not ticking (status check)
- Owner provides context requirements to execution context
- Tick should happen AFTER setup and requests are processed
