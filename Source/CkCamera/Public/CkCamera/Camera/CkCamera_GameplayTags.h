// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------
// Native gameplay tags naming the per-camera tuner attributes (one per FCk_CameraProfile leaf). The camera
// materializer (UCk_Utils_Camera_UE::Add) labels each tuner attribute with its tag; reference the tag VARIABLE,
// never the raw string. FloatRange leaves (Limits / NormalSpeedRange) are single Float-with-MinMax attributes.
// --------------------------------------------------------------------------------------------------------------------

// ---- Rig ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_BoomArmPivotOffset);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_BoomArmLength);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_FramingOffset);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_FramingPitch);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_FramingYaw);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Rig_FixedBoomRotation);

// ---- Springs ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Springs_GroupBaseLocationInterpSpeed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Springs_LookAtLocationInterpSpeed);

// ---- Sensor ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Sensor_FOV);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Sensor_AspectRatio);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Sensor_OrthoWidth);

// ---- Noise (Pitch / Yaw / Roll) ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Pitch_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Pitch_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Pitch_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Pitch_OutOfRangeMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Yaw_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Yaw_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Yaw_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Yaw_OutOfRangeMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Roll_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Roll_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Roll_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Noise_Roll_OutOfRangeMultiplier);

// ---- Orientation Control (Pitch / Yaw) ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Pitch_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Pitch_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Pitch_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Pitch_OutOfRangeMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Yaw_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Yaw_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Yaw_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_OrientationControl_Yaw_OutOfRangeMultiplier);

// ---- Auto Reorient (Pitch / Yaw + look-at offsets) ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Pitch_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Pitch_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Pitch_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Pitch_OutOfRangeMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Yaw_Speed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Yaw_Limits);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Yaw_NormalSpeedRange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_Yaw_OutOfRangeMultiplier);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_LookAtTargetLocationPitchOffset);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_LookAtTargetLocationYawOffset);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_LookAtCameraLocationPitchOffset);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_AutoReorient_LookAtCameraLocationYawOffset);

// ---- Collision ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Collision_NumPredictionWhiskers);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Collision_MaxWhiskerAngle);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Collision_AdditionalOffsetFromHit);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Collision_ClippingSpeed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_Collision_ReturningSpeed);

// ---- Depth Of Field ----
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_DepthOfField_Fstop);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_DepthOfField_FocalDistance);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_DepthOfField_SensorWidth);

// --------------------------------------------------------------------------------------------------------------------
