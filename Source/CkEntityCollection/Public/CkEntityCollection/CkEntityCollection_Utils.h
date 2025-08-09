#pragma once

#include "CkEntityCollection/CkEntityCollection_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEntityCollection_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityCollection_StorePrevious;
    class FProcessor_EntityCollection_HandleRequests;
    class FProcessor_EntityCollection_FireSignals;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EntityCollection"))
class CKENTITYCOLLECTION_API UCk_Utils_EntityCollection_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityCollection_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityCollection);

private:
    struct RecordOfEntityCollections_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntityCollections> {};
    struct EntityCollections_RecordOfEntities_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_EntityCollections_RecordOfEntities> {};
    struct EntityCollections_RecordOfEntities_Previous_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_EntityCollections_RecordOfEntities_Previous> {};

public:
    friend class ck::FProcessor_EntityCollection_StorePrevious;
    friend class ck::FProcessor_EntityCollection_HandleRequests;
    friend class ck::FProcessor_EntityCollection_FireSignals;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][EntityCollection] Add New EntityCollection")
    static FCk_Handle_EntityCollection
    Add(
        UPARAM(ref) FCk_Handle& InEntityCollectionOwnerEntity,
        const FCk_Fragment_EntityCollection_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[Ck][EntityCollection] Add Multiple New EntityCollections")
    static TArray<FCk_Handle_EntityCollection>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InEntityCollectionOwnerEntity,
        const FCk_Fragment_MultipleEntityCollection_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InEntityCollectionOwnerEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityCollection",
        DisplayName="[Ck][EntityCollection] Has Any EntityCollection")
    static bool
    Has_Any(
        const FCk_Handle& InEntityCollectionOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[Ck][EntityCollection] Try Get EntityCollection")
    static FCk_Handle_EntityCollection
    TryGet_EntityCollection(
        const FCk_Handle& InEntityCollectionOwnerEntity,
        UPARAM(meta = (Categories = "EntityCollection")) FGameplayTag InEntityCollectionName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[Ck][EntityCollection] Get All Entities In Collection",
              meta = (Keywords = "content"))
    static FCk_EntityCollection_Content
    Get_EntitiesInCollection(
        const FCk_Handle_EntityCollection& InEntityCollectionHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[Ck][EntityCollection] Get Num Entities In Collection")
    static int32
    Get_NumEntitiesInCollection(
        const FCk_Handle_EntityCollection& InEntityCollectionHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityCollection",
        DisplayName="[Ck][EntityCollection] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EntityCollection
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityCollection",
        DisplayName="[Ck][EntityCollection] Handle -> EntityCollection Handle",
        meta = (CompactNodeTitle = "<AsEntityCollection>", BlueprintAutocast))
    static FCk_Handle_EntityCollection
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid EntityCollection Handle",
        Category = "Ck|Utils|EntityCollection",
        meta = (CompactNodeTitle = "INVALID_EntityCollectionHandle", Keywords = "make"))
    static FCk_Handle_EntityCollection
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[Ck][EntityCollection] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_EntityCollection>
    ForEach_EntityCollection(
        UPARAM(ref) FCk_Handle& InEntityCollectionOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_EntityCollection(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const TFunction<void(FCk_Handle_EntityCollection)>& InFunc) -> void;

    static auto
    ForEach_EntityCollection_If(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const TFunction<void(FCk_Handle_EntityCollection)>& InFunc,
        const TFunction<bool(FCk_Handle_EntityCollection)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][EntityCollection] Request Add Entities To Collection")
    static FCk_Handle_EntityCollection
    Request_AddEntities(
        UPARAM(ref) FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Request_EntityCollection_AddEntities& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][EntityCollection] Request Remove Entities From Collection")
    static FCk_Handle_EntityCollection
    Request_RemoveEntities(
        UPARAM(ref) FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Request_EntityCollection_RemoveEntities& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[EntityCollection] Bind to OnCollectionUpdated")
    static FCk_Handle_EntityCollection
    BindTo_OnCollectionUpdated(
        UPARAM(ref) FCk_Handle_EntityCollection& InEntityCollectionHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityCollection_OnCollectionUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityCollection",
              DisplayName="[EntityCollection] Unbind From OnCollectionUpdated")
    static FCk_Handle_EntityCollection
    UnbindFrom_OnCollectionUpdated(
        UPARAM(ref) FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Delegate_EntityCollection_OnCollectionUpdated& InDelegate);

private:
    static auto
    Request_CollectionUpdated(
        FCk_Handle_EntityCollection& InEntityCollectionHandle) -> void;

    static auto
    Request_StorePreviousCollection(
        FCk_Handle_EntityCollection& InEntityCollectionHandle) -> void;

    static auto
    Request_TryReplicateEntityCollection(
        FCk_Handle_EntityCollection& InEntityCollectionHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
