#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"

#include "Component/CkActorComponent.h"

#include "CkEcsConstructionScript_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUNREAL_API FCk_EcsConstructionScript_ConstructionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EcsConstructionScript_ConstructionInfo);

private:
    UPROPERTY(meta=(AllowPrivateAccess))
    AActor* _OutermostActor = nullptr;

    UPROPERTY(meta=(AllowPrivateAccess))
    TSubclassOf<AActor> _ActorToReplicate = nullptr;

    UPROPERTY(meta=(AllowPrivateAccess))
    APlayerController* _OwningPlayerController = nullptr;

    UPROPERTY(meta=(AllowPrivateAccess))
    FCk_ReplicatedObjects _ReplicatedObjects;

    UPROPERTY(meta=(AllowPrivateAccess))
    int32                 _OriginalOwnerEntity;

    UPROPERTY(meta=(AllowPrivateAccess))
    FTransform            _Transform;

public:
    CK_PROPERTY(_OutermostActor);
    CK_PROPERTY(_ActorToReplicate);
    CK_PROPERTY(_OwningPlayerController);
    CK_PROPERTY(_ReplicatedObjects);
    CK_PROPERTY(_OriginalOwnerEntity);
    CK_PROPERTY(_Transform);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKUNREAL_API UCk_EcsConstructionScript_ActorComponent : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsConstructionScript_ActorComponent);

public:
    UCk_EcsConstructionScript_ActorComponent();

public:
    UFUNCTION(Server, Reliable)
    void
    Request_ReplicateActor_OnServer(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_ReplicateActor_OnClients(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_ReplicateObject(
        AActor* InReplicatedOwner,
        TSubclassOf<UCk_Ecs_ReplicatedObject> InObject,
        FName InReplicatedName);

public:
    virtual auto
    BeginDestroy() -> void override;

protected:
    virtual auto
    Do_Construct_Implementation(const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UCk_UnrealEntity_Base_PDA> _UnrealEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
    FCk_Handle _Entity;

public:
    CK_PROPERTY(_UnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------