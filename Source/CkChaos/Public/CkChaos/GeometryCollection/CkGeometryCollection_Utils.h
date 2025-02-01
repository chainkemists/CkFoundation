#pragma once

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment_Data.h"
#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkGeometryCollection_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCHAOS_API UCk_Utils_GeometryCollection_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GeometryCollection_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GeometryCollection);

public:
    static auto
    Add(
        FCk_Handle_GeometryCollectionOwner& InHandle,
        const FCk_Fragment_GeometryCollection_ParamsData& InParams) -> FCk_Handle_GeometryCollection;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][GeometryCollection] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_GeometryCollection
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][GeometryCollection] Handle -> Chaos Handle",
        meta = (CompactNodeTitle = "<AsChaos>", BlueprintAutocast))
    static FCk_Handle_GeometryCollection
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid GeometryCollection Handle",
        Category = "Ck|Utils|GeometryCollection",
        meta = (CompactNodeTitle = "INVALID_GeometryCollectionHandle", Keywords = "make"))
    static FCk_Handle_GeometryCollection
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][GeometryCollection] Get Are All Clusters Anchored")
    static bool
    Get_AreAllClustersAnchored(
        const FCk_Handle_GeometryCollection& InGeometryCollection);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][GeometryCollection] Get GeometryCollection Component")
    static UGeometryCollectionComponent*
    Get_GeometryCollectionComponent(
        const FCk_Handle_GeometryCollection& InGeometryCollection);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Apply Radial Strain (NOT Replicated)")
    static FCk_Handle_GeometryCollection
    Request_ApplyRadialStrain(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyRadialStrain& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Crumble NonAnchoredClusters (NOT Replicated)")
    static FCk_Handle_GeometryCollection
    Request_CrumbleNonAnchoredClusters(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Remove All Anchors (NOT Replicated)")
    static FCk_Handle_GeometryCollection
    Request_RemoveAllAnchors(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection);
};

// --------------------------------------------------------------------------------------------------------------------
