#pragma once

#include "CkWorld/Public/CkWorld/CkWorld.h"

#include "CkWorldActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKWORLD_API ACk_World_Actor_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_World_Actor_UE);

public:
    friend class UCk_World_GameInstance_Subsystem_UE;

public:
    using FEcsWorldType = ck::FEcsWorld;

public:
    ACk_World_Actor_UE();

protected:
    virtual auto Tick(float DeltaSeconds) -> void override;

public:
    auto Initialize(ETickingGroup InTickingGroup) -> void;

private:
    TOptional<FEcsWorldType> _EcsWorld;
};

