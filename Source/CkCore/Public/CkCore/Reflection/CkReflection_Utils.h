#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkReflection_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How a default-value literal should be interpreted by the consumer. Reflection returns a neutral
// representation; callers that target a specific language (e.g. AngelScript) are responsible for
// wrapping String / Name / Text values in their language-specific syntax.
UENUM()
enum class ECk_PropertyDefaultValueKind : uint8
{
    // Literal expression, valid as-is in both C++ and AngelScript (numbers, bools, nullptr, Enum::Value,
    // whitelisted struct literals like FTransform::Identity).
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

// Single leaf-level override on a struct's CDO. DottedFieldPath is the path from the
// outer struct to the differing field, e.g. "LocalRotationOffset" for a top-level
// field, or "Audio.Submix" for a field on a nested struct. Used by Get_StructFieldOverrides
// to drive field-assignment-style emission (sidesteps the AS positional-ctor `<null handle>`
// trap on structs with UObject* fields).
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

    // Returns true when the class is declared in an editor-only module. The owning plugin
    // module's host type (Editor/EditorNoCommandlet/EditorAndProgram/UncookedOnly) is
    // authoritative when the module is described by a plugin descriptor; engine modules fall
    // back to a known-editor-modules set, then a name heuristic (contains "Editor"). Used by
    // codegen tooling to decide whether emitted references need `#if Editor` guards.
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

    // Returns a neutral representation of the default value for the given property, read from
    // InContainer (typically a CDO). Returns an empty TOptional when the default is not
    // representable (e.g. containers, valid FGameplayTag, non-Identity FTransform).
    //
    // Primitives: bool, int8/16/32/64, uint16/32/64, float, double, FString, FName, FText,
    //             FEnumProperty, FByteProperty-with-enum, FObjectPropertyBase (-> nullptr).
    //
    // Structs: named constants are emitted for idiomatic output —
    //   FTransform::Identity, FVector::ZeroVector/OneVector, FRotator::ZeroRotator, FGameplayTag().
    //   For all other structs, a general field-by-field decomposition is attempted: if every
    //   non-parm UPROPERTY field is itself representable, emits StructName(expr1, ...) or
    //   StructName() when all fields are at their InitializeStruct defaults. Any field whose
    //   type is not representable causes the whole struct to return empty TOptional.
    //
    // The returned FCk_PropertyDefaultValueLiteral carries a Kind so callers that target a
    // specific language (e.g. AngelScript) can wrap String / Name / Text content as needed.
    static auto
    Get_PropertyDefaultValueLiteral(
        const FProperty* InProperty,
        const void* InContainer) -> TOptional<FCk_PropertyDefaultValueLiteral>;

    // Returns the leaf-level field overrides for a struct property whose CDO differs from
    // its InitializeStruct default. Used by the EntityScript spawn-params generator to emit
    // field-assignment-style ctor bodies (`<Field>.<Path> = <Value>;`) instead of the
    // positional-ctor expression (`<Type>(arg1, ..., nullptr)`) that AS rejects when the
    // struct contains any UObject* field. Empty when the struct is fully at default.
    //
    // Recursion: when a nested struct also contains a UObject* field AND has its own diffs,
    // the recursion descends into it to build longer dotted paths. When a nested struct has
    // diffs but contains NO UObject* field, the recursion stops and emits the nested struct's
    // positional-ctor expression at that path — safe because the nested ctor takes no nullptr.
    static auto
    Get_StructFieldOverrides(
        const FStructProperty* InStructProperty,
        const void*            InContainer) -> TArray<FCk_StructFieldOverride>;

    // True when the struct contains at least one FObjectPropertyBase / FInterfaceProperty
    // field anywhere in its recursive decomposition. Drives the emission switch in the
    // EntityScript spawn-params generator.
    static auto
    Has_UObjectPointerField(
        const UScriptStruct* InStruct) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
