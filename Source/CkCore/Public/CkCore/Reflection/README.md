# CkCore / Reflection

`FProperty`-level reflection helpers. Knows about user-defined struct quirks (name mangling, GUIDs), reinstancing placeholder classes, and property compatibility. Used by the AngelScript generator, the dynamic system, and any tooling that walks Unreal reflection.

**Key file:** `CkReflection_Utils.h` → `UCk_Utils_Reflection_UE`

## Public types

```cpp
UENUM() enum class ECk_PropertyDefaultValueKind : uint8
{
    RawLiteral, // valid in both C++ and AngelScript as-is
    String,     // wrap as "<value>" for AS
    Name,       // wrap as n"<value>" for AS
    Text,       // wrap as FText::FromString("<value>") for AS
};

struct CKCORE_API FCk_PropertyDefaultValueLiteral
{
    ECk_PropertyDefaultValueKind _Kind = RawLiteral;
    FString                      _Value;
};
```

## Key methods on `UCk_Utils_Reflection_UE`

- `Get_SanitizedUserDefinedPropertyName(const FProperty*)` — UserDefined struct properties use the format `"MyProp_SomeNumber_SomeGuid"`. This strips the tail.
- `Get_PropertyBySanitizedName(UObject*, const FString&)` — inverse lookup.
- `Get_UserDefinedPropertyGuid(const FProperty*)` — extracts the GUID tail from a UDS property.
- `Get_ArePropertiesCompatible(const FProperty*, const FProperty*)` — type-and-flags compat check (use when mapping properties across structs).
- `Get_ArePropertiesDifferent(const TArray<FProperty*>&, const TArray<FProperty*>&)` — array-wise diff.
- `Get_ExposedPropertiesOfClass(const UClass*)` — BP-exposed `FProperty*`s.
- `Get_IsDelegateProperty(const FProperty*)`.
- `Is_PlaceholderClass(const UClass*)` — returns `true` for `SKEL_/REINST_/TRASHCLASS_/HOTRELOADED_` prefixed classes. Tooling that walks reflection must skip these.

Plus default-value extraction helpers that return `FCk_PropertyDefaultValueLiteral` so each consumer can render the literal in its target language (C++ vs AS) correctly.

## Why this exists

Unreal's reflection API is low-level and has sharp edges — user-defined struct property names carry a `_<num>_<guid>` suffix, hot-reload leaves placeholder classes in `GetObjectsOfClass` results, and property-compat checks need to account for `FArrayProperty` / `FMapProperty` inner types. This module centralizes those quirks so callers don't reinvent them inconsistently.

## Depends on
`Macros/`, `Format/`, UE's `CoreUObject`.

## Used by
`CkAngelscriptGenerator`, `CkDynamic`, `CkDynamicEditor`, `CkK2Nodes`, `CkRecord`, `CkEcsEditor`, asset exporters. Any code that reads/writes properties on UDS instances goes through these helpers.

## Pitfalls

1. Don't compare UDS property **display names** directly — always sanitize first, or compare by GUID.
2. Always check `Is_PlaceholderClass` before queueing a `UClass*` for code-gen or asset-registry walks. Missing this causes "ghost" entries after Live Coding.
3. Property compatibility is not symmetric with respect to `const` and `ref` flags — if you're mapping, check both directions or call `Get_ArePropertiesDifferent` (which already handles both).

## See also
- `Format/README.md` — default-value literals get formatted per language.
- `CkDynamic/` / `CkAngelscriptGenerator/` — primary consumers; read their usage sites for idiomatic examples.
