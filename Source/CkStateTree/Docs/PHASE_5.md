# Phase 5: Testing and Polish

## Objectives
1. Create test StateTree asset with entity schema
2. Verify all lifecycle operations
3. Remove replication code (not supported)
4. Clean up any remaining stubs

## Dependencies
- All previous phases complete

## Tasks

### 5.1 Remove Replication Support
**File:** `CkStateTree_Fragment.h` / `CkStateTree_Processor.h`

Remove:
- `UCk_Fragment_StateTree_Rep` class
- `FProcessor_StateTree_Replicate` class
- `FTag_StateTree_Updated` tag
- Replication parameter from `Add()` function

Update `UCk_Utils_StateTree_UE::Add`:
```cpp
// Remove ECk_Replication parameter
static FCk_Handle_StateTree
Add(
    UPARAM(ref) FCk_Handle& InHandle,
    const FCk_Fragment_StateTree_ParamsData& InParams);
```

### 5.2 Clean Up Processor Injector
**File:** `ProcessorInjector/CkStateTree_ProcessorInjector.cpp`

- Ensure all processors are properly registered
- Remove any replicate processor registration
- Verify tick order: Setup → HandleRequests → Tick → EndPlay

### 5.3 Verify Module Dependencies
**File:** `CkStateTree.Build.cs`

Ensure all required modules are listed:
```cpp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "GameplayTags",
    "StateTreeModule",  // Required for StateTree types
    
    "CkCore",
    "CkEcs",
    "CkEcsExt",
    "CkLabel",
    "CkLog",
    "CkProvider",
    "CkRecord",
    "CkSettings",
});
```

### 5.4 Documentation Header
**File:** Each new header file

Add standard copyright and description:
```cpp
// Copyright Saad Taame. All Rights Reserved.

#pragma once

// [Description of file contents]
```

### 5.5 Manual Testing Checklist

1. **Schema Test:**
   - Create new StateTree asset
   - Select "CK Entity StateTree" schema
   - Verify entity context appears in editor

2. **Basic Execution:**
   - Create entity with StateTree feature
   - Assign StateTree with simple task
   - Verify auto-start works
   - Verify StateTree ticks

3. **Lifecycle Control:**
   - Test `Request_StartLogic` on stopped tree
   - Test `Request_StopLogic` on running tree
   - Test `Request_PauseLogic` / `Request_ResumeLogic`
   - Test `Request_RestartLogic`

4. **State Queries:**
   - Verify `Get_IsRunning` returns correct values
   - Verify `Get_IsPaused` returns correct values
   - Verify `Get_RunStatus` matches actual state

5. **Cleanup:**
   - Destroy entity while StateTree running
   - Verify no crashes or leaks
   - Check logs for errors

### 5.6 Update Existing Condition
**File:** `CkStateTree_Condition.h`

The existing `UCk_StateTree_Condition` base class is fine - just ensure it works with the new schema.

## Success Criteria
1. No replication code remains
2. All tests pass manually
3. No compiler warnings
4. Clean build

## Files to Modify
- **MODIFY:** `Public/CkStateTree/CkStateTree_Fragment.h` - Remove rep class
- **MODIFY:** `Public/CkStateTree/CkStateTree_Processor.h` - Remove rep processor
- **MODIFY:** `Public/CkStateTree/CkStateTree_Utils.h` - Remove rep param
- **MODIFY:** `ProcessorInjector/CkStateTree_ProcessorInjector.cpp`
- **VERIFY:** `CkStateTree.Build.cs`

## Notes
- Replication may be added later as separate feature
- Keep the feature minimal and focused
- Test in PIE and standalone
