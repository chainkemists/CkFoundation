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

    // Returns a neutral representation of the default value for the given property, read from
    // InContainer (typically a CDO). Returns an empty TOptional when the default is not representable
    // by this helper (unknown struct types, containers, user-authored non-identity struct defaults, etc.).
    // Supported: bool, numeric (int/float/double), FString, FName, FText, FEnumProperty, FByteProperty-with-enum,
    // FObjectPropertyBase (emits nullptr), and a known-good FStructProperty allowlist:
    //   FTransform -> FTransform::Identity
    //   FVector    -> FVector::ZeroVector / FVector::OneVector
    //   FRotator   -> FRotator::ZeroRotator
    //   FGameplayTag -> FGameplayTag()
    // The returned FCk_PropertyDefaultValueLiteral carries a Kind so callers that target a specific
    // language (e.g. AngelScript) can wrap String / Name / Text content as needed.
    static auto
    Get_PropertyDefaultValueLiteral(
        const FProperty* InProperty,
        const void* InContainer) -> TOptional<FCk_PropertyDefaultValueLiteral>;
};

// --------------------------------------------------------------------------------------------------------------------
