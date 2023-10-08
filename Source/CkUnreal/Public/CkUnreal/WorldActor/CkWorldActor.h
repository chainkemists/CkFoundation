#pragma once

#include "CkEcs/World/CkEcsWorld.h"

#include <GameFramework/Info.h>

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkWorldActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKUNREAL_API ACk_World_Actor_UE : public ACk_World_Actor_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_World_Actor_UE);

public:
    friend class UCk_EcsWorld_Subsystem_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    virtual auto Initialize(ETickingGroup InTickingGroup) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
