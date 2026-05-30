#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include <Camera/CameraComponent.h>

#include "CkCamera_Component.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The output sink. Overrides UCameraComponent::GetCameraView to return the director entity's composed
// FMinimalViewInfo at the moment the default APlayerCameraManager asks for it (pull-based, race-free).
// No custom PlayerCameraManager required — the default PCM resolves the view target's active camera
// component and calls GetCameraView (AActor::CalcCamera -> UCameraComponent::GetCameraView).
// --------------------------------------------------------------------------------------------------------------------

UCLASS(ClassGroup = (Ck), meta = (BlueprintSpawnableComponent))
class CKCAMERA_API UCk_CameraComponent : public UCameraComponent
{
    GENERATED_BODY()

public:
    virtual void
    GetCameraView(
        float DeltaTime,
        FMinimalViewInfo& DesiredView) override;

public:
    auto
    Set_DirectorEntity(
        FCk_Handle_Camera InDirectorEntity) -> void;

    auto
    Get_DirectorEntity() const -> FCk_Handle_Camera { return _DirectorEntity; }

private:
    UPROPERTY(Transient)
    FCk_Handle_Camera _DirectorEntity;
};

// --------------------------------------------------------------------------------------------------------------------
