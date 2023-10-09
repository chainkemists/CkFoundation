#pragma once

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"

#include "CkEcsConstructionScript_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       Blueprintable,
       BlueprintType,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKUNREAL_API UCk_EcsConstructionScript_ActorComponent_UE : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsConstructionScript_ActorComponent_UE);

public:
    UCk_EcsConstructionScript_ActorComponent_UE();

protected:
    auto OnUnregister() -> void override;

protected:
    auto Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

public:
    // Temporary function for Obsidian Toggle
    UFUNCTION(BlueprintImplementableEvent)
    bool ShouldConstruct() const;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UCk_UnrealEntity_WithActor_PDA> _UnrealEntity;

public:
    CK_PROPERTY(_UnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------