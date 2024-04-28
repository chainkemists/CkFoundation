#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <CoreMinimal.h>

#include "CkGame_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_PlayerState_UE;
class UGameInstance;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameStatus : uint8
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
              DisplayName = "[Ck] Get Game Status",
              meta     = (WorldContext="InWorldContextObject"))
    static ECk_GameStatus
    Get_GameStatus(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get Is In Game",
              meta     = (WorldContext="InWorldContextObject"))
    static bool
    Get_IsInGame(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get Is PIE",
              meta     = (WorldContext="InWorldContextObject"))
    static bool
    Get_IsPIE(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get World For Object",
              meta     = (WorldContext="InContextObject"))
    static UWorld*
    Get_WorldForObject(
        const UObject* InContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get Game Instance",
              meta     = (WorldContext="InWorldContextObject"))
    static UGameInstance*
    Get_GameInstance(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get Primary Player State (As Client)",
              meta     = (WorldContext="InWorldContextObject"))
    static ACk_PlayerState_UE*
    Get_PrimaryPlayerState_AsClient(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Game|Utils",
              DisplayName = "[Ck] Get Primary Player Controller",
              meta     = (WorldContext="InWorldContextObject"))
    static APlayerController*
    Get_PrimaryPlayerController(
        const UObject* InWorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------
