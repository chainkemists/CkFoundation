# Phase 2: Fragment Data Structures

## Objectives
1. Define `FCk_Fragment_StateTree_ParamsData` with all configuration
2. Define runtime state in `FFragment_StateTree_Current`
3. Define request types for lifecycle control
4. Update `FFragment_StateTree_Requests` variant

## Dependencies
- Phase 1 (Schema and Owner must exist)

## Tasks

### 2.1 Update FCk_Fragment_StateTree_ParamsData
**File:** `CkStateTree_Fragment_Data.h`

```cpp
UENUM(BlueprintType)
enum class ECk_StateTree_AutoStart : uint8
{
    Disabled,       // Must call Request_StartLogic manually
    OnSetup         // Auto-start when feature is setup (default)
};

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Fragment_StateTree_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_StateTree_ParamsData);

private:
    // Required: The StateTree asset to execute
    UPROPERTY(EditAnywhere, BlueprintReadWrite, 
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStateTree> _StateTree;

    // Auto-start behavior
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_StateTree_AutoStart _AutoStart = ECk_StateTree_AutoStart::OnSetup;

public:
    CK_PROPERTY_GET(_StateTree);
    CK_PROPERTY(_AutoStart);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_StateTree_ParamsData, _StateTree);
};
```

### 2.2 Update FFragment_StateTree_Current
**File:** `CkStateTree_Fragment.h`

```cpp
UENUM(BlueprintType)
enum class ECk_StateTree_RunStatus : uint8
{
    Stopped,
    Running,
    Paused
};

struct CKSTATETREE_API FFragment_StateTree_Current
{
    CK_GENERATED_BODY(FFragment_StateTree_Current);

    friend class FProcessor_StateTree_Setup;
    friend class FProcessor_StateTree_Tick;
    friend class FProcessor_StateTree_HandleRequests;
    friend class FProcessor_StateTree_EndPlay;
    friend class UCk_Utils_StateTree_UE;

private:
    // Owner object for StateTree execution
    TStrongObjectPtr<UCk_StateTree_Owner> _Owner;
    
    // Runtime instance data
    FStateTreeInstanceData _InstanceData;
    
    // Current execution status
    ECk_StateTree_RunStatus _RunStatus = ECk_StateTree_RunStatus::Stopped;

public:
    CK_PROPERTY_GET(_Owner);
    CK_PROPERTY_GET(_InstanceData);
    CK_PROPERTY_GET(_RunStatus);
};
```

### 2.3 Define Request Structs
**File:** `CkStateTree_Fragment_Data.h`

```cpp
// Start logic execution
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_StartLogic : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_StateTree_StartLogic);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_StartLogic);
};

// Restart logic (stop + start)
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_RestartLogic : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_StateTree_RestartLogic);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_RestartLogic);
};

// Stop logic execution
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_StopLogic : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_StateTree_StopLogic);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_StopLogic);
};

// Pause logic execution
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_PauseLogic : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_StateTree_PauseLogic);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_PauseLogic);
};

// Resume paused logic
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_ResumeLogic : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_StateTree_ResumeLogic);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_ResumeLogic);
};
```

### 2.4 Update Request Variant
**File:** `CkStateTree_Fragment.h`

```cpp
struct CKSTATETREE_API FFragment_StateTree_Requests
{
    CK_GENERATED_BODY(FFragment_StateTree_Requests);

    friend class FProcessor_StateTree_HandleRequests;
    friend class UCk_Utils_StateTree_UE;

    using RequestType = std::variant<
        FCk_Request_StateTree_StartLogic,
        FCk_Request_StateTree_RestartLogic,
        FCk_Request_StateTree_StopLogic,
        FCk_Request_StateTree_PauseLogic,
        FCk_Request_StateTree_ResumeLogic
    >;
    using RequestList = TArray<RequestType>;

private:
    RequestList _Requests;

public:
    CK_PROPERTY_GET(_Requests);
};
```

## Success Criteria
1. `FCk_Fragment_StateTree_ParamsData` accepts StateTree asset reference
2. All request structs compile with proper macros
3. Fragment variant includes all request types
4. `FFragment_StateTree_Current` can hold instance data and owner

## Files to Modify
- **MODIFY:** `Public/CkStateTree/CkStateTree_Fragment_Data.h`
- **MODIFY:** `Public/CkStateTree/CkStateTree_Fragment.h`

## Notes
- `TStrongObjectPtr` required for Owner since it's a UObject in non-UPROPERTY context
- `FStateTreeInstanceData` is stored directly in fragment (not pointer)
- Simple request structs with no parameters - lifecycle control only
