#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkEcsExt/Transform/CkTransform_Fragment.h>
#include <CkEcsExt/Transform/CkTransform_Fragment_Data.h>

#include <CkCore/Macros/CkMacros.h>

#include <CkCore/TypeTraits/CkTypeTraits.h>

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsExt/CkEcsExt_Log.h"
#include "CkNet/CkNet_Utils.h"

#include "CkTransform_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Transform_SyncFromActor;
    class FProcessor_Transform_HandleRequests;
    class FProcessor_Transform_SyncToActor;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_Transform_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transform_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Transform);

public:
    friend class ck::FProcessor_Transform_SyncFromActor;
    friend class ck::FProcessor_Transform_HandleRequests;
    friend class ck::FProcessor_Transform_SyncToActor;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Add Feature")
    static FCk_Handle_Transform
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Add Feature (AttachTo Unreal Component)")
    static FCk_Handle_Transform
    AddAndAttachToUnrealComponent(
        UPARAM(ref) FCk_Handle& InHandle,
        USceneComponent* InAttachTo,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Transform",
        DisplayName="[Ck][Transform] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Transform
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Transform",
        DisplayName="[Ck][Transform] Handle -> Transform Handle",
        meta = (CompactNodeTitle = "<AsTransform>", BlueprintAutocast))
    static FCk_Handle_Transform
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Transform Handle",
        Category = "Ck|Utils|Transform",
        meta = (CompactNodeTitle = "INVALID_TransformHandle", Keywords = "make"))
    static FCk_Handle_Transform
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location")
    static void
    Request_SetLocation(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Location Offset")
    static void
    Request_AddLocationOffset(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Rotation")
    static void
    Request_SetRotation(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Rotation Offset")
    static void
    Request_AddRotationOffset(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Scale")
    static void
    Request_SetScale(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Transform")
    static void
    Request_SetTransform(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Transform")
    static FTransform
    Get_EntityCurrentTransform(
        const FCk_Handle_Transform& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Location")
    static FVector
    Get_EntityCurrentLocation(
        const FCk_Handle_Transform& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Rotation")
    static FRotator
    Get_EntityCurrentRotation(
        const FCk_Handle_Transform& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Get Scale")
    static FVector
    Get_EntityCurrentScale(
        const FCk_Handle_Transform& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Bind To OnUpdate")
    static FCk_Handle_Transform
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Unbind From OnUpdate")
    static FCk_Handle_Transform
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

private:
    UFUNCTION(Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Add Feature (DEPRECATED)",
              meta=(DeprecatedFunction, DeprecationMessage = "Use the non-deprecated Add Feature instead"))
    static void
    DoAdd(
        UPARAM(ref) FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

private:
    static auto
    Request_TransformUpdated(
            FCk_Handle_Transform& InHandle) -> void;

    static auto
    Request_SetWorldTransformFastPath(
        USceneComponent* InSceneComp,
        const FTransform& InTransform) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

// Transform is a bit special and is one of the few Features that works on type-unsafe Entities as well
UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_Transform_TypeUnsafe_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transform_TypeUnsafe_UE);

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
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_TransformInterpolation_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TransformInterpolation_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_TransformInterpolation);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform|Interpolation",
              DisplayName="[Ck][TransformInterpolation] Add Feature")
    static FCk_Handle_TransformInterpolation
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Transform_Interpolation_Settings& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Transform|Interpolation",
              DisplayName="[Ck][TransformInterpolation] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Transform|Interpolation",
        DisplayName="[Ck][TransformInterpolation] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_TransformInterpolation
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Transform|Interpolation",
        DisplayName="[Ck][TransformInterpolation] Handle -> TransformInterpolation Handle",
        meta = (CompactNodeTitle = "<AsTransformInterpolation>", BlueprintAutocast))
    static FCk_Handle_TransformInterpolation
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid TransformInterpolation Handle",
        Category = "Ck|Utils|TransformInterpolation",
        meta = (CompactNodeTitle = "INVALID_TransformInterpolationHandle", Keywords = "make"))
    static FCk_Handle_TransformInterpolation
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform|Interpolation",
              DisplayName = "[Ck][TransformInterpolation] Request Set Interpolation Goal (Location)")
    static void
    Request_SetInterpolationGoal_LocationOffset(
        UPARAM(ref) FCk_Handle_TransformInterpolation& InHandle,
        FVector InOffset);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform|Interpolation",
              DisplayName = "[Ck][TransformInterpolation] Request Set Interpolation Goal (Rotation)")
    static void
    Request_SetInterpolationGoal_RotationOffset(
        UPARAM(ref) FCk_Handle_TransformInterpolation& InHandle,
        FRotator   InOffset);
};

// --------------------------------------------------------------------------------------------------------------------
