#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"

#include "Component/CkActorComponent.h"

#include "CkConstructionScript_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKUNREAL_API UCk_EcsConstructionScript_ActorComponent : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsConstructionScript_ActorComponent);

public:
    virtual auto BeginDestroy() -> void override;

protected:
    virtual auto Do_Construct_Implementation(const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UCk_UnrealEntity_Base_PDA> _UnrealEntity;

    UPROPERTY(Transient)
    FCk_Handle _Entity;

public:
    CK_PROPERTY(_UnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------