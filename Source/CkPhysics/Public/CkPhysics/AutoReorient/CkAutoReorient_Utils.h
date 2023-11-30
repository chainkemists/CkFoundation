#pragma once

#include "CkAutoReorient_Fragment_Data.h"

#include "CkAutoReorient_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_AutoReorient_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AutoReorient_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="Add AutoReorient")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AutoReorient_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="Has AutoReorient")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="Ensure Has AutoReorient")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient")
    static ECk_AutoReorient_Policy
    Get_AutoReorientPolicy(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="Request Start AutoReorient")
    static void
    Request_Start(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="Request Stop AutoReorient")
    static void
    Request_Stop(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

