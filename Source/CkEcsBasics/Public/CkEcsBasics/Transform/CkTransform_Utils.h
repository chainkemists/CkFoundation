#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkEcsBasics/Transform/CkTransform_Fragment.h>
#include <CkEcsBasics/Transform/CkTransform_Fragment_Params.h>

#include <CkCore/Macros/CkMacros.h>

#include <CkCore/TypeTraits/CkTypeTraits.h>

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkNet/CkNet_Utils.h"

#include "CkTransform_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSBASICS_API UCk_Utils_Transform_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transform_UE);

public:
    template <typename T_ConstOrNonConst = ck::type_traits::NonConst>
    static auto
    Add(
        FCk_Handle InHandle,
        FTransform InInitialTransform,
        FCk_Transform_Interpolation_Settings InSettings) -> void;

    template <typename T_ConstOrNonConst = void>
    static auto
    Has(
        FCk_Handle InHandle) -> bool;

    template <typename T_ConstOrNonConst = void>
    static auto
    Ensure(
        FCk_Handle InHandle) -> bool;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Set Entity Location")
    static void
    Request_SetLocation(
        FCk_Handle                               InHandle,
        const FCk_Request_Transform_SetLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Add Entity Location Offset")
    static void
    Request_AddLocationOffset(
        FCk_Handle                                     InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Set Entity Rotation")
    static void
    Request_SetRotation(
        FCk_Handle                               InHandle,
        const FCk_Request_Transform_SetRotation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Add Entity Rotation Offset")
    static void
    Request_AddRotationOffset(
        FCk_Handle                                     InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Set Entity Scale")
    static void
    Request_SetScale(
        FCk_Handle                             InHandle,
        const FCk_Request_Transform_SetScale&  InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Set Entity Transform")
    static void
    Request_SetTransform(
        FCk_Handle                                InHandle,
        const FCk_Request_Transform_SetTransform& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Request Set Interpolation Goal [LOCATION]")
    static void
    Request_SetInterpolationGoal_Offset(
        FCk_Handle InHandle,
        FVector    InOffset);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform")
    static FTransform
    Get_EntityCurrentTransform(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform")
    static FVector
    Get_EntityCurrentLocation(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform")
    static FRotator
    Get_EntityCurrentRotation(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform")
    static FVector
    Get_EntityCurrentScale(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Bind To OnTransformUpdate")
    static void
    BindTo_OnUpdate(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "Bind To OnTransformUpdate")
    static void
    UnbindFrom_OnUpdate(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName="Add Transform")
    static void
    DoAdd(
        FCk_Handle InHandle,
        FTransform InInitialTransform,
        FCk_Transform_Interpolation_Settings InSettings);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName="Has Transform")
    static bool
    DoHas(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName="Ensure Has Transform")
    static bool
    DoEnsure(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ConstOrNonConst>
auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle InHandle,
        FTransform InInitialTransform,
        FCk_Transform_Interpolation_Settings InSettings)
    -> void
{
    InHandle.Add<ck::TFragment_Transform<T_ConstOrNonConst>>(InInitialTransform);
    InHandle.Add<ck::FFragment_Transform_Params>(FCk_Transform_ParamsData{}.Set_InterpolationSettings(std::move(InSettings)));

    TryAddReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle);
}

template <typename T_ConstOrNonConst>
auto
    UCk_Utils_Transform_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    if (std::is_same_v<T_ConstOrNonConst, void>)
    {
        return InHandle.Has_Any<ck::FFragment_Transform_Current, ck::FFragment_ImmutableTransform_Current>();
    }

    return InHandle.Has_Any<ck::TFragment_Transform<T_ConstOrNonConst>>();
}

template <typename T_ConstOrNonConst>
auto
    UCk_Utils_Transform_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has<T_ConstOrNonConst>(InHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
