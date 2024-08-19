#pragma once

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

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
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][Chaos] Add Feature")
    static FCk_Handle_GeometryCollection
    Add(
        UPARAM(ref) FCk_Handle_GeometryCollectionOwner& InHandle,
        const FCk_Fragment_GeometryCollection_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chaos",
              DisplayName="[Ck][Chaos] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][Chaos] Cast",
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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Apply Strain and Velocity")
    static FCk_Handle_GeometryCollection
    Request_ApplyStrainAndVelocity(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyStrain& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Apply AoE")
    static FCk_Handle_GeometryCollection
    Request_ApplyAoE(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyAoE& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chaos|GeometryCollection",
              DisplayName="[Ck][GeometryCollection] Request Crumble NonAnchoredClusters")
    static FCk_Handle_GeometryCollection
    Request_CrumbleNonAnchoredClusters(
        UPARAM(ref) FCk_Handle_GeometryCollection& InGeometryCollection);
};

// --------------------------------------------------------------------------------------------------------------------
