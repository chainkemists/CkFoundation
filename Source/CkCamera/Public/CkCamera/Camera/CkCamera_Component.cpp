#include "CkCamera_Component.h"

#include "CkCamera/Camera/CkCamera_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_CameraComponent::
    GetCameraView(
        float DeltaTime,
        FMinimalViewInfo& DesiredView)
{
    if (ck::IsValid(_DirectorEntity) && _DirectorEntity.Has<ck::FFragment_Camera_Current>())
    {
        DesiredView = _DirectorEntity.Get<ck::FFragment_Camera_Current>().Get_ViewInfo();
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
