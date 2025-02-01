#pragma once

#include "CkCamera/CameraShake/CkCameraShake_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkCameraShake_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCAMERA_API UCk_Utils_CameraShake_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CameraShake_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CameraShake);

private:
    struct RecordOfCameraShakes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraShakes> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][CameraShake] Add New CameraShake")
    static FCk_Handle_CameraShake
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_CameraShake_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Add Multiple New CameraShakes")
    static TArray<FCk_Handle_CameraShake>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleCameraShake_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CameraShake",
        DisplayName="[Ck][CameraShake] Has Any CameraShake")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Try Get CameraShake")
    static FCk_Handle_CameraShake
    TryGet_CameraShake(
        const FCk_Handle& InCameraShakeOwnerEntity,
        UPARAM(meta = (Categories = "CameraShake")) FGameplayTag InCameraShakeName);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|CameraShake",
        DisplayName="[Ck][CameraShake] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CameraShake
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CameraShake",
        DisplayName="[Ck][CameraShake] Handle -> CameraShake Handle",
        meta = (CompactNodeTitle = "<AsCameraShake>", BlueprintAutocast))
    static FCk_Handle_CameraShake
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid CameraShake Handle",
        Category = "Ck|Utils|CameraShake",
        meta = (CompactNodeTitle = "INVALID_CameraShakeHandle", Keywords = "make"))
    static FCk_Handle_CameraShake
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_CameraShake>
    ForEach_CameraShake(
        UPARAM(ref) FCk_Handle& InCameraShakeOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_CameraShake(
        FCk_Handle& InCameraShakeOwnerEntity,
        const TFunction<void(FCk_Handle_CameraShake)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][CameraShake] Request Play On Target")
    static void
    Request_PlayOnTarget(
        UPARAM(ref) FCk_Handle_CameraShake& InCameraShakeHandle,
        const FCk_Request_CameraShake_PlayOnTarget& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][CameraShake] Request Play At Location")
    static void
    Request_PlayAtLocation(
        UPARAM(ref) FCk_Handle_CameraShake& InCameraShakeHandle,
        const FCk_Request_CameraShake_PlayAtLocation& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------

