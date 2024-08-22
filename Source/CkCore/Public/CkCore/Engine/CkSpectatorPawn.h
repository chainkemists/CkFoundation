#pragma once

#include <CoreMinimal.h>
#include <GameFramework/SpectatorPawn.h>

#include "CkSpectatorPawn.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_SpectatorPawn_UE : public ASpectatorPawn
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|SpectatorPawn",
              DisplayName = "[Ck][SpectatorPawn] Request View Next Player")
    void
    Request_ViewNextPlayer();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|SpectatorPawn",
              DisplayName = "[Ck][SpectatorPawn] Request View Previous Player")
    void
    Request_ViewPreviousPlayer();
};

// --------------------------------------------------------------------------------------------------------------------
