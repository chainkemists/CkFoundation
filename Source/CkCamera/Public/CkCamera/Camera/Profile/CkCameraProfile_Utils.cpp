#include "CkCameraProfile_Utils.h"

#include "CkCamera/Camera/Profile/CkCameraProfile_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    Create(
        FCk_Handle_Camera& InOwner)
    -> FCk_Handle_CameraProfile
{
    auto ProfileEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(ProfileEntity, TEXT("CameraProfile"));

    ProfileEntity.Add<ck::FFragment_CameraProfile_Rig>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_Springs>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_Sensor>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_Noise>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_OrientationControl>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_AutoReorient>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_Collision>();
    ProfileEntity.Add<ck::FFragment_CameraProfile_DepthOfField>();

    auto Profile = Cast(ProfileEntity);
    DoWrite(Profile, FCk_CameraProfile{});
    return Profile;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    Get_Profile(
        const FCk_Handle_CameraProfile& InProfile)
    -> FCk_CameraProfile
{
    auto Out = FCk_CameraProfile{};

    Out.Set_Rig    (InProfile.Get<ck::FFragment_CameraProfile_Rig>()    .Get_Rig());
    Out.Set_Springs(InProfile.Get<ck::FFragment_CameraProfile_Springs>().Get_Springs());
    Out.Set_Sensor (InProfile.Get<ck::FFragment_CameraProfile_Sensor>() .Get_Sensor());
    Out.Set_Noise  (InProfile.Get<ck::FFragment_CameraProfile_Noise>()  .Get_Noise());

    const auto& OrientationControl = InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>();
    Out.Set_HasOrientationControl(OrientationControl.Get_Enabled());
    Out.Set_OrientationControl(OrientationControl.Get_OrientationControl());

    const auto& AutoReorient = InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>();
    Out.Set_HasAutoReorient(AutoReorient.Get_Enabled());
    Out.Set_AutoReorient(AutoReorient.Get_AutoReorient());

    const auto& Collision = InProfile.Get<ck::FFragment_CameraProfile_Collision>();
    Out.Set_HasCollision(Collision.Get_Enabled());
    Out.Set_Collision(Collision.Get_Collision());

    const auto& DepthOfField = InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>();
    Out.Set_UsePostProcess(DepthOfField.Get_Enabled());
    Out.Set_DepthOfField(DepthOfField.Get_DepthOfField());

    return Out;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    Get_Rig(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_Rig
{ return InProfile.Get<ck::FFragment_CameraProfile_Rig>().Get_Rig(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_Rig(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Rig& InRig) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Rig>().Set_Rig(InRig); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_Springs(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_Springs
{ return InProfile.Get<ck::FFragment_CameraProfile_Springs>().Get_Springs(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_Springs(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Springs& InSprings) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Springs>().Set_Springs(InSprings); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_Sensor(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_Sensor
{ return InProfile.Get<ck::FFragment_CameraProfile_Sensor>().Get_Sensor(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_Sensor(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Sensor& InSensor) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Sensor>().Set_Sensor(InSensor); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_Noise(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_Noise
{ return InProfile.Get<ck::FFragment_CameraProfile_Noise>().Get_Noise(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_Noise(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Noise& InNoise) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Noise>().Set_Noise(InNoise); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_OrientationControl(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_OrientationControl
{ return InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>().Get_OrientationControl(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_OrientationControl(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_OrientationControl& InOrientationControl) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>().Set_OrientationControl(InOrientationControl); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_HasOrientationControl(const FCk_Handle_CameraProfile& InProfile) -> bool
{ return InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>().Get_Enabled(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_HasOrientationControl(FCk_Handle_CameraProfile& InProfile, bool InEnabled) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>().Set_Enabled(InEnabled); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_AutoReorient(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_AutoReorient
{ return InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>().Get_AutoReorient(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_AutoReorient(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_AutoReorient& InAutoReorient) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>().Set_AutoReorient(InAutoReorient); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_HasAutoReorient(const FCk_Handle_CameraProfile& InProfile) -> bool
{ return InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>().Get_Enabled(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_HasAutoReorient(FCk_Handle_CameraProfile& InProfile, bool InEnabled) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>().Set_Enabled(InEnabled); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_Collision(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_Collision
{ return InProfile.Get<ck::FFragment_CameraProfile_Collision>().Get_Collision(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_Collision(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Collision& InCollision) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Collision>().Set_Collision(InCollision); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_HasCollision(const FCk_Handle_CameraProfile& InProfile) -> bool
{ return InProfile.Get<ck::FFragment_CameraProfile_Collision>().Get_Enabled(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_HasCollision(FCk_Handle_CameraProfile& InProfile, bool InEnabled) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_Collision>().Set_Enabled(InEnabled); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_DepthOfField(const FCk_Handle_CameraProfile& InProfile) -> FCk_CameraProfile_DepthOfField
{ return InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>().Get_DepthOfField(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_DepthOfField(FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_DepthOfField& InDepthOfField) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>().Set_DepthOfField(InDepthOfField); return InProfile; }

auto
    UCk_Utils_CameraProfile_UE::
    Get_UsePostProcess(const FCk_Handle_CameraProfile& InProfile) -> bool
{ return InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>().Get_Enabled(); }

auto
    UCk_Utils_CameraProfile_UE::
    Set_UsePostProcess(FCk_Handle_CameraProfile& InProfile, bool InEnabled) -> FCk_Handle_CameraProfile
{ InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>().Set_Enabled(InEnabled); return InProfile; }

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    DoWrite(
        FCk_Handle_CameraProfile& InProfile,
        const FCk_CameraProfile& InProfileValue)
    -> void
{
    InProfile.Get<ck::FFragment_CameraProfile_Rig>()    .Set_Rig(InProfileValue.Get_Rig());
    InProfile.Get<ck::FFragment_CameraProfile_Springs>().Set_Springs(InProfileValue.Get_Springs());
    InProfile.Get<ck::FFragment_CameraProfile_Sensor>() .Set_Sensor(InProfileValue.Get_Sensor());
    InProfile.Get<ck::FFragment_CameraProfile_Noise>()  .Set_Noise(InProfileValue.Get_Noise());

    auto& OrientationControl = InProfile.Get<ck::FFragment_CameraProfile_OrientationControl>();
    OrientationControl.Set_Enabled(InProfileValue.Get_HasOrientationControl());
    OrientationControl.Set_OrientationControl(InProfileValue.Get_OrientationControl());

    auto& AutoReorient = InProfile.Get<ck::FFragment_CameraProfile_AutoReorient>();
    AutoReorient.Set_Enabled(InProfileValue.Get_HasAutoReorient());
    AutoReorient.Set_AutoReorient(InProfileValue.Get_AutoReorient());

    auto& Collision = InProfile.Get<ck::FFragment_CameraProfile_Collision>();
    Collision.Set_Enabled(InProfileValue.Get_HasCollision());
    Collision.Set_Collision(InProfileValue.Get_Collision());

    auto& DepthOfField = InProfile.Get<ck::FFragment_CameraProfile_DepthOfField>();
    DepthOfField.Set_Enabled(InProfileValue.Get_UsePostProcess());
    DepthOfField.Set_DepthOfField(InProfileValue.Get_DepthOfField());
}

auto
    UCk_Utils_CameraProfile_UE::
    Reset(
        FCk_Handle_CameraProfile& InProfile)
    -> void
{
    DoWrite(InProfile, FCk_CameraProfile{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraProfile_UE::
    BlendInto(
        FCk_Handle_CameraProfile& InOutProfile,
        const FCk_CameraProfile& InTarget,
        float InAlpha)
    -> FCk_Handle_CameraProfile
{
    auto Profile = Get_Profile(InOutProfile);
    BlendInto(Profile, InTarget, InAlpha);
    DoWrite(InOutProfile, Profile);

    return InOutProfile;
}

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

    // ---- Springs (continuous location-lag) ----
    {
        auto Springs = InOutProfile.Get_Springs();
        const auto& Target = InTarget.Get_Springs();
        Springs.Set_GroupBaseLocationInterpSpeed(FMath::Lerp(
            Springs.Get_GroupBaseLocationInterpSpeed(), Target.Get_GroupBaseLocationInterpSpeed(), Alpha));
        Springs.Set_LookAtLocationInterpSpeed(FMath::Lerp(
            Springs.Get_LookAtLocationInterpSpeed(), Target.Get_LookAtLocationInterpSpeed(), Alpha));
        InOutProfile.Set_Springs(Springs);
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

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CameraProfile_UE, FCk_Handle_CameraProfile, ck::FFragment_CameraProfile_Sensor);

// --------------------------------------------------------------------------------------------------------------------
