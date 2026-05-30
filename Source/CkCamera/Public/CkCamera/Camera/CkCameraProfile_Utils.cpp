#include "CkCameraProfile_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    BlendInto(
        FCk_CameraProfile& InOutProfile,
        const FCk_CameraProfile& InTarget,
        float InAlpha)
    -> void
{
    const auto Alpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);

    if (Alpha <= 0.0f)
    { return; }

    constexpr auto DominanceThreshold = 0.5f;
    const auto TargetDominates = Alpha >= DominanceThreshold;

    // ---- Sensor (continuous) ----
    {
        auto Sensor = InOutProfile.Get_Sensor();
        const auto& Target = InTarget.Get_Sensor();
        Sensor.Set_FOV(FMath::Lerp(Sensor.Get_FOV(), Target.Get_FOV(), Alpha));
        Sensor.Set_AspectRatio(FMath::Lerp(Sensor.Get_AspectRatio(), Target.Get_AspectRatio(), Alpha));
        if (TargetDominates)
        { Sensor.Set_ConstrainAspectRatio(Target.Get_ConstrainAspectRatio()); }
        InOutProfile.Set_Sensor(Sensor);
    }

    // ---- Rig (continuous) ----
    {
        auto Rig = InOutProfile.Get_Rig();
        const auto& Target = InTarget.Get_Rig();
        Rig.Set_BoomArmLength(FMath::Lerp(Rig.Get_BoomArmLength(), Target.Get_BoomArmLength(), Alpha));
        Rig.Set_BoomArmPivotOffset(FMath::Lerp(Rig.Get_BoomArmPivotOffset(), Target.Get_BoomArmPivotOffset(), Alpha));
        Rig.Set_FramingOffset(FMath::Lerp(Rig.Get_FramingOffset(), Target.Get_FramingOffset(), Alpha));
        Rig.Set_FramingPitch(FMath::Lerp(Rig.Get_FramingPitch(), Target.Get_FramingPitch(), Alpha));
        Rig.Set_FramingYaw(FMath::Lerp(Rig.Get_FramingYaw(), Target.Get_FramingYaw(), Alpha));
        InOutProfile.Set_Rig(Rig);
    }

    // ---- Depth of Field (continuous) ----
    {
        auto DoF = InOutProfile.Get_DepthOfField();
        const auto& Target = InTarget.Get_DepthOfField();
        DoF.Set_Fstop(FMath::Lerp(DoF.Get_Fstop(), Target.Get_Fstop(), Alpha));
        DoF.Set_FocalDistance(FMath::Lerp(DoF.Get_FocalDistance(), Target.Get_FocalDistance(), Alpha));
        DoF.Set_SensorWidth(FMath::Lerp(DoF.Get_SensorWidth(), Target.Get_SensorWidth(), Alpha));
        InOutProfile.Set_DepthOfField(DoF);
    }

    // ---- Discrete feature blocks (adopt the target's once it dominates the blend) ----
    if (TargetDominates)
    {
        InOutProfile.Set_Noise(InTarget.Get_Noise());

        InOutProfile.Set_HasOrientationControl(InTarget.Get_HasOrientationControl());
        InOutProfile.Set_OrientationControl(InTarget.Get_OrientationControl());

        InOutProfile.Set_HasAutoReorient(InTarget.Get_HasAutoReorient());
        InOutProfile.Set_AutoReorient(InTarget.Get_AutoReorient());

        InOutProfile.Set_HasCollision(InTarget.Get_HasCollision());
        InOutProfile.Set_Collision(InTarget.Get_Collision());

        InOutProfile.Set_UsePostProcess(InTarget.Get_UsePostProcess());
    }
}

// --------------------------------------------------------------------------------------------------------------------
