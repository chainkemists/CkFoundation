#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkReflection_Utils.generated.h"

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
};

// --------------------------------------------------------------------------------------------------------------------
