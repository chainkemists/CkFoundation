#include "CkCamera_Component.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_CameraComponent::
    GetCameraView(
        float DeltaTime,
        FMinimalViewInfo& DesiredView)
{
    // The stored director handle can legitimately be invalid (not yet wired / destroyed), so this guard stays.
    if (ck::IsValid(_DirectorEntity))
    {
        DesiredView = UCk_Utils_Camera_UE::Get_ViewInfo(_DirectorEntity);
        return;
    }

    Super::GetCameraView(DeltaTime, DesiredView);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraComponent::
    Set_DirectorEntity(
        FCk_Handle_Camera InDirectorEntity)
    -> void
{
    _DirectorEntity = InDirectorEntity;
}

// --------------------------------------------------------------------------------------------------------------------
