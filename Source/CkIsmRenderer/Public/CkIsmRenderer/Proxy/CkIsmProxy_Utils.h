#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkIsmProxy_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_IsmProxy_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IsmProxy"))
class CKISMRENDERER_API UCk_Utils_IsmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmProxy);

public:
    friend class ck::FProcessor_IsmProxy_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy|Proxy",
              DisplayName="[Ck][IsmProxy] Add Feature")
    static FCk_Handle_IsmProxy
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_IsmProxy_Spec& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy|Proxy",
              DisplayName="[Ck][IsmProxy] Create")
    static FCk_Handle_IsmProxy
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FTransform& InInitialTransform,
        const FCk_IsmProxy_Spec& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Handle -> IsmProxy Handle",
              meta = (CompactNodeTitle = "<AsIsmProxy>", BlueprintAutocast))
    static FCk_Handle_IsmProxy
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid IsmProxy Handle",
        Category = "Ck|Utils|IsmProxy",
        meta = (CompactNodeTitle = "INVALID_IsmProxyHandle", Keywords = "make"))
    static FCk_Handle_IsmProxy
    Get_InvalidHandle() { return {}; };

public:
UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Request Set Custom Instance Data",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IsmProxy
    Request_SetCustomInstanceData(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomInstanceData& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Request Set Custom Instance Data Value",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IsmProxy
    Request_SetCustomInstanceDataValue(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Request Set Custom Primitive Data",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IsmProxy
    Request_SetCustomPrimitiveData(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomPrimitiveData& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|IsmProxy",
        DisplayName="[Ck][IsmProxy] Request Enable/Disable",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IsmProxy
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Local Location Offset")
    static FVector
    Get_LocalLocationOffset(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Local Rotation Offset")
    static FRotator
    Get_LocalRotationOffset(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Scale Multiplier")
    static FVector
    Get_ScaleMultiplier(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Mobility")
    static ECk_Mobility
    Get_Mobility(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Mesh Bounds")
    static FBoxSphereBounds
    Get_MeshBounds(
        const FCk_Handle_IsmProxy& InHandle,
        ECk_ScaledUnscaled InScaling = ECk_ScaledUnscaled::Scaled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName="[Ck][IsmProxy] Get Mesh")
    static UStaticMesh*
    Get_Mesh(
        const FCk_Handle_IsmProxy& InHandle);

    // ---- Entity-outline observability (see CkUsf/DESIGN_EntityOutlines.md; used by autotests/gyms) ----

    // True once the outline Sync processor mirrored this proxy's instance into a shadow ISM.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Is Outline Applied")
    static bool
    Get_IsOutlineApplied(
        const FCk_Handle_IsmProxy& InHandle);

    // Instance count of the shadow ISM this proxy's outline lives in (all outlined proxies sharing the
    // same renderer+preset). INDEX_NONE when no outline is applied.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Outline Shadow Instance Count")
    static int32
    Get_OutlineShadowInstanceCount(
        const FCk_Handle_IsmProxy& InHandle);

    // The proxy's CPU-authoritative per-instance custom data (what was last pushed to the ISM).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Custom Instance Data")
    static TArray<float>
    Get_CustomInstanceData(
        const FCk_Handle_IsmProxy& InHandle);

    // The custom data actually resident on this proxy's SHADOW instance. Must match
    // Get_CustomInstanceData for a WPO-animated material (CkVat) to silhouette the animated pose
    // rather than the bind pose. Empty when no outline is applied.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Outline Shadow Custom Data")
    static TArray<float>
    Get_OutlineShadowCustomData(
        const FCk_Handle_IsmProxy& InHandle);

    // The material on the shadow ISM at InSlot — it must be the source ISM's material, or the outline
    // pass runs a different vertex shader than the mesh it is meant to trace.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Outline Shadow Material")
    static UMaterialInterface*
    Get_OutlineShadowMaterial(
        const FCk_Handle_IsmProxy& InHandle,
        int32 InSlot = 0);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Relative Socket Transform")
    static FTransform
    Get_RelativeSocketTransform(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Relative Socket Location ")
    static FVector
    Get_RelativeSocketLocation(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Relative Socket Rotation")
    static FRotator
    Get_RelativeSocketRotation(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmProxy",
              DisplayName = "[Ck][IsmProxy] Get Relative Socket Scale")
    static FVector
    Get_RelativeSocketScale(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName);

private:
    static auto
    Request_NeedsInstanceAdded(
        FCk_Handle_IsmProxy& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
