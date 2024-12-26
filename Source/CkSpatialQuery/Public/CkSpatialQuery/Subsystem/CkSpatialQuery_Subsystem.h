#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <edyn/edyn.hpp>

#include "CkSpatialQuery_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKSPATIALQUERY_API UCk_SpatialQuery_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SpatialQuery_Subsystem_UE);

public:
    auto
        Initialize(
            FSubsystemCollectionBase& InCollection)
            -> void override;

    auto
        Tick(
            float InDeltaTime)
            -> void override;

    auto
        Deinitialize()
            -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

private:
    edyn::init_config _Config;
    ck::FEcsWorld _EcsWorld;

public:
    CK_PROPERTY_GET(_EcsWorld);
};

// --------------------------------------------------------------------------------------------------------------------
