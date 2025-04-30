#pragma once

#include "CkResolverTarget_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

#include "CkResolverTarget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRESOLVER_API UCk_Utils_ResolverTarget_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ResolverTarget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ResolverTarget);

public:
    using RecordOfDataBundles_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDataBundles>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResolverTarget",
              DisplayName="[Ck][ResolverTarget] Add Feature")
    static FCk_Handle_ResolverTarget
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResolverTarget",
              DisplayName="[Ck][ResolverTarget] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // The ResolverTarget is destroyed when the Owner is destroyed OR after 1 frame if InLifetime == AfterOneFrame
    UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|ResolverTarget",
          DisplayName="[Ck][ResolverTarget] Create New ResolverTarget")
    static FCk_Handle_ResolverTarget
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    // Transient means that the onus of destroying the ResolverTarget is now on the user OR after 1 frame if InLifetime == AfterOneFrame
    UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|ResolverTarget",
          DisplayName="[Ck][ResolverTarget] Create New ResolverTarget (Transient)",
          meta = (WorldContext="InWorldContextObject"))
    static FCk_Handle_ResolverTarget
    Create_Transient(
        const FTransform& InTransform,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

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

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid ResolverTarget Handle",
        Category = "Ck|Utils|ResolverTarget",
        meta = (CompactNodeTitle = "INVALID_ResolverTargetHandle", Keywords = "make"))
    static FCk_Handle_ResolverTarget
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverTarget",
        DisplayName="[Ck][ResolverTarget] ForEach ResolverDataBundle",
        meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_ResolverDataBundle>
    ForEach_ResolverDataBundle(
        UPARAM(ref) FCk_Handle_ResolverTarget& InTarget,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_ResolverDataBundle(
        FCk_Handle_ResolverTarget& InTarget,
        const TFunction<void(FCk_Handle_ResolverDataBundle)>& InFunc) -> void;

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
