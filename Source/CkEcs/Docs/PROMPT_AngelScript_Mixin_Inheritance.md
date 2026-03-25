# AngelScript Mixin Inheritance for Derived Handle Types

**Status: IMPLEMENTED (Generic Solution)**

## Goal
Make all `FCk_Handle` mixin methods (like `Request_DestroyEntity`, `Get_WorldForEntity`, etc.) available on all derived handle types (like `FCk_Handle_StateTree`, `FCk_Handle_Probe`, etc.) in AngelScript.

Currently, AngelScript mixins defined via `UCLASS(Meta = (ScriptMixin = "FCk_Handle"))` only apply to `FCk_Handle` itself. Derived types like `FCk_Handle_StateTree` don't inherit these methods, forcing users to cast back to `FCk_Handle` to access common operations.

## Implementation

The solution uses the AngelScript engine's `asITypeInfo` API to enumerate all methods bound to `FCk_Handle` at runtime, then re-binds them to all registered derived handle types.

### Key Changes

**CkHandle_TypeSafe_AngelScript.cpp:**
- `BindBaseMixinMethods()` enumerates all methods on `FCk_Handle` using `asITypeInfo::GetMethodCount()` and `GetMethodByIndex()`
- For each system function (native binding), extracts the function declaration, function pointer, and calling convention
- Re-registers the same method on each derived handle type using `RegisterObjectMethod()`
- Skips operators (`op*`), conversion methods (`As_*`, `Is_*`), and utility methods (`IsValid`, `ToString`, `Debug`, `H`) which are already handled by `CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION`

### How It Works

The implementation leverages the fact that:
1. Derived handle types implicitly convert to `FCk_Handle&` via the bindings in `CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION`
2. The mixin methods all take `FCk_Handle&` or `const FCk_Handle&` as their first parameter (which becomes `this`)
3. By re-registering the exact same function pointer with the same calling convention on derived types, the implicit conversion handles the type compatibility

### Methods Automatically Propagated

Any method bound to `FCk_Handle` via `ScriptMixin` metadata is automatically propagated, including:
- `UCk_Utils_EntityLifetime_UE` methods (Request_DestroyEntity, Get_WorldForEntity, etc.)
- Any other Utils class with `ScriptMixin = "FCk_Handle"`
- Future mixin methods added without code changes

### Excluded Methods

The following methods are NOT propagated (they're already handled separately):
- Operators (`op*`) - handled by macro
- Conversion methods (`As_*`, `Is_*`) - handled by `BindCrossHandleConversions()`
- Utility methods (`IsValid`, `ToString`, `Debug`, `H`) - handled by macro

## Current Architecture

### Mixin Registration (via UCLASS metadata)
```cpp
// D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\EntityLifetime\CkEntityLifetime_Utils.h
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle"))
class CKECS_API UCk_Utils_EntityLifetime_UE : public UBlueprintFunctionLibrary
{
    // Methods like Request_DestroyEntity, Get_WorldForEntity, etc.
    // These are only available on FCk_Handle in AngelScript
};
```

### Derived Handle Registration
```cpp
// D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\Handle\CkHandle_TypeSafe_AngelScript.h
// The CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION macro registers each derived handle type
// It already binds: opImplConv, IsValid, ToString, As_X, Is_X methods
// But does NOT propagate FCk_Handle mixin methods
```

### Handle Type Registry
```cpp
// D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\Handle\CkHandle_TypeSafe_AngelScript.cpp
// FCkAngelScriptHandleTypeRegistry - tracks all registered handle types
// FCkAngelScriptHandleRegistration - hooks into FAngelscriptCodeModule::GetPreCompile()
// BindCrossHandleConversions() - binds As_/Is_ methods between handle types
```

## Key Files to Modify

1. **D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\Handle\CkHandle_TypeSafe_AngelScript.h**
   - Contains `CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION` macro
   - Contains `FCkAngelScriptHandleTypeRegistry` class
   - Contains `FCkAngelScriptHandleBindingTracker` class

2. **D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\Handle\CkHandle_TypeSafe_AngelScript.cpp**
   - Contains `FCkAngelScriptHandleTypeRegistry::BindCrossHandleConversions()`
   - This is where cross-handle As_/Is_ methods are bound
   - **This is the ideal place to also bind base mixin methods**

3. **D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\EntityLifetime\CkEntityLifetime_Utils.h**
   - Contains the mixin methods we want to propagate
   - Reference for which methods need to be available

## Proposed Solution

### Option A: Extend BindCrossHandleConversions()
After binding cross-handle conversions, iterate through all registered derived handle types and bind wrapper methods that call the base `FCk_Handle` mixin methods.

```cpp
// In BindCrossHandleConversions() or a new BindBaseMixinMethods()
for (const auto& HandleType : GetRegisteredHandleTypes())
{
    auto Bind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*HandleType.TypeName));
    if (Bind.GetTypeInfo() == nullptr) continue;
    
    // Bind Request_DestroyEntity
    Bind.Method("void Request_DestroyEntity(ECk_EntityLifetime_DestructionBehavior = ECk_EntityLifetime_DestructionBehavior::ForceDestroy)",
        [](FCk_Handle& Self, ECk_EntityLifetime_DestructionBehavior Behavior)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Self, Behavior);
        });
    
    // ... other methods
}
```

### Option B: Create a Registration System for Base Mixin Methods
Create a registry where base mixin methods can be registered, then automatically propagate them to all derived handle types.

```cpp
class FCkAngelScriptBaseMixinRegistry
{
public:
    struct FMixinMethodInfo
    {
        FString Signature;
        TFunction<void(FAngelscriptBinds&)> BindFunc;
    };
    
    static void RegisterBaseMixinMethod(const FMixinMethodInfo& Info);
    static void BindToAllDerivedTypes();  // Called after all handle types registered
};
```

## Methods to Propagate (from UCk_Utils_EntityLifetime_UE)

These methods take `FCk_Handle&` or `const FCk_Handle&` as first parameter:

1. `Request_DestroyEntity(FCk_Handle&, ECk_EntityLifetime_DestructionBehavior)`
2. `Get_LifetimeOwner(const FCk_Handle&, ECk_PendingKill_Policy)` -> `FCk_Handle`
3. `Get_LifetimeDependents(const FCk_Handle&)` -> `TArray<FCk_Handle>`
4. `Get_IsPendingDestroy(const FCk_Handle&, ECk_EntityLifetime_DestructionPhase)` -> `bool`
5. `Get_IsTransientEntity(const FCk_Handle&)` -> `bool`
6. `Get_WorldForEntity(const FCk_Handle&)` -> `UWorld*`
7. `BindTo_OnBeginDestroy(FCk_Handle&, delegate, policy, behavior)`
8. `UnbindFrom_OnBeginDestroy(FCk_Handle&, delegate)`

## Technical Considerations

1. **Timing**: The binding must happen after:
   - All derived handle types are registered via `CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION`
   - The `FCk_Handle` type is available in AngelScript
   - Before AngelScript compilation completes

2. **Method Signatures**: The AngelScript method signatures must match exactly. Since derived handles can implicitly convert to `FCk_Handle&`, we can use wrapper lambdas.

3. **Include Dependencies**: `CkHandle_TypeSafe_AngelScript.cpp` may need to include `CkEntityLifetime_Utils.h` to call the actual methods.

4. **Extensibility**: Consider making this system generic so other Utils classes with `ScriptMixin = "FCk_Handle"` can also register their methods for propagation.

## Reference: AngelScript Binding API

From `D:\Repos\UnrealEngineAngelscript\Engine\Plugins\Angelscript\Source\AngelscriptCode\Public\AngelscriptBinds.h`:

```cpp
// Bind a method using a lambda
template<typename T>
inline int Method(FBindString Signature, T Function, void* UserData = nullptr)
{
    auto FunctionPointer = (typename TLambdaFuncPtr<T>::Type)Function;
    return BindExternMethod(Signature, asFUNCTION(FunctionPointer), ...);
}

// Get existing class binding
static FAngelscriptBinds ExistingClass(FBindString Name);
```

## Testing

After implementation, verify in AngelScript:
```angelscript
FCk_Handle_StateTree StateTreeHandle = ...;

// These should work without casting to FCk_Handle first:
StateTreeHandle.Request_DestroyEntity();
UWorld@ World = StateTreeHandle.Get_WorldForEntity();
bool bPending = StateTreeHandle.Get_IsPendingDestroy(ECk_EntityLifetime_DestructionPhase::EndPlay);
```
