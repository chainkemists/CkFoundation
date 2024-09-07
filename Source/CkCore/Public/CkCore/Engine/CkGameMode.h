#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/GameModeBase.h>

#include "CkGameMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_GameMode_UE : public AGameModeBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_GameMode_UE);

protected:
    auto AllowCheats(
        APlayerController* InPlayerController) -> bool override;

private:
    UPROPERTY(EditDefaultsOnly,
              Category = "ACk_GameMode_UE|Development",
              DisplayName = "Allow Multiplayer Cheats (In Development)")
    bool _AllowMultiplayerCheats = false;
};

// --------------------------------------------------------------------------------------------------------------------
