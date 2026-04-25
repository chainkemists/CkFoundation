# CLAUDE.md — CkFoundation

CkFoundation is an ECS framework plugin for Unreal Engine 5.5, built on EnTT 3.15.0. It provides 80+ modules implementing gameplay systems (Abilities, Inventory, Grid, Attributes, Audio, etc.) through a composition-based Entity Component System architecture.

## Companion Guidelines

- **C++ deep-dive:** `Source/CLAUDE.md` — extended C++ rules and examples beyond what's in this file.
- **AngelScript (.as):** `Script/CLAUDE.md` — required reading before editing any `.as` file. Covers `utils_*` shortcuts, dynamic handle registration (`Script/Generated/DynamicHandleTypes.json`), spawn params, by-value struct param gotcha, and C++↔AS differences (no lambdas, no `static_cast`, no `NOT` macro, RPCs are reliable-by-default, `float` is 64-bit).

## Finding Modules

Each gameplay system lives in `Source/Ck<Name>/`. Editor-only counterparts use `Ck<Name>Editor`. The full module set with load phases is enumerated in `CkFoundation.uplugin`.

## Architecture Principles

- **Composition over inheritance.** Design new systems from first principles using ECS composition. Do not copy patterns from third-party plugins unless explicitly asked.
- **Event-driven over timer-based.** Prefer delegate/signal callbacks. Never use timer deferrals as a workaround for ordering or race conditions.
- **Requests are deferred.** Mutations to ECS state go through request fragments processed by processors. Never mutate entity state directly from utility functions — enqueue a request.
- **Authority matters.** Always check `UCk_Utils_Net_UE::Get_HasAuthority` before enqueuing requests. Use `NetMulticast` for RPCs on unowned actors — `Client` RPCs require ownership.

## Module Structure

Every module follows this layout:

```
CkModuleName/
├── CkModuleName.Build.cs            # Inherits from CkModuleRules (C++20, explicit PCH)
├── CkModuleName_Module.cpp/h        # Module init
├── CkModuleName_Log.cpp/h           # Log category
└── Public/CkModuleName/
    └── Subsystem/
        ├── CkSubsystem_Fragment.h        # Fragments, Tags, Records, Signals
        ├── CkSubsystem_Fragment_Data.h   # USTRUCT data types (reflected)
        ├── CkSubsystem_Fragment_Data.cpp # Constructors, gameplay tag definitions
        ├── CkSubsystem_Processor.h       # Processor declarations
        ├── CkSubsystem_Processor.cpp     # Processor implementations
        ├── CkSubsystem_Utils.h           # UBlueprintFunctionLibrary (public API)
        ├── CkSubsystem_Utils.cpp         # Utils implementation
        └── ProcessorInjector/            # Processor registration
```

## Naming Conventions

### Prefixes
| Prefix | Meaning | Example |
|--------|---------|---------|
| `F` | Struct | `FCk_Handle_Inventory`, `FFragment_Inventory_Params` |
| `U` | UObject | `UCk_Utils_Inventory_UE` |
| `A` | Actor | `ACk_CueExecutor_UE` |
| `T` | Template | `TProcessor`, `TFragment_ContainerEntryRef` |
| `E` | Enum | `ECk_InventoryType` |
| `I` | Interface | `ICk_CustomActorComponentVisualizer_Interface` |

### Suffixes & Patterns
| Pattern | Purpose | Example |
|---------|---------|---------|
| `_Fragment` | ECS fragment | `FFragment_Inventory_SyncReplication` |
| `_Fragment_Data` | Reflected data struct | `FCk_Fragment_Inventory_ParamsData` |
| `_Processor` | ECS system | `FProcessor_Inventory_HandleRequests` |
| `_Utils` / `_Utils_UE` | Blueprint function library | `UCk_Utils_Inventory_UE` |
| `_Tag` | ECS tag | `FTag_Inventory_Spatial` |
| `FTag_` | ECS tag (no data) | `FTag_Inventory_MayRequireReplication` |
| `FCk_Handle_` | Type-safe handle | `FCk_Handle_Inventory`, `FCk_Handle_Item` |
| `MarkedDirtyBy` | Processor dirty-check type alias | `using MarkedDirtyBy = FFragment_Inventory_Requests;` |

### Fragment Params Pattern
Reflected data structs live in `_Fragment_Data.h`. The ECS fragment is a `using` alias:
```cpp
// In _Fragment_Data.h (reflected, Blueprint-visible)
USTRUCT(BlueprintType)
struct FCk_Fragment_Inventory_ParamsData { ... };

// In _Fragment.h (ECS-side alias)
using FFragment_Inventory_Params = FCk_Fragment_Inventory_ParamsData;
```

## ECS Patterns

### Fragment Definition
```cpp
struct CKINVENTORY_API FFragment_Inventory_Requests
{
    CK_GENERATED_BODY(FFragment_Inventory_Requests);
    friend class FProcessor_Inventory_HandleRequests;

    using AddItemRequestType    = FCk_Request_Inventory_AddItem;
    using RemoveItemRequestType = FCk_Request_Inventory_RemoveItem;
    using RequestType = std::variant<AddItemRequestType, RemoveItemRequestType>;

private:
    TArray<RequestType> _Requests;
};
```

### Tag Definition
```cpp
namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Inventory_Spatial);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayRequireReplication);
}
```

### Handle Definition
```cpp
USTRUCT()
struct CKINVENTORY_API FCk_Handle_Inventory : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Inventory);
};
```

### Record Definition
```cpp
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfInventoryItems, FCk_Handle_Item);
```

### Signal Definition
```cpp
CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
    CKINVENTORY_API,
    Inventory_OnItemsChanged,
    FCk_Delegate_Inventory_OnItemsChanged,
    FCk_Handle_Inventory,    // Signal source
    TArray<FCk_Handle>,      // Items added
    TArray<FCk_Handle>);     // Items removed
```

### Processor Definition
```cpp
class CKINVENTORY_API FProcessor_Inventory_HandleRequests : public ck_exp::TProcessor<
    FProcessor_Inventory_HandleRequests,
    FCk_Handle_Inventory,
    FFragment_Inventory_Params,
    FFragment_Inventory_Requests,
    CK_IGNORE_PENDING_KILL>
{
public:
    using MarkedDirtyBy = FFragment_Inventory_Requests;
    using TProcessor::TProcessor;

public:
    auto ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Inventory_Params& InParams,
        FFragment_Inventory_Requests& InRequests) const -> void;
};
```

### Request Pattern
Requests inherit `FRequest_Base` and use `CK_REQUEST_DEFINE_DEBUG_NAME`:
```cpp
struct CKINVENTORY_API FCk_Request_Inventory_AddItem : FRequest_Base
{
    CK_GENERATED_BODY(FCk_Request_Inventory_AddItem);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_AddItem);
    friend class FProcessor_Inventory_HandleRequests;

private:
    FCk_Handle _ItemToAdd;
public:
    CK_PROPERTY_GET(_ItemToAdd);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_AddItem, _ItemToAdd);
};
```

### Per-Request Completion Delegates
Use `CK_SIGNAL_BIND_REQUEST_FULFILLED` for optional callbacks fired when a deferred request completes:
```cpp
CK_SIGNAL_BIND_REQUEST_FULFILLED(
    ck::UUtils_Signal_Inventory_OnItemAddedOrNot,
    InRequest.PopulateRequestHandle(InInventory),
    InDelegate);
```
UFUNCTION parameters use `AutoCreateRefTerm` for the optional delegate.

### Replication Handler Registration
Client-side sync requires registering with `FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy`:
```cpp
FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
    []() -> UScriptStruct* { return FCk_RepData_InventoryItems::StaticStruct(); },
    {
        .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old) { ... },
        .OnAdd    = [](FCk_Handle& Entity, const FInstancedStruct& Data) { ... }
    });
```

### Gameplay Tag Categories
Features requiring tag categories define them with `UE_DEFINE_GAMEPLAY_TAG_STATIC` in a `_Fragment_Data.cpp`:
```cpp
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Category_Inventory, TEXT("Inventory"));
```

## CK Macros Reference

| Macro | Purpose |
|-------|---------|
| `CK_GENERATED_BODY(Class)` | Class metadata + validation |
| `CK_DEFINE_CONSTRUCTORS(Class, vars...)` | Default + parametrized constructors |
| `CK_DEFINE_CONSTRUCTOR(Class, vars...)` | Parametrized only (no default) |
| `CK_PROPERTY(_Var)` | Get + Set + Update |
| `CK_PROPERTY_GET(_Var)` | Const ref getter only |
| `CK_PROPERTY_GET_BY_COPY(_Var)` | By-value getter only |
| `CK_PROPERTY_SET(_Var)` | Setter only |
| `CK_DEFINE_ECS_TAG(Name)` | Tag with no data |
| `CK_DEFINE_ECS_TAG_COUNTED(Name)` | Counted tag |
| `CK_DEFINE_RECORD_OF_ENTITIES(Fragment, Handle)` | Record fragment |
| `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(...)` | Signal + utils + delegate type |
| `CK_IGNORE_PENDING_KILL` | Processor excludes pending-destroy entities |
| `CK_ENSURE_IF_NOT(cond, fmt, args...)` | Conditional ensure with `{}` format |
| `ON_SCOPE_EXIT { ... }` | Deferred scope-exit block |

## Formatting & Validation

- **`CK_ENSURE_IF_NOT`** uses `{}` format specifiers (libfmt-style), NOT `%s`:
  ```cpp
  CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
      TEXT("Invalid handle [{}].{}"), InHandle, ck::Context(this))
  { return {}; }
  ```
- **`ck::Context()`** provides caller context. Pass directly — do not dereference with `*`.
- **`ck::IsValid()` / `ck::Is_NOT_Valid()`** for validity checks. Never use raw pointer null checks.

## Utility Class Pattern

```cpp
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory"))
class CKINVENTORY_API UCk_Utils_Inventory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    CK_GENERATED_BODY(UCk_Utils_Inventory_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Add")
    static FCk_Handle_Inventory Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams);
};
```

## Algorithm Library (`ck::algo`)

Located in `CkCore/Algorithms/CkAlgorithms.h`. Key functions:

| Function | Variants | Notes |
|----------|----------|-------|
| `Filter` | Copy (`const&` → new container) | Returns filtered copy via `FilterByPredicate` |
| `FilterInPlace` | Mutating (`&` → `void`) | Removes non-matching via `RemoveAll` |
| `Sort` | In-place (`&`), Copy (`const&`) | In-place is the default meaning |
| `Except(A, B)` | Basic, Projection overload | Set difference: elements in A not in B |
| `Intersect(A, B)` | Basic, Projection overload | Set intersection |
| `Transform` | To new container, Into existing | Map operation |
| `ForEach` | Container, Iterator, `IsValid` variants | Iteration |
| `ForEachRequest` | `TArray`, `TOptional`, `DontResetContainer` | Request processing |
| `AllOf` / `AnyOf` / `NoneOf` | Container, Iterator | Predicates |
| `FindIf` | Iterator, `TOptional` return | Search |
| `CountIf` / `FindIndex` | Container | Counting / indexing |

Projection overloads take a member function pointer for comparison by a projected key:
```cpp
const auto ByItemHandle = &FCk_InventoryItem_ReplicatedEntry::Get_ItemHandle;
const auto Added   = ck::algo::Except(Current, Previous, ByItemHandle);
const auto Removed = ck::algo::Except(Previous, Current, ByItemHandle);
```

## Variant Dispatch with `ck::Visitor`

`ck::Visitor` wraps a **single** callable with `auto` parameter for `std::variant` dispatch. Do NOT pass multiple lambdas:
```cpp
// CORRECT — single generic lambda
ck::algo::ForEachRequest(Requests, ck::Visitor(
    [&](const auto& InRequest) -> void
    {
        DoHandleRequest(InHandle, InParams, InRequest, ItemsAdded, ItemsRemoved);
    }), ck::policy::DontResetContainer{});

// WRONG — multiple lambdas (ck::Visitor is not std::visit Overload)
ck::Visitor([&](const AddRequest& r) { ... }, [&](const RemoveRequest& r) { ... })
```
Use function overloading on the handler side (e.g., `DoHandleRequest` overloads) instead of lambda overloads.

## Technique Pipeline (`ck::Technique`)

Located in `CkCore/Public/CkCore/Technique/CkTechnique.h`. A CRTP-based step pipeline for expressing multi-phase operations as named steps instead of comment-delimited code blocks.

**When to use:** Processor logic with 3+ distinct phases (validate, transform, finalize) where phase comments like `// ---- Phase 1: ... ----` would otherwise be needed. The step function names replace the comments.

```cpp
// Context struct holds all mutable state shared across steps
struct FContext_MyOperation
{
    // Inputs, outputs, cached state
    int32 Remaining = 0;
    ECk_Result Result = ECk_Result::Failed;
};

// Derive from Technique, register steps in constructor
struct FTechnique_MyOperation
    : ck::Technique<FTechnique_MyOperation, FContext_MyOperation&>
{
    FTechnique_MyOperation()
    {
        AddStep(&FTechnique_MyOperation::Validate);
        AddStep(&FTechnique_MyOperation::ProcessItems);
        AddStep(&FTechnique_MyOperation::DetermineResult);
    }

    // Optional: define ShouldAbort() to enable short-circuit between steps
    auto ShouldAbort() const -> bool { return _Abort; }

    static auto Validate(FTechnique_MyOperation& InSelf, FContext_MyOperation& Ctx) -> void;
    static auto ProcessItems(FTechnique_MyOperation& InSelf, FContext_MyOperation& Ctx) -> void;
    static auto DetermineResult(FTechnique_MyOperation& InSelf, FContext_MyOperation& Ctx) -> void;

    bool _Abort = false;
};

// Usage in processor
static auto Technique = FTechnique_MyOperation{};
Technique._Abort = false;
Technique.ProcessAllSteps(Context);
```

**Key points:**
- Steps run in registration order. If `ShouldAbort()` is defined on the derived type, it is checked between steps automatically (SFINAE-detected).
- Context struct is a plain aggregate — no inheritance needed.
- Technique instances can be `static` since they only hold the step list (state lives in Context).
- Steps are `static` member functions taking `(DerivedType&, T_Params&&...)`.

## Code Style

- **Trailing return types:** `auto Foo() -> void` / `auto Foo() -> int32`
- **Member variables:** Prefixed with `_` (e.g., `_Requests`, `_Name`)
- **Namespaces:** All ECS types in `namespace ck`. No anonymous namespaces — use `static` or internal classes.
- **Constructor definitions:** In `.cpp` files, not headers.
- **Include order:** Standard library → Unreal Engine → CkCore/CkEcs → Module-specific → `.generated.h` (always last)
- **NOT macro:** Use `NOT` instead of `!` for boolean negation in conditionals.
- **Section separators:** Use `// ----` comment lines between logical sections.
- **`auto` everywhere:** Prefer `auto` for local variables. Use explicit types only when clarity demands it.
- **`MoveTemp`:** Use UE's `MoveTemp` instead of `std::move`.

## Common Pitfalls

- **Missing replication handler:** If a `SyncReplication` processor never fires on clients, check that `FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy` is registered in the `_Fragment.cpp`.
- **Parameter shadowing:** Never use `Instigator`, `Controller`, or other AActor member names as parameters.
- **`TObjectPtr` vs raw pointers:** Use `TObjectPtr` for `UPROPERTY` members. Raw pointers are fine for local variables and non-reflected code.
- **Overload ambiguity with const/non-const:** When adding both mutating (`T&` → `void`) and copy (`const T&` → `T`) overloads, the mutating overload wins for non-const arguments. Use distinct names (e.g., `Filter` vs `FilterInPlace`) when callers might assign the return value.
- **`std::vector` in TArray code:** Never use `std::vector` as an intermediary. Use `TArrayBackInserter` for STL algorithm output.
- **Format specifiers:** `CK_ENSURE_IF_NOT` and `ck::Format` use `{}`, never `%s` or `%d`.

## Build System

CkFoundation is an Unreal plugin — there is no standalone build/test entry point in this directory. Compilation, cooking, and automation tests run via the host UE project that includes the plugin (UnrealBuildTool / RunUAT / `Engine\Binaries\Win64\UnrealEditor-Cmd.exe ... -ExecCmds="Automation RunTests ..."`).

All modules inherit from `CkModuleRules` (defined in `CkBuildConfig`):
- C++20 standard
- Explicit/shared PCH
- Key defines: `CK_FORMAT_FORCE_DETAILED`, `CK_BUILD_LOGGING`, `WITH_ANGELSCRIPT_CK` (conditional)
- Core dependencies most modules need: `CkCore`, `CkEcs`, `CkLog`, `CkThirdParty`

### Editor-callable maintenance (AngelScript)

`UCkDynamicHandleSubsystem` exposes two `CallInEditor` buttons (Editor Subsystems panel, or invoke from Blueprint):
- `GenerateHandleTypeRegistry()` — discovers all `UCkDynamic_HandleDefinition` assets and writes `Script/Generated/DynamicHandleTypes.json` sorted by `TypeName`.
- `ForceRefreshDynamicHandleBindings()` — regenerates + re-registers AS bindings without an editor restart (dev-only).

Editor restart is normally required after registry changes; hot reload does not pick them up.

## Third-Party Libraries (in CkThirdParty)

- **EnTT 3.15.0** — ECS backend
- **fmt** — String formatting
- **cleantype** — Type name utilities
- **ctti** — Compile-time type info
- **JoltPhysics** — Physics
