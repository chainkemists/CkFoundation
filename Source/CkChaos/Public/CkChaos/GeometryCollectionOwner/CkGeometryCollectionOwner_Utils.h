#pragma once

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment_Data.h"
#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkNet/CkNet_Utils.h"

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
        meta = (CompactNodeTitle = "<AsChaos>", BlueprintAutocast))
    static FCk_Handle_GeometryCollectionOwner
    DoCastChecked(
        FCk_Handle InHandle);

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
        DisplayName="[Ck][GeometryCollectionOwner] Request Crumble NonActive Clusters")
    static FCk_Handle_GeometryCollectionOwner
    Request_CrumbleNonActiveClusters(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request RemoveAllAnchors")
    static FCk_Handle_GeometryCollectionOwner
    Request_RemoveAllAnchors(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request Crumble NonActive Clusters and RemoveAllAnchors")
    static FCk_Handle_GeometryCollectionOwner
    Request_CrumbleNonActiveClustersAndRemoveAllAnchors(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request Apply Strain and Velocity (From DataAsset)")
    static FCk_Handle_GeometryCollectionOwner
    Request_ApplyStrainAndVelocity(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner,
        const FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection|Owner",
        DisplayName="[Ck][GeometryCollectionOwner] Request Apply AoE (From DataAsset)")
    static FCk_Handle_GeometryCollectionOwner
    Request_ApplyAoE(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InGeometryCollectionOwner,
        const FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
