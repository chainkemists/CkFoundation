#include "CkGameplayCamera_Component.h"

#include "CkCamera/GameplayCamera/CkGameplayCamera_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_GameplayCameraComponent::
    GetCameraView(
        float DeltaTime,
        FMinimalViewInfo& DesiredView)
{
    if (ck::IsValid(_DirectorEntity) && _DirectorEntity.Has<ck::FFragment_GameplayCamera_Current>())
    {
        DesiredView = _DirectorEntity.Get<ck::FFragment_GameplayCamera_Current>().Get_ViewInfo();
        return;
    }

    Super::GetCameraView(DeltaTime, DesiredView);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameplayCameraComponent::
    Set_DirectorEntity(
        FCk_Handle_GameplayCamera InDirectorEntity)
    -> void
{
    _DirectorEntity = InDirectorEntity;
}

// --------------------------------------------------------------------------------------------------------------------
