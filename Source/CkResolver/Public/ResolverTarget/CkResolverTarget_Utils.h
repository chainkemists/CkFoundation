#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

#include "CkResolverTarget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRESOLVER_API UCk_Utils_ResolverTarget_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ResolverTarget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(UCk_Utils_ResolverTarget_UE);

public:
    using RecordOfDataBundles_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDataBundles>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResolverTarget",
              DisplayName="[Ck][ResolverTarget] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResolverTarget",
              DisplayName="[Ck][ResolverTarget] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ResolverTarget
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] Handle -> ResolverTarget Handle",
        meta = (CompactNodeTitle = "<AsResolverTarget>", BlueprintAutocast))
    static FCk_Handle_ResolverTarget
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] Request Initiate New Resolution")
    static FCk_Handle_ResolverTarget
    Request_InitiateNewResolution(
        UPARAM(ref) FCk_Handle_ResolverTarget& InResolverTarget,
        const FCk_Request_ResolverTarget_InitiateNewResolution& InRequest,
        FCk_Delegate_ResolverTarget_OnNewResolverDataBundle InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] Bind to OnNewResolverDataBundle")
    static FCk_Handle_ResolverTarget
    BindTo_OnNewResolverDataBundle(
        UPARAM(ref) FCk_Handle_ResolverTarget& InResolverTarget,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverTarget_OnNewResolverDataBundle& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] Unbind From OnNewResolverDataBundle")
    static FCk_Handle_ResolverTarget
    UnbindFrom_OnNewResolverDataBundle(
        UPARAM(ref) FCk_Handle_ResolverTarget& InResolverTarget,
        const FCk_Delegate_ResolverTarget_OnNewResolverDataBundle& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
