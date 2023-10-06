#pragma once

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/Entity/CkUnrealEntity_Fragment_Params.h"

#include "CkEcsConstructionScript_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// TODO: merging ReplicationDriver and EcsConstructionScript - to be called "EcsReplicationDriver_ActorComponent"

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ConstructionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ConstructionInfo);

private:
    UPROPERTY()
    FCk_Entity _OriginalEntity;

    UPROPERTY()
    TObjectPtr<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

public:
    CK_PROPERTY(_OriginalEntity);
    CK_PROPERTY_GET(_ConstructionScript);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ConstructionInfo, _ConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ReplicateObjects_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ReplicateObjects_Data);

private:
    UPROPERTY()
    TArray<TSubclassOf<UCk_Ecs_ReplicatedObject_UE>> _Objects;

    UPROPERTY()
    TArray<FName> _NetStableNames;

public:
    CK_PROPERTY(_Objects);
    CK_PROPERTY(_NetStableNames);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ReplicationData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ReplicationData);

private:
    UPROPERTY()
    FCk_EntityReplicationDriver_ConstructionInfo _ConstructionInfo;

    UPROPERTY()
    FCk_EntityReplicationDriver_ReplicateObjects_Data _ReplicatedObjectsData;

public:
    CK_PROPERTY_GET(_ConstructionInfo);
    CK_PROPERTY_GET(_ReplicatedObjectsData);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ReplicationData, _ConstructionInfo, _ReplicatedObjectsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_Request_ReplicationDriver_ReplicateEntity
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_ReplicationDriver_ReplicateEntity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_EntityReplicationDriver_ConstructionInfo _ConstructionInfo;

public:
    CK_PROPERTY_GET(_ConstructionInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_ReplicationDriver_ReplicateEntity, _ConstructionInfo);
};

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EcsConstructionScript_ConstructionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EcsConstructionScript_ConstructionInfo);

private:
    UPROPERTY(meta=(AllowPrivateAccess))
    TObjectPtr<AActor> _OutermostActor;

    UPROPERTY(meta=(AllowPrivateAccess))
    TSubclassOf<AActor> _ActorToReplicate;

    UPROPERTY(meta=(AllowPrivateAccess))
    TObjectPtr<APlayerController> _OwningPlayerController;

    UPROPERTY(meta=(AllowPrivateAccess))
    int32                 _OriginalOwnerEntity;

    UPROPERTY(meta=(AllowPrivateAccess))
    FTransform            _Transform;

public:
    CK_PROPERTY(_OutermostActor);
    CK_PROPERTY(_ActorToReplicate);
    CK_PROPERTY(_OwningPlayerController);
    CK_PROPERTY(_OriginalOwnerEntity);
    CK_PROPERTY(_Transform);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EcsConstructionScript_ReplicateObjects_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EcsConstructionScript_ReplicateObjects_Data);

private:
    UPROPERTY()
    TObjectPtr<AActor> _Owner;

    UPROPERTY()
    TArray<TSubclassOf<UCk_Ecs_ReplicatedObject_UE>> _Objects;

    UPROPERTY()
    TArray<FName> _NetStableNames;

public:
    CK_PROPERTY_GET(_Owner);
    CK_PROPERTY(_Objects);
    CK_PROPERTY(_NetStableNames);

    CK_PROPERTY_GET_NON_CONST(_Objects);
    CK_PROPERTY_GET_NON_CONST(_NetStableNames);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_EcsConstructionScript_ReplicateObjects_Data, _Owner);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EcsConstructionScript_Replication_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EcsConstructionScript_Replication_Data);

private:
    UPROPERTY()
    FCk_EcsConstructionScript_ConstructionInfo _ConstructionInfo;

    UPROPERTY()
    FCk_EcsConstructionScript_ReplicateObjects_Data _ReplicatedObjects;

public:
    CK_PROPERTY_GET(_ConstructionInfo);
    CK_PROPERTY_GET(_ReplicatedObjects);

    CK_DEFINE_CONSTRUCTORS(FCk_EcsConstructionScript_Replication_Data, _ConstructionInfo, _ReplicatedObjects);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       Blueprintable,
       BlueprintType,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKNET_API UCk_EcsConstructionScript_ActorComponent_UE : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsConstructionScript_ActorComponent_UE);

public:
    UCk_EcsConstructionScript_ActorComponent_UE();

public:
    UFUNCTION(Server, Reliable)
    void
    Request_ReplicateActor_OnServer(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest);

private:
    auto Request_ReplicateActor_OnClients(
        const FCk_EcsConstructionScript_ReplicateObjects_Data& InData,
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest) -> void;

protected:
    auto OnUnregister() -> void override;

protected:
    auto Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UCk_UnrealEntity_WithActor_PDA> _UnrealEntity;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient)
    FCk_Handle _Entity;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    FCk_EcsConstructionScript_Replication_Data _ReplicationData;

    UFUNCTION()
    void OnRep_ReplicationData();

    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

    bool _DoConstructCalled = false;
    bool _ReplicationDataReplicated = false;

public:
    CK_PROPERTY(_UnrealEntity);

public:
    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    // REPLICATION DRIVER STUFF - To be consolidated with the above after refactor of this ActorComponent

public:
    auto
    Request_Replicate(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo) -> void;

    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
};

// --------------------------------------------------------------------------------------------------------------------