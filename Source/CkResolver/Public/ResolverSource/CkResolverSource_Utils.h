#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"
#include "ResolverDataBundle/CkResolverDataBundle_Fragment_Data.h"

#include "CkResolverSource_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRESOLVER_API UCk_Utils_ResolverSource_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ResolverSource_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ResolverSource);

public:
    using RecordOfDataBundles_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDataBundles>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResolverSource",
              DisplayName="[Ck][ResolverSource] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_ResolverSource_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResolverSource",
              DisplayName="[Ck][ResolverSource] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverSource",
        DisplayName="[Ck][ResolverSource] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ResolverSource
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverSource",
        DisplayName="[Ck][ResolverSource] Handle -> ResolverDataBundle Handle",
        meta = (CompactNodeTitle = "<AsResolverSource>", BlueprintAutocast))
    static FCk_Handle_ResolverSource
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverSource",
        DisplayName="[Ck][ResolverSource] Request Initiate New Resolution")
    static FCk_Handle_ResolverSource
    Request_InitiateNewResolution(
        UPARAM(ref) FCk_Handle_ResolverSource& InResolverSource,
        const FCk_Request_ResolverSource_InitiateNewResolution& InRequest,
        FCk_Delegate_ResolverSource_OnNewResolverDataBundle InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverSource",
        DisplayName="[Ck][ResolverSource] Bind to OnNewResolverDataBundle")
    static FCk_Handle_ResolverSource
    BindTo_OnNewResolverDataBundle(
        UPARAM(ref) FCk_Handle_ResolverSource& InResolverSource,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverSource_OnNewResolverDataBundle& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverSource",
        DisplayName="[Ck][ResolverSource] Unbind From OnNewResolverDataBundle")
    static FCk_Handle_ResolverSource
    UnbindFrom_OnNewResolverDataBundle(
        UPARAM(ref) FCk_Handle_ResolverSource& InResolverSource,
        const FCk_Delegate_ResolverSource_OnNewResolverDataBundle& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
