#pragma once

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkTransform_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Transform_SyncFromActor;
    class FProcessor_Transform_SyncFromMeshSocket;
    class FProcessor_Transform_SyncFromMeshSocket_SceneNode;
    class FProcessor_Transform_HandleRequests;
    class FProcessor_Transform_SyncToActor;
}

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Transform_OnAdded, const FCk_Handle_Transform&);
#endif

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Transform"))
class CKECSEXT_API UCk_Utils_Transform_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transform_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Transform);

public:
    friend class ck::FProcessor_Transform_SyncFromActor;
    friend class ck::FProcessor_Transform_SyncFromMeshSocket;
    friend class ck::FProcessor_Transform_SyncFromMeshSocket_SceneNode;
    friend class ck::FProcessor_Transform_HandleRequests;
    friend class ck::FProcessor_Transform_SyncToActor;

#if WITH_EDITOR
public:
    // Creation-time editor signal used by retained visualizers. Consumers coalesce the callback and
    // reconcile after composition completes; no per-frame polling is required.
    static auto Get_OnAdded() -> FCk_Transform_OnAdded&;
#endif

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
              DisplayName="[Ck][Transform] Create")
    static FCk_Handle_Transform
    Create(
        UPARAM(ref) FCk_Handle& InOwningEntity,
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

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName="[Ck][Transform] Add Feature (AttachTo Unreal Mesh Socket)")
    static FCk_Handle_Transform
    AddAndAttachToUnrealMesh(
        UPARAM(ref) FCk_Handle& InHandle,
        const UMeshComponent* InAttachTo,
        FName InSocketName,
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
    // Composite: enqueues a SetLocation and a SetRotation request. The completion delegate rides the
    // rotation request, which the drain handles after the location one, so it reports once for both.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location & Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetLocationAndRotation(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocationAndRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetLocation(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Location Offset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddLocationOffset(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetRotation(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Rotation Offset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddRotationOffset(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Force Refresh",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_ForceRefresh(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // A pending scale request is REPLACED rather than queued, so the superseded one completes with
    // Failed before the new one takes its slot.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Scale",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetScale(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Composite: enqueues location, rotation and scale requests. The completion delegate rides the
    // scale request, which the drain handles last, so it reports once for all three.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Transform",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetTransform(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

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
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Transform",
        DisplayName = "[Ck][Transform] Get Identity Transform",
        meta = (CompactNodeTitle="Identity"))
    static FTransform
    Get_IdentityMatrix();

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Bind To OnUpdate")
    static FCk_Handle_Transform
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

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

public:
    // Parallel-safe transform write — operates on fragment references directly.
    // Returns modified component flags. Caller is responsible for tagging FTag_Transform_Updated.
    static auto
    Apply_SetTransform_DirectWrite(
        ck::FFragment_Transform& InTransformFragment,
        ck::FFragment_Transform_Previous& InPrevTransformFragment,
        const FTransform& InNewTransform) -> ECk_TransformComponents;

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
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle"))
class CKECSEXT_API UCk_Utils_Transform_TypeUnsafe_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transform_TypeUnsafe_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location & Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetLocationAndRotation(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocationAndRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Location",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetLocation(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Location Offset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddLocationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetRotation(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Add Rotation Offset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddRotationOffset(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Force Refresh",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_ForceRefresh(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Scale",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetScale(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Transform",
              DisplayName = "[Ck][Transform] Request Set Transform",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetTransform(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

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
class CKECSEXT_API UCk_Utils_TransformInterpolation_UE : public UBlueprintFunctionLibrary
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
