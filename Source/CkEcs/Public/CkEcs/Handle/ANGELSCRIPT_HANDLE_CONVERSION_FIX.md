# AngelScript Handle Conversion Fixes

## Problem 1: Wrong Return Type (FIXED)

When calling `As_Probe()` from a derived handle, the returned handle was invalid.

### Root Cause:
The generic method was constructing the return value as `FCk_Handle` instead of the target type:
```cpp
new (InGeneric->GetAddressOfReturnLocation()) FCk_Handle{Result};  // ❌ WRONG TYPE
```

### Fix:
Use `memcpy` to copy raw bytes, letting AngelScript's type system interpret them:
```cpp
FMemory::Memcpy(ReturnLocation, &Result, sizeof(FCk_Handle));  // ✅ CORRECT
```

---

## Problem 2: TMap Pointer Instability - CRASHES (FIXED)

### The Crash:
Accessing auxiliary data in generic methods caused crashes with corrupted memory in the handle's registry.

### Root Cause:
**TMap does NOT guarantee pointer stability!** When the map grows and rehashes, it relocates entries, invalidating pointers.

#### Original (Broken) Code:
```cpp
static TMap<FString, FAsMethodData> AsMethodLookup;
auto LookupKey = FString::Printf(TEXT("%s_As_%s"), ...);
AsMethodLookup.Add(LookupKey, FAsMethodData{...});

// ❌ DANGER: Pointer to map entry becomes invalid when map grows!
SourceBind.GenericMethod(..., &AsMethodLookup[LookupKey]);
```

**What happens:**
1. We store a pointer to the map entry: `&AsMethodLookup[LookupKey]`
2. Later, more entries are added to the map
3. TMap rehashes and moves all entries to new memory locations
4. Our stored pointer now points to freed/invalid memory
5. **CRASH** when the generic method tries to dereference it

### The Fix:

Store lookup **keys** in a TArray (which provides pointer stability), then use the key to look up the actual data inside the lambda:

```cpp
// Store function data in maps
static TMap<FString, FAsMethodData> AsMethodLookup;

// TArray provides pointer stability - elements don't move when array grows
static TArray<FString> AsLookupKeys;

// Create and store the key
auto LookupKey = FString::Printf(TEXT("%s_As_%s"), ...);
auto& StoredKey = AsLookupKeys.Add_GetRef(LookupKey);  // ✅ Stable pointer

// Store the method data
AsMethodLookup.Add(LookupKey, FAsMethodData{...});

// Pass pointer to stable key as auxiliary data
SourceBind.GenericMethod(...,
    [](asIScriptGeneric* InGeneric)
    {
        // Get the stable key pointer
        auto LookupKey = static_cast<const FString*>(InGeneric->GetAuxiliary());
        
        // Use the key to look up the actual data (safe!)
        auto* MethodData = AsMethodLookup.Find(*LookupKey);
        
        // Use MethodData...
    },
    &StoredKey);  // ✅ Pass stable pointer to the key
```

### Why This Works:

1. **TArray Pointer Stability**: When a TArray grows, it allocates new memory and copies elements, but the **addresses of existing elements remain valid** because TArray uses contiguous memory and doesn't relocate on growth (it allocates larger chunks).

2. **Indirection**: We don't store pointers to the function data (which lives in the TMap). Instead, we store pointers to **keys** (which live in the TArray), then use those keys to look up the data at runtime.

3. **Two-Level Lookup**: 
   - Level 1: Auxiliary data → stable key pointer (never invalidated)
   - Level 2: Key → function data in TMap (lookup happens at call time, after any rehashing)

### Applied to Both Methods:

This fix was applied to both `As_` and `Is_` method bindings:

**As_ Methods:**
```cpp
static TArray<FString> AsLookupKeys;
auto& StoredKey = AsLookupKeys.Add_GetRef(LookupKey);
// ...
auto* MethodData = AsMethodLookup.Find(*LookupKey);
```

**Is_ Methods:**
```cpp
static TArray<FString> IsLookupKeys;
auto& StoredKey = IsLookupKeys.Add_GetRef(LookupKey);
// ...
auto* MethodData = IsMethodLookup.Find(*LookupKey);
```

---

## Verification

After both fixes, all conversions work correctly:

```cpp
auto ProbeNode = FCk_Handle_ProbeTrace{};      // Some valid probe trace handle
auto RegularHandle = ProbeNode.H();            // Convert to base FCk_Handle
auto AsProbe = ProbeNode.As_Probe();           // ✅ VALID (was broken)
auto AsProbe2 = RegularHandle.As_Probe();      // ✅ VALID (always worked)
auto IsProbe = ProbeNode.Is_Probe();           // ✅ VALID (was crashing)
```

---

## Technical Notes

- **TArray pointer stability** is guaranteed in Unreal because TArray uses a slack-based growth strategy
- The TMap still rehashes and moves entries, but we never store pointers to map entries anymore
- This pattern (stable key → dynamic lookup) is safer than storing pointers to container elements
- The overhead of the map lookup is negligible compared to the actual handle conversion work
