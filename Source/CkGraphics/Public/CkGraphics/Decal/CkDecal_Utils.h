#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkGraphics/Decal/CkDecal_Fragment_Data.h"

#include <Engine/DecalActor.h>

#include "CkDecal_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKGRAPHICS_API UCk_Utils_Decal_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Decal_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Decal);

private:
    static FCk_Handle_Decal
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        UDecalComponent* InDecalComponent,
        const FCk_Fragment_Decal_ParamsData& InParams);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Decal] Create New Decal (From Actor)")
    static FCk_Handle_Decal
    Create_FromActor(
        const FCk_Handle& InAnyValidHandle,
        const TSubclassOf<ADecalActor> InDecalActorClass,
        const FTransform& InTransform);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Decal] Create New Decal")
    static FCk_Handle_Decal
    Create(
        const FCk_Handle& InAnyValidHandle,
        const FTransform& InTransform,
        const FCk_Fragment_Decal_ParamsData& InDecalParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Decal",
              DisplayName="[Ck][Decal] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Decal",
        DisplayName="[Ck][Decal] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Decal
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Decal",
        DisplayName="[Ck][Decal] Handle -> Decal Handle",
        meta = (CompactNodeTitle = "<AsDecal>", BlueprintAutocast))
    static FCk_Handle_Decal
    DoCastChecked(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
