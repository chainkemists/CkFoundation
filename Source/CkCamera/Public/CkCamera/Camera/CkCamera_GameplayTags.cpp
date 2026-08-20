// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkCamera/Camera/CkCamera_GameplayTags.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_BoomArmPivotOffset, TEXT("Camera.Rig.BoomArmPivotOffset"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_BoomArmLength,      TEXT("Camera.Rig.BoomArmLength"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_FramingOffset,      TEXT("Camera.Rig.FramingOffset"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_FramingPitch,       TEXT("Camera.Rig.FramingPitch"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_FramingYaw,         TEXT("Camera.Rig.FramingYaw"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Rig_FixedBoomRotation,  TEXT("Camera.Rig.FixedBoomRotation"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Springs_GroupBaseLocationInterpSpeed, TEXT("Camera.Springs.GroupBaseLocationInterpSpeed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Springs_LookAtLocationInterpSpeed,    TEXT("Camera.Springs.LookAtLocationInterpSpeed"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Sensor_FOV,         TEXT("Camera.Sensor.FOV"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Sensor_AspectRatio, TEXT("Camera.Sensor.AspectRatio"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Sensor_OrthoWidth,  TEXT("Camera.Sensor.OrthoWidth"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Pitch_Speed,                TEXT("Camera.Noise.Pitch.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Pitch_Limits,               TEXT("Camera.Noise.Pitch.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Pitch_NormalSpeedRange,     TEXT("Camera.Noise.Pitch.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Pitch_OutOfRangeMultiplier, TEXT("Camera.Noise.Pitch.OutOfRangeMultiplier"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Yaw_Speed,                  TEXT("Camera.Noise.Yaw.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Yaw_Limits,                 TEXT("Camera.Noise.Yaw.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Yaw_NormalSpeedRange,       TEXT("Camera.Noise.Yaw.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Yaw_OutOfRangeMultiplier,   TEXT("Camera.Noise.Yaw.OutOfRangeMultiplier"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Roll_Speed,                 TEXT("Camera.Noise.Roll.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Roll_Limits,                TEXT("Camera.Noise.Roll.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Roll_NormalSpeedRange,      TEXT("Camera.Noise.Roll.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Noise_Roll_OutOfRangeMultiplier,  TEXT("Camera.Noise.Roll.OutOfRangeMultiplier"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Pitch_Speed,                TEXT("Camera.OrientationControl.Pitch.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Pitch_Limits,               TEXT("Camera.OrientationControl.Pitch.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Pitch_NormalSpeedRange,     TEXT("Camera.OrientationControl.Pitch.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Pitch_OutOfRangeMultiplier, TEXT("Camera.OrientationControl.Pitch.OutOfRangeMultiplier"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Yaw_Speed,                  TEXT("Camera.OrientationControl.Yaw.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Yaw_Limits,                 TEXT("Camera.OrientationControl.Yaw.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Yaw_NormalSpeedRange,       TEXT("Camera.OrientationControl.Yaw.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_OrientationControl_Yaw_OutOfRangeMultiplier,   TEXT("Camera.OrientationControl.Yaw.OutOfRangeMultiplier"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Pitch_Speed,                TEXT("Camera.AutoReorient.Pitch.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Pitch_Limits,               TEXT("Camera.AutoReorient.Pitch.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Pitch_NormalSpeedRange,     TEXT("Camera.AutoReorient.Pitch.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Pitch_OutOfRangeMultiplier, TEXT("Camera.AutoReorient.Pitch.OutOfRangeMultiplier"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Yaw_Speed,                  TEXT("Camera.AutoReorient.Yaw.Speed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Yaw_Limits,                 TEXT("Camera.AutoReorient.Yaw.Limits"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Yaw_NormalSpeedRange,       TEXT("Camera.AutoReorient.Yaw.NormalSpeedRange"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_Yaw_OutOfRangeMultiplier,   TEXT("Camera.AutoReorient.Yaw.OutOfRangeMultiplier"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_LookAtTargetLocationPitchOffset, TEXT("Camera.AutoReorient.LookAtTargetLocationPitchOffset"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_LookAtTargetLocationYawOffset,   TEXT("Camera.AutoReorient.LookAtTargetLocationYawOffset"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_LookAtCameraLocationPitchOffset, TEXT("Camera.AutoReorient.LookAtCameraLocationPitchOffset"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_AutoReorient_LookAtCameraLocationYawOffset,   TEXT("Camera.AutoReorient.LookAtCameraLocationYawOffset"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Collision_NumPredictionWhiskers,  TEXT("Camera.Collision.NumPredictionWhiskers"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Collision_MaxWhiskerAngle,        TEXT("Camera.Collision.MaxWhiskerAngle"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Collision_AdditionalOffsetFromHit, TEXT("Camera.Collision.AdditionalOffsetFromHit"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Collision_ClippingSpeed,          TEXT("Camera.Collision.ClippingSpeed"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_Collision_ReturningSpeed,         TEXT("Camera.Collision.ReturningSpeed"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_DepthOfField_Fstop,         TEXT("Camera.DepthOfField.Fstop"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_DepthOfField_FocalDistance, TEXT("Camera.DepthOfField.FocalDistance"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_DepthOfField_SensorWidth,   TEXT("Camera.DepthOfField.SensorWidth"));

// --------------------------------------------------------------------------------------------------------------------
