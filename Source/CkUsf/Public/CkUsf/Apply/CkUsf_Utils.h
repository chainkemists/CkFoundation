#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkUsf_Utils.generated.h"

class UCkUsf_LookDefinition;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUSF_API UCk_Utils_Usf_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    CK_GENERATED_BODY(UCk_Utils_Usf_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Get Look Master Material")
    static UMaterialInterface*
    Get_LookMasterMaterial(
        const UCkUsf_LookDefinition* InLook);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Create MID For Look")
    static UMaterialInstanceDynamic*
    Create_MID_ForLook(
        const UCkUsf_LookDefinition* InLook,
        UObject* InOuter);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Set Scalar")
    static void
    Set_Scalar(
        UMaterialInstanceDynamic* InMID, FName InName, float InValue);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Set Vector")
    static void
    Set_Vector(
        UMaterialInstanceDynamic* InMID, FName InName, FLinearColor InValue);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Set Texture")
    static void
    Set_Texture(
        UMaterialInstanceDynamic* InMID, FName InName, UTexture* InValue);
};
