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
              DisplayName="[Ck][AutoReorient] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AutoReorient_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Ensure Has Feature")
    static bool
    Ensure(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Get Policy")
    static ECk_AutoReorient_Policy
    Get_AutoReorientPolicy(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Request Start")
    static void
    Request_Start(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Request Stop")
    static void
    Request_Stop(
        UPARAM(ref) FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

