#pragma once

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEntityReplicationChannel_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_EntityReplicationChannel_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityReplicationChannel_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityReplicationChannel);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityReplicationChannel",
              DisplayName="[Ck][EntityReplicationChannel] Add Feature")
    static FCk_Handle_EntityReplicationChannel
    Add(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityReplicationChannel",
              DisplayName="[Ck][EntityReplicationChannel] Get Channel Actor")
    static ACk_EcsChannel_Actor_UE*
    Get_ChannelActor(
        const FCk_Handle_EntityReplicationChannel& InEntityReplicationChannel);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityReplicationChannel",
              DisplayName="[Ck][EntityReplicationChannel] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityReplicationChannel",
        DisplayName="[Ck][EntityReplicationChannel] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EntityReplicationChannel
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityReplicationChannel",
        DisplayName="[Ck][EntityReplicationChannel] Handle -> EntityReplicationChannel Handle",
        meta = (CompactNodeTitle = "<AsEntityReplicationChannel>", BlueprintAutocast))
    static FCk_Handle_EntityReplicationChannel
    DoCastChecked(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_EntityReplicationChannelOwner_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityReplicationChannelOwner_UE);

private:
    struct RecordOfEntityReplicationChannels_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntityReplicationChannels> {};

public:
    static void
    AddNewChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity,
        FCk_Handle_EntityReplicationChannel& InEcsChannel);

public:
    static auto
    Get_NextAvailableEcsChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity) -> FCk_Handle_EntityReplicationChannel;

    static auto
    ForEach_EntityReplicationChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity,
        const TFunction<void(FCk_Handle_EntityReplicationChannel&)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

