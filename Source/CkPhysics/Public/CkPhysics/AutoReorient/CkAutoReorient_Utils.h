#pragma once

#include "CkAutoReorient_Fragment_Data.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkAutoReorient_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_AutoReorient"))
class CKPHYSICS_API UCk_Utils_AutoReorient_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AutoReorient_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AutoReorient);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Add Feature")
    static FCk_Handle_AutoReorient
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_AutoReorient_Spec& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Create")
    static FCk_Handle_AutoReorient
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_AutoReorient_Spec& InParams);

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
              DisplayName="[Ck][AutoReorient] Request Start",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Start(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AutoReorient",
              DisplayName="[Ck][AutoReorient] Request Stop",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Stop(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

