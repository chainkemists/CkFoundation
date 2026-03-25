# Phase 1: Schema and Owner Object Foundation

## Objectives
1. Create `UCk_StateTree_Schema_Entity` - custom schema for entity-based StateTrees
2. Create `UCk_StateTree_Owner` - minimal UObject to own StateTree execution

## Dependencies
- None (first phase)

## Tasks

### 1.1 Create UCk_StateTree_Schema_Entity
**File:** `CkStateTree_Schema.h` / `CkStateTree_Schema.cpp`

Schema must define:
- Context data: `FCk_Handle_StateTree` (the entity executing the tree)
- External data pattern for entity access

```cpp
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, 
       Meta = (DisplayName = "CK Entity StateTree"))
class CKSTATETREE_API UCk_StateTree_Schema_Entity : public UStateTreeSchema
{
    GENERATED_BODY()
    
protected:
    // Override to define context data requirements
    virtual void PostLoad() override;
    virtual void GetContextDataDescs(
        TArray<FStateTreeContextDataDesc>& OutContextDataDescs) const override;
    
    // Define what external data the schema supports
    virtual bool IsExternalItemTypeAllowed(
        const UStruct& InItemType) const override;
};
```

Key decisions:
- Context provides: Entity handle, World pointer
- No actor context (we're actorless)
- Allow subsystem access as external data

### 1.2 Create UCk_StateTree_Owner
**File:** `CkStateTree_Owner.h` / `CkStateTree_Owner.cpp`

Minimal UObject that:
- Serves as owner for `FStateTreeExecutionContext`
- Holds reference to owning entity handle
- Created with World as outer (no actor)

```cpp
UCLASS()
class CKSTATETREE_API UCk_StateTree_Owner : public UObject
{
    GENERATED_BODY()
    
public:
    // Entity this owner is bound to
    FCk_Handle_StateTree _OwnerEntity;
    
    // Factory method
    static auto Create(
        UWorld* InWorld,
        FCk_Handle_StateTree InOwnerEntity) -> UCk_StateTree_Owner*;
    
    // Context setup helper
    auto SetContextRequirements(
        FStateTreeExecutionContext& InContext) const -> bool;
    
    // External data collection callback
    auto CollectExternalData(
        const FStateTreeExecutionContext& InContext,
        const UStateTree* InStateTree,
        TArrayView<const FStateTreeExternalDataDesc> InExternalDataDescs,
        TArrayView<FStateTreeDataView> OutDataViews) -> bool;
};
```

### 1.3 Create Context Data Struct
**File:** `CkStateTree_ContextData.h`

Struct that holds the entity context available to tasks/conditions:

```cpp
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_StateTree_EntityContext
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCk_Handle_StateTree Entity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UWorld> World;
};
```

## Success Criteria
1. Can create StateTree asset and select "CK Entity StateTree" schema
2. Schema shows Entity context in editor
3. `UCk_StateTree_Owner` compiles and can be instantiated with World outer
4. Module dependencies updated in Build.cs if needed

## Files to Create/Modify
- **CREATE:** `Public/CkStateTree/CkStateTree_Schema.h`
- **CREATE:** `Public/CkStateTree/CkStateTree_Schema.cpp`
- **CREATE:** `Public/CkStateTree/CkStateTree_Owner.h`
- **CREATE:** `Public/CkStateTree/CkStateTree_Owner.cpp`
- **CREATE:** `Public/CkStateTree/CkStateTree_ContextData.h`
- **MODIFY:** `CkStateTree.Build.cs` (if additional dependencies needed)

## Notes
- Schema class MUST be BlueprintType to appear in StateTree editor dropdown
- Owner object uses World as outer since we have no Actor
- The context struct makes entity access type-safe in tasks/conditions
