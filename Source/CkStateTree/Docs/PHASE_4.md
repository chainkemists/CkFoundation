# Phase 4: Utils API

## Objectives
1. Implement all `Request_` functions for lifecycle control
2. Implement all `Get_` functions for state queries
3. Ensure Blueprint/Angelscript compatibility

## Dependencies
- Phase 2 (Request structs)
- Phase 3 (Processors that handle requests)

## Tasks

### 4.1 Request Functions
**File:** `CkStateTree_Utils.h` / `CkStateTree_Utils.cpp`

```cpp
// Start executing the StateTree
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Request Start Logic")
static FCk_Handle_StateTree
Request_StartLogic(
    UPARAM(ref) FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_StartLogic& InRequest);

// Restart from beginning
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Request Restart Logic")
static FCk_Handle_StateTree
Request_RestartLogic(
    UPARAM(ref) FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_RestartLogic& InRequest);

// Stop execution
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Request Stop Logic")
static FCk_Handle_StateTree
Request_StopLogic(
    UPARAM(ref) FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_StopLogic& InRequest);

// Pause execution
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Request Pause Logic")
static FCk_Handle_StateTree
Request_PauseLogic(
    UPARAM(ref) FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_PauseLogic& InRequest);

// Resume paused execution
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Request Resume Logic")
static FCk_Handle_StateTree
Request_ResumeLogic(
    UPARAM(ref) FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_ResumeLogic& InRequest);
```

### 4.2 Query Functions
**File:** `CkStateTree_Utils.h` / `CkStateTree_Utils.cpp`

```cpp
// Is the StateTree currently running?
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Is Running")
static bool
Get_IsRunning(
    const FCk_Handle_StateTree& InHandle);

// Is execution paused?
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Is Paused")
static bool
Get_IsPaused(
    const FCk_Handle_StateTree& InHandle);

// Get current run status enum
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Get Run Status")
static ECk_StateTree_RunStatus
Get_RunStatus(
    const FCk_Handle_StateTree& InHandle);

// Get the StateTree asset reference
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|StateTree",
          DisplayName = "[Ck][StateTree] Get StateTree Asset")
static UStateTree*
Get_StateTree(
    const FCk_Handle_StateTree& InHandle);
```

### 4.3 Implementation Pattern
Each Request_ function follows standard pattern:

```cpp
auto UCk_Utils_StateTree_UE::Request_StartLogic(
    FCk_Handle_StateTree& InHandle,
    const FCk_Request_StateTree_StartLogic& InRequest)
    -> FCk_Handle_StateTree
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid handle passed to Request_StartLogic"))
    { return InHandle; }
    
    auto& Requests = InHandle.AddOrGet<ck::FFragment_StateTree_Requests>();
    Requests._Requests.Add(InRequest);
    
    return InHandle;
}
```

Each Get_ function reads from Current fragment:

```cpp
auto UCk_Utils_StateTree_UE::Get_IsRunning(
    const FCk_Handle_StateTree& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid handle passed to Get_IsRunning"))
    { return false; }
    
    const auto& Current = InHandle.Get<ck::FFragment_StateTree_Current>();
    return Current.Get_RunStatus() == ECk_StateTree_RunStatus::Running;
}
```

### 4.4 Remove Example Request
Remove the stub `Request_ExampleRequest` and `FCk_Request_StateTree_ExampleRequest` from:
- `CkStateTree_Fragment_Data.h`
- `CkStateTree_Utils.h` / `.cpp`

## Success Criteria
1. All Request_ functions queue appropriate requests
2. All Get_ functions return correct values
3. Functions work from C++, Blueprints, and Angelscript
4. No stub code remains

## Files to Modify
- **MODIFY:** `Public/CkStateTree/CkStateTree_Utils.h`
- **MODIFY:** `Public/CkStateTree/CkStateTree_Utils.cpp`
- **MODIFY:** `Public/CkStateTree/CkStateTree_Fragment_Data.h` (remove example)

## Notes
- Request functions return handle for chaining
- Get functions are BlueprintPure (no side effects)
- Follow existing Utils patterns exactly
