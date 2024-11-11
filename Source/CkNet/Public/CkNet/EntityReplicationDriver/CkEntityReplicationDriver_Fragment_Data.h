#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEntityReplicationDriver_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (
    HasNativeMake = "/Script/CkNet.Ck_Utils_EntityReplicationDriver_UE:Make_HandleReplicator",
    HasNativeBreak = "/Script/CkNet.Ck_Utils_EntityReplicationDriver_UE:Break_HandleReplicator"))
struct CKNET_API FCk_HandleReplicator
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_HandleReplicator);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated,
        meta=(AllowPrivateAccess))
    FCk_Handle _Handle;
    UPROPERTY()
    TWeakObjectPtr<class UCk_Fragment_EntityReplicationDriver_Rep> _Handle_RepObj;

public:
    auto
    NetSerialize(
        FArchive& Ar,
        class UPackageMap* Map,
        bool& bOutSuccess) -> bool;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_Handle_RepObj);

    CK_DEFINE_CONSTRUCTORS(FCk_HandleReplicator, _Handle, _Handle_RepObj);
};

template<>
struct TStructOpsTypeTraits<FCk_HandleReplicator> : public TStructOpsTypeTraitsBase2<FCk_HandleReplicator>
{
    enum
    { WithNetSerializer = true };
};

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

    // TODO: do we need the ConstructionScript Ptr if EntityBridge if the ReplicatedActor already has a reference to it?
    UPROPERTY()
    TObjectPtr<class UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

    UPROPERTY()
    TArray<TWeakObjectPtr<UCk_ReplicatedObject_UE>> _ReplicatedObjects;

public:
    CK_PROPERTY_GET(_ReplicatedActor);
    CK_PROPERTY_GET(_ConstructionScript);
    CK_PROPERTY(_ReplicatedObjects);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor, _ReplicatedActor, _ConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor);

    UPROPERTY()
    TObjectPtr<AActor> _OuterReplicatedActor;

    UPROPERTY()
    TSubclassOf<AActor> _NonReplicatedActor;

    UPROPERTY()
    int32 _OriginalEntity = -1;

    UPROPERTY()
    TObjectPtr<APlayerController> _OwningPlayerController;

    UPROPERTY()
    FVector _StartingLocation = FVector::Zero();

    UPROPERTY()
    TArray<TWeakObjectPtr<class UCk_ReplicatedObject_UE>> _ReplicatedObjects;
public:
    CK_PROPERTY_GET(_OuterReplicatedActor);
    CK_PROPERTY_GET(_NonReplicatedActor);
    CK_PROPERTY_GET(_OriginalEntity);
    CK_PROPERTY_GET(_OwningPlayerController);
    CK_PROPERTY(_StartingLocation);
    CK_PROPERTY(_ReplicatedObjects);

    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor,
        _OuterReplicatedActor, _NonReplicatedActor, _OriginalEntity, _OwningPlayerController);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_EntityReplicationDriver_ConstructionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_ConstructionInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    TScriptInterface<ICk_Entity_ConstructionScript_Interface> _ConstructionScript;

public:
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
    TArray<TWeakObjectPtr<class UCk_ReplicatedObject_UE>> _Objects;

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
struct CKNET_API FCk_EntityReplicationDriver_AbilityData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityReplicationDriver_AbilityData);

private:
    UPROPERTY()
    TObjectPtr<class UCk_Fragment_EntityReplicationDriver_Rep> _OwningEntityDriver;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_DataAsset_PDA> _AbilityScriptClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _AbilitySource;

    UPROPERTY()
    FCk_EntityReplicationDriver_ReplicateObjects_Data _ReplicatedObjectsData;

public:
    CK_PROPERTY(_OwningEntityDriver)
    CK_PROPERTY_GET(_AbilityScriptClass)
    CK_PROPERTY_GET(_AbilitySource)
    CK_PROPERTY_GET(_ReplicatedObjectsData)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_EntityReplicationDriver_AbilityData, _AbilityScriptClass, _AbilitySource, _ReplicatedObjectsData);
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

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_EntityReplicationDriver_OnReplicationComplete,
    FCk_Handle, InHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_EntityReplicationDriver_OnReplicationComplete_MC,
    FCk_Handle, InHandle);

// --------------------------------------------------------------------------------------------------------------------
