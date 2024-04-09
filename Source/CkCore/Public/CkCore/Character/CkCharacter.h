#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <GameFramework/Character.h>

#include "CkCharacter.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_Character_UE : public ACharacter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_Character_UE);
};

// --------------------------------------------------------------------------------------------------------------------
