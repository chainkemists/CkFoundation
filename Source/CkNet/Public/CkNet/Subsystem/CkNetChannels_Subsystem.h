#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Fragment_Data.h"

#include "CkNetChannels_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_NetChannels")
class CKNET_API UCk_NetChannels_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_NetChannels_Subsystem_UE);

public:
    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

    auto
    ShouldCreateSubsystem(
        UObject* InOuter) const -> bool override;
};

// --------------------------------------------------------------------------------------------------------------------
