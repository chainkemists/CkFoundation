#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/GameInstance.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <CoreMinimal.h>

#include "CkGame_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameStatus
{
    NotInGame,
    InPIE,
    InGame,
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Game_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Game_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              meta     = (WorldContext="InWorldContextObject"))
    static ECk_GameStatus
    Get_GameStatus(const UObject* InWorldContextObject, bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              meta     = (WorldContext="InWorldContextObject"))
    static bool
    Get_IsInGame(const UObject* InWorldContextObject, bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              meta     = (WorldContext="InWorldContextObject"))
    static bool
    Get_IsPIE(const UObject* InWorldContextObject, bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils")
    static UWorld*
    Get_WorldForObject(const UObject* InContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              meta     = (WorldContext="InWorldContextObject"))
    static UGameInstance*
    Get_GameInstance(const UObject* InWorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------
