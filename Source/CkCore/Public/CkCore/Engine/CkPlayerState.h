#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/PlayerState.h>

#include "CkPlayerState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_PlayerState_UE : public APlayerState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_PlayerState_UE);
};

// --------------------------------------------------------------------------------------------------------------------
