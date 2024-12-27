#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

#include "CkIsmProxy_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_IsmProxy_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKISMRENDERER_API UCk_Utils_IsmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmProxy);

public:
    friend class ck::FProcessor_IsmProxy_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy|Proxy",
              DisplayName="[Ck][IsmProxy] Add Feature")
    static FCk_Handle_IsmProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Handle -> IsmProxy Handle",
              meta = (CompactNodeTitle = "<AsIsmProxy>", BlueprintAutocast))
    static FCk_Handle_IsmProxy
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Request Set Custom Data")
    static FCk_Handle_IsmProxy
    Request_SetCustomData(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomData& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Request Set Custom Data Value")
    static FCk_Handle_IsmProxy
    Request_SetCustomDataValue(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomDataValue& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Associated Renderer Name")
    static FGameplayTag
    Get_RendererName(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Local Location Offset")
    static FVector
    Get_LocalLocationOffset(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Local Rotation Offset")
    static FRotator
    Get_LocalRotationOffset(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Scale Multiplier")
    static FVector
    Get_ScaleMultiplier(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Mobility")
    static ECk_Mobility
    Get_Mobility(
        const FCk_Handle_IsmProxy& InHandle);

private:
    static auto
    Request_NeedsInstanceAdded(
        FCk_Handle_IsmProxy& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
