#pragma once

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment_Data.h"
#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGeometryCollectionOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCHAOS_API UCk_Utils_GeometryCollectionOwner_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GeometryCollectionOwner_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GeometryCollectionOwner);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Add Feature")
    static FCk_Handle_GeometryCollectionOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chaos",
              DisplayName="[Ck][GeometryCollectionOwner] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_GeometryCollectionOwner
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Handle -> GeometryCollectionOwner Handle",
        meta = (CompactNodeTitle = "<AsGeometryCollectionOwner>", BlueprintAutocast))
    static FCk_Handle_GeometryCollectionOwner
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid GeometryCollectionOwner Handle",
        Category = "Ck|Utils|GeometryCollectionOwner",
        meta = (CompactNodeTitle = "INVALID_GeometryCollectionOwnerHandle", Keywords = "make"))
    static FCk_Handle_GeometryCollectionOwner
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] For Each",
        meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_GeometryCollection>
    ForEach_GeometryCollection(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_GeometryCollection(
        FCk_Handle_GeometryCollectionOwner& InOwner,
        const TFunction<void(FCk_Handle_GeometryCollection)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request Crumble NonActive Clusters (Replicated)")
    static FCk_Handle_GeometryCollectionOwner
    Request_CrumbleNonActiveClusters(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request RemoveAllAnchors (Replicated)")
    static FCk_Handle_GeometryCollectionOwner
    Request_RemoveAllAnchors(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request Apply Radial Strain (Replicated)")
    static FCk_Handle_GeometryCollectionOwner
    Request_ApplyRadianStrain(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner,
        const FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
