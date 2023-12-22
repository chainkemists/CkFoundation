#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>

#include "CkNet_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKNET_API UCk_Net_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Net_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
