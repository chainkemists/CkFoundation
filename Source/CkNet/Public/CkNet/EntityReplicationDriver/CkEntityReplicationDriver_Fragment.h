#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEntityReplicationDriver_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

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

UCLASS(Blueprintable)
class CKNET_API UCk_Fragment_EntityReplicationDriver_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityReplicationDriver_Rep);

private:
    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

public:
    UFUNCTION(Server, Reliable)
    void
    Request_ReplicateEntity(
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo);

private:
    UFUNCTION()
    void
    OnRep_ReplicationData();

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    TArray<FCk_EntityReplicationDriver_ReplicationData> _ReplicationData;

    int32 _ReplicateFrom = 0;
};

// --------------------------------------------------------------------------------------------------------------------
