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
              DisplayName="[Ck][CameraShake] Add New Camera Shake")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_CameraShake_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Add Multiple New Camera Shakes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleCameraShake_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Has Camera Shake")
    static bool
    Has(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Has Any Camera Shake")
    static bool
    Has_Any(
        FCk_Handle InCameraShakeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Ensure Has Camera Shake")
    static bool
    Ensure(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Ensure Has Any Camera Shake")
    static bool
    Ensure_Any(
        FCk_Handle InCameraShakeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Request Play On Target")
    static void
    Request_PlayOnTarget(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayOnTarget& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraShake",
              DisplayName="[Ck][CameraShake] Request Play At Location")
    static void
    Request_PlayAtLocation(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayAtLocation& InRequest);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

