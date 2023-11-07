#pragma once

#include "CkCamera/CameraShake/CkCameraShake_Fragment.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
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

private:
    struct RecordOfCameraShakes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraShakes> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Add New Camera Shake")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_CameraShake_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Add Multiple New Camera Shake")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleCameraShake_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Has Camera Shake")
    static bool
    Has(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Has Any Camera Shake")
    static bool
    Has_Any(
        FCk_Handle InCameraShakeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Ensure Has Camera Shake")
    static bool
    Ensure(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Ensure Has Any Camera Shake")
    static bool
    Ensure_Any(
        FCk_Handle InCameraShakeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="Get All Camera Shakes")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InCameraShakeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake")
    static void
    Request_PlayCameraShakeOnTarget(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayOnTarget& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake")
    static void
    Request_PlayCameraShakeAtLocation(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayAtLocation& InRequest);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

