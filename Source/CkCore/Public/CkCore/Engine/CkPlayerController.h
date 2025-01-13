#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/PlayerController.h>

#include "CkPlayerController.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PlayerController_State : uint8
{
    Playing,
    Spectating,
    Inactive
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PlayerController_State);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PlayerController_Spectating_Policy : uint8
{
    // When going into Spectating mode, the spectator will default to a static free-cam mode
    Default,

    // When going into Spectating mode, immediately view the next valid player
    AutoSpectate,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PlayerController_Spectating_Policy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_PlayerController_UE : public APlayerController
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_PlayerController_UE);

public:
    auto BeginSpectatingState() -> void override;
    auto OnRep_Pawn() -> void override;
    auto AddCheats(bool InForce) -> void override;
    auto PostActorCreated() -> void override;
    auto ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass) -> void override;
protected:
    auto BeginPlay() -> void override;

public:
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly,
              Category = "Ck|PlayerController",
              DisplayName = "[Ck][PlayerController] Request Set Controller State")
    void
    Request_SetControllerState(
        ECk_PlayerController_State InState);

private:
    UPROPERTY(EditDefaultsOnly, Category = "ACk_PlayerController_UE")
    ECk_PlayerController_Spectating_Policy _SpectatingPolicy = ECk_PlayerController_Spectating_Policy::AutoSpectate;

    UPROPERTY(EditDefaultsOnly,
              Category = "ACk_PlayerController_UE|Development",
              DisplayName = "Add Multiplayer Cheats (In Development)")
    bool _AddMultiplayerCheats = false;

private:
    auto DoSetControllerState_Playing() -> void;
    auto DoSetControllerState_Spectating() -> void;
    auto DoSetControllerState_Inactive() -> void;
    auto DoViewNextPlayer() -> void;
    auto DoViewPreviousPlayer() -> void;

public:
    CK_PROPERTY_GET(_SpectatingPolicy);
};

// --------------------------------------------------------------------------------------------------------------------
