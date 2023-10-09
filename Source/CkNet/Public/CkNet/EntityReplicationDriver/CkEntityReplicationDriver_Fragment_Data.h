#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"

#include "CkEntityReplicationDriver_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor);

private:
    UPROPERTY()
    TObjectPtr<AActor> _ReplicatedActor;

    UPROPERTY()
    TObjectPtr<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

    UPROPERTY()
    TArray<UCk_ReplicatedObject_UE*> _ReplicatedObjects;

public:
    CK_PROPERTY_GET(_ReplicatedActor);
    CK_PROPERTY_GET(_ConstructionScript);
    CK_PROPERTY(_ReplicatedObjects);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor, _ReplicatedActor, _ConstructionScript);
};

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
    TArray<UCk_ReplicatedObject_UE*> _Objects;

public:
    CK_PROPERTY_GET(_Objects);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ReplicateObjects_Data, _Objects);
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
    TObjectPtr<class UCk_Fragment_EntityReplicationDriver_Rep> _OwningEntityDriver;

    UPROPERTY()
    FCk_EntityReplicationDriver_ConstructionInfo _ConstructionInfo;

    UPROPERTY()
    FCk_EntityReplicationDriver_ReplicateObjects_Data _ReplicatedObjectsData;

public:
    CK_PROPERTY(_OwningEntityDriver);
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
