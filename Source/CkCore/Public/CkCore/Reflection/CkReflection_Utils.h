#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkReflection_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Reflection returns a neutral literal; the caller targeting a specific language owns the wrapping.
UENUM()
enum class ECk_PropertyDefaultValueKind : uint8
{
    // Valid as-is in both C++ and AngelScript: numbers, bools, nullptr, Enum::Value, FTransform::Identity.
    RawLiteral,
    // Raw string content with no surrounding quotes. AngelScript wraps as "<value>".
    String,
    // Raw FName content with no surrounding quotes or prefix. AngelScript wraps as n"<value>".
    Name,
    // Raw FText content with no surrounding quotes. AngelScript wraps as FText::FromString("<value>").
    Text,
};

struct CKCORE_API FCk_PropertyDefaultValueLiteral
{
    ECk_PropertyDefaultValueKind _Kind = ECk_PropertyDefaultValueKind::RawLiteral;
    FString _Value;
};

// Single leaf-level override on a struct's CDO. DottedFieldPath runs from the outer struct to the
// differing field — "LocalRotationOffset" at top level, "Audio.Submix" on a nested struct.
struct CKCORE_API FCk_StructFieldOverride
{
    FString                         _DottedFieldPath;
    FCk_PropertyDefaultValueLiteral _Literal;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_Utils_Reflection_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Reflection_UE);

public:
    // UserDefined struct property names user the format "MyPropertyName_SomeNumber_SomeGuid"
    static auto
    Get_SanitizedUserDefinedPropertyName(
        const FProperty* InProperty) -> FString;

    static auto
    Get_PropertyBySanitizedName(
        UObject* InObject,
        const FString& InSanitizedPropertyName) -> FProperty*;

    static auto
    Get_UserDefinedPropertyGuid(
        const FProperty* InProperty) -> FString;

    static auto
    Get_ArePropertiesCompatible(
        const FProperty* InPropertyA,
        const FProperty* InPropertyB)-> bool;

    static auto
    Get_ArePropertiesDifferent(
        const TArray<FProperty*>& InPropertiesA,
        const TArray<FProperty*>& InPropertiesB) -> bool;

    static auto
    Get_ExposedPropertiesOfClass(
        const UClass* InClass) -> TArray<FProperty*>;

    static auto
    Get_IsDelegateProperty(
        const FProperty* InProperty) -> bool;

    // Returns true for UE reinstancing placeholder classes (SKEL_/REINST_/TRASHCLASS_/HOTRELOADED_)
    // that should be skipped by reflection-walking tooling (AS generators, class discovery).
    static auto
    Is_PlaceholderClass(
        const UClass* InClass) -> bool;

    // Returns true when the class is declared in an editor-only module. The owning plugin module's host
    // type is authoritative; engine modules fall back to a known-editor-modules set, then to a name
    // heuristic. Used by codegen tooling to decide whether emitted references need `#if Editor` guards.
    static auto
    Is_EditorOnlyClass(
        const UClass* InClass) -> bool;

    // AngelScript-specific escaping for emission into generated .as files.
    static auto
    Get_EscapedStringForAngelscript(
        const FString& InRaw) -> FString;

    // Converts a neutral default-value literal to its final AngelScript expression string,
    // applying language-specific quoting for String / Name / Text kinds. Returns empty string
    // for kinds with no representation.
    static auto
    Get_AngelscriptDefaultExpression(
        const struct FCk_PropertyDefaultValueLiteral& InLiteral) -> FString;

    // Neutral representation of the property's default value, read from InContainer (typically a CDO).
    // Structs emit a named constant when one exists, else a whole-struct field-by-field decomposition.
    // Empty TOptional means "not representable" — never a partial expression — so the caller omits the
    // initializer (containers, a valid FGameplayTag, a non-Identity FTransform, any struct with one
    // unrepresentable field). The returned Kind tells a language-specific caller how to quote the value.
    static auto
    Get_PropertyDefaultValueLiteral(
        const FProperty* InProperty,
        const void* InContainer) -> TOptional<FCk_PropertyDefaultValueLiteral>;

    // Leaf-level field overrides for a struct property whose CDO differs from its InitializeStruct
    // default; empty when fully at default. Drives field-assignment emission (`<Field>.<Path> = <Value>;`)
    // because AS rejects the positional ctor for any struct carrying a UObject* field. The recursion
    // descends only into nested structs that themselves carry one — others emit their own ctor at that path.
    static auto
    Get_StructFieldOverrides(
        const FStructProperty* InStructProperty,
        const void*            InContainer) -> TArray<FCk_StructFieldOverride>;

    // True when the struct contains at least one FObjectPropertyBase / FInterfaceProperty field anywhere
    // in its recursive decomposition. Drives the emission switch in the spawn-params generator.
    static auto
    Has_UObjectPointerField(
        const UScriptStruct* InStruct) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
