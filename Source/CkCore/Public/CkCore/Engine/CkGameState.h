#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/GameStateBase.h>

#include "CkGameState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_GameState_UE : public AGameStateBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_GameState_UE);
};

// --------------------------------------------------------------------------------------------------------------------
