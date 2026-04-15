# CkCore / Types

General-purpose types that don't deserve their own folder.

**Key files:** `CkPtrWrapper.h`, `DataAsset/` (subfolder for `UDataAsset` bases used across CkFoundation).

## `TPtrWrapper<T_ComplexPtrType>`

Forces const-correctness for pointer members. A raw `T*` member leaks mutability through `const` methods; wrapping the pointer in `TPtrWrapper` makes the pointee respect the wrapper's const-ness.

```cpp
namespace ck
{
    template <typename T_ComplexPtrType>
    class TPtrWrapper
    {
        CK_GENERATED_BODY(TPtrWrapper<T_ComplexPtrType>);
    public:
        using ValueType       = T_ComplexPtrType;
        using StoredValueType = typename type_traits::ExtractValueType<T_ComplexPtrType>::type;
        // ... ctors, deref, comparison (see header)
    };
}
```

Use when you have a `UObject*` / POD-pointer member that should be observably immutable through `const` member functions. For `UObject*` lifetime, prefer `TStrongObjectPtr` / `TObjectPtr` per the root CLAUDE.md memory-management guide — `TPtrWrapper` is orthogonal (const-correctness, not lifetime).

## `DataAsset/`

Subfolder hosting project-wide `UDataAsset` base classes. Notably `UCk_GameplayTags` (declarable from AngelScript via `asset ... of UCk_GameplayTags { GameplayTags.Add(n"..."); }`).

## Depends on
`Macros/`, `TypeTraits/`.

## Used by
Any type that holds a pointer-to-external-thing and wants const-correct dereference.

## See also
- `/Source/CLAUDE.md` section 11 "Memory management" — `TStrongObjectPtr` / `TObjectPtr` guidance for `UObject` lifetimes.
- `/Source/CLAUDE.md` section 15 "Angelscript asset creation" — how `UCk_GameplayTags` assets get defined from AS.
