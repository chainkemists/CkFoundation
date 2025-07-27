#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <GameFramework/Character.h>
#include <Components/CapsuleComponent.h>

#include "CkCharacter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "ACharacter"))
class CKCORE_API UCk_Utils_Character_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Character_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure,
              DisplayName = "[Ck] Add Character Half-Height To Location",
              Category = "Ck|Utils|Character")
    static FVector
    Add_CharacterHalfHeightToLocation(
        ACharacter* InCharacter,
        FVector InLocation);

    UFUNCTION(BlueprintCallable, BlueprintPure,
              DisplayName = "[Ck] Subtract Character Half-Height From Location",
              Category = "Ck|Utils|Character")
    static FVector
    Subtract_CharacterHalfHeightFromLocation(
        ACharacter* InCharacter,
        FVector InLocation);

    UFUNCTION(BlueprintCallable, BlueprintPure,
              DisplayName = "[Ck] Add Capsule Half-Height To Location",
              Category = "Ck|Utils|Character")
    static FVector
    Add_CapsuleHalfHeightToLocation(
        UCapsuleComponent* InCapsule,
        FVector InLocation);

    UFUNCTION(BlueprintCallable, BlueprintPure,
              DisplayName = "[Ck] Subtract Capsule Half-Height From Location",
              Category = "Ck|Utils|Character")
    static FVector
    Subtract_CapsuleHalfHeightFromLocation(
        UCapsuleComponent* InCapsule,
        FVector InLocation);
};

// --------------------------------------------------------------------------------------------------------------------
