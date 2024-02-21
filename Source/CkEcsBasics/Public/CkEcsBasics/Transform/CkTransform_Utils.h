#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkEcsBasics/Transform/CkTransform_Fragment.h>
#include <CkEcsBasics/Transform/CkTransform_Fragment_Data.h>

#include <CkCore/Macros/CkMacros.h>

#include <CkCore/TypeTraits/CkTypeTraits.h>

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/CkEcsBasics_Log.h"
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
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        const FCk_Transform_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates) -> void;

    template <typename T_ConstOrNonConst = void>
    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

    template <typename T_ConstOrNonConst = void>
    static auto
    Ensure(
        const FCk_Handle& InHandle) -> bool;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location")
    static void
    Request_SetLocation(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Location Offset")
    static void
    Request_AddLocationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Rotation")
    static void
    Request_SetRotation(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Rotation Offset")
    static void
    Request_AddRotationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Scale")
    static void
    Request_SetScale(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Transform")
    static void
    Request_SetTransform(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Interpolation Goal (Location)")
    static void
    Request_SetInterpolationGoal_LocationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        FVector    InOffset);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Interpolation Goal (Rotation)")
    static void
    Request_SetInterpolationGoal_RotationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        FRotator   InOffset);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Transform")
    static FTransform
    Get_EntityCurrentTransform(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Location")
    static FVector
    Get_EntityCurrentLocation(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Rotation")
    static FRotator
    Get_EntityCurrentRotation(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Scale")
    static FVector
    Get_EntityCurrentScale(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Bind To OnUpdate")
    static void
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Unbind From OnUpdate")
    static void
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Add Feature")
    static void
    DoAdd(
        UPARAM(ref) FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        const FCk_Transform_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Has Feature")
    static bool
    DoHas(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Ensure Has Feature")
    static bool
    DoEnsure(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ConstOrNonConst>
auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        const FCk_Transform_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    InHandle.Add<ck::TFragment_Transform<T_ConstOrNonConst>>(InInitialTransform);
    InHandle.Add<ck::FFragment_Transform_Params>(InParams);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_basics::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    TryAddReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle);
}

template <typename T_ConstOrNonConst>
auto
    UCk_Utils_Transform_UE::
    Has(
        const FCk_Handle& InHandle)
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
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has<T_ConstOrNonConst>(InHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
