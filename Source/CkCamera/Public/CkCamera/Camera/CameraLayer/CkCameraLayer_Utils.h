#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkCameraLayer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraLayer_EntityScript;

// --------------------------------------------------------------------------------------------------------------------
// UCk_Utils_CameraLayer_UE — the SmTask-style utils for camera layers.
//
//  - Create(camera, layerClass): spawns the layer entity under a camera (stores the owning camera, attaches the
//    layer EntityScript). Mirrors UCk_Utils_SmTask_UE::Create. A layer is always created from a camera handle.
//  - Acquire_CameraModifier_<Tuner>(layer, op, target[, component]): lazily creates (once per tuner per layer) an
//    attribute modifier on the owning camera's tuner attribute, records its TARGET (the value at full blend) + op,
//    and returns the typed modifier handle. The camera is derived from the layer (no camera param). The framework
//    auto-blends the modifier in/out (FProcessor_CameraLayer_Blend scales target→effective by the layer's alpha).
//    Re-acquiring the same tuner updates the target (ensuring the operation matches).
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CameraLayer"))
class CKCAMERA_API UCk_Utils_CameraLayer_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CameraLayer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CameraLayer);

public:
    static auto
    Create(
        FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraLayer_EntityScript> InLayerClass) -> FCk_Handle_CameraLayer;

    static bool
    Has(
        const FCk_Handle& InHandle);

    // Destroys every attribute modifier this layer acquired (they live under the tuner attributes, not the layer).
    // Called on layer teardown (ExitLayer) so removing a layer cleanly removes its contributions.
    static auto
    Remove_AllAcquiredModifiers(
        FCk_Handle_CameraLayer& InLayer) -> void;

    // ================================================================================================================
    // ACQUIRE — Rig
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier BoomArmPivotOffset")
    static FCk_Handle_VectorAttributeModifier
    Acquire_CameraModifier_BoomArmPivotOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, FVector InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier BoomArmLength")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_BoomArmLength(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FramingOffset")
    static FCk_Handle_VectorAttributeModifier
    Acquire_CameraModifier_FramingOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, FVector InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FramingPitch")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_FramingPitch(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FramingYaw")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_FramingYaw(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FixedBoomRotation")
    static FCk_Handle_RotatorAttributeModifier
    Acquire_CameraModifier_FixedBoomRotation(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, FRotator InTarget);

    // ================================================================================================================
    // ACQUIRE — Springs
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier GroupBaseLocationInterpSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_GroupBaseLocationInterpSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier LookAtLocationInterpSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_LookAtLocationInterpSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Sensor
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FOV")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_FOV(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AspectRatio")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AspectRatio(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Noise
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoisePitchSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoisePitchSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoisePitchLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoisePitchLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoisePitchNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoisePitchNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoisePitchOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoisePitchOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseYawSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseYawSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseYawLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseYawLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseYawNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseYawNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseYawOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseYawOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseRollSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseRollSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseRollLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseRollLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseRollNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseRollNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NoiseRollOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_NoiseRollOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Orientation Control
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlPitchSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlPitchSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlPitchLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlPitchLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlPitchNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlPitchNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlPitchOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlPitchOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlYawSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlYawSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlYawLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlYawLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlYawNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlYawNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier OrientationControlYawOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_OrientationControlYawOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Auto Reorient
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientPitchSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientPitchSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientPitchLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientPitchLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientPitchNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientPitchNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientPitchOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientPitchOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientYawSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientYawSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientYawLimits")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientYawLimits(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientYawNormalSpeedRange")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientYawNormalSpeedRange(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AutoReorientYawOutOfRangeMultiplier")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AutoReorientYawOutOfRangeMultiplier(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier LookAtTargetLocationPitchOffset")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_LookAtTargetLocationPitchOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier LookAtTargetLocationYawOffset")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_LookAtTargetLocationYawOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier LookAtCameraLocationPitchOffset")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_LookAtCameraLocationPitchOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier LookAtCameraLocationYawOffset")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_LookAtCameraLocationYawOffset(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Collision
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier NumPredictionWhiskers")
    static FCk_Handle_IntegerAttributeModifier
    Acquire_CameraModifier_NumPredictionWhiskers(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, int32 InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier MaxWhiskerAngle")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_MaxWhiskerAngle(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier AdditionalOffsetFromHit")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_AdditionalOffsetFromHit(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier ClippingSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_ClippingSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier ReturningSpeed")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_ReturningSpeed(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // ACQUIRE — Depth Of Field
    // ================================================================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier Fstop")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_Fstop(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier FocalDistance")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_FocalDistance(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][Camera] Acquire Modifier SensorWidth")
    static FCk_Handle_FloatAttributeModifier
    Acquire_CameraModifier_SensorWidth(UPARAM(ref) FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOperation, float InTarget);

    // ================================================================================================================
    // INTERNAL
    // ================================================================================================================
private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][CameraLayer] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CameraLayer
    DoCast(UPARAM(ref) FCk_Handle& InHandle, ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraLayer", DisplayName = "[Ck][CameraLayer] Handle -> CameraLayer Handle",
        meta = (CompactNodeTitle = "<AsCameraLayer>", BlueprintAutocast))
    static FCk_Handle_CameraLayer
    DoCastChecked(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, DisplayName = "[Ck] Get Invalid CameraLayer Handle", Category = "Ck|Utils|CameraLayer",
        meta = (CompactNodeTitle = "INVALID_CameraLayerHandle", Keywords = "make"))
    static FCk_Handle_CameraLayer
    Get_InvalidHandle() { return {}; };

private:
    static auto
    DoGet_LayerTag(
        const FCk_Handle_CameraLayer& InLayer) -> FGameplayTag;

    static auto
    DoGet_OwningCamera(
        const FCk_Handle_CameraLayer& InLayer) -> FCk_Handle_Camera;

    static auto
    DoAcquire_Float(
        FCk_Handle_CameraLayer& InLayer, FCk_Handle_FloatAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier;

    static auto
    DoAcquire_Vector(
        FCk_Handle_CameraLayer& InLayer, FCk_Handle_VectorAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation, FVector InTarget) -> FCk_Handle_VectorAttributeModifier;

    static auto
    DoAcquire_Rotator(
        FCk_Handle_CameraLayer& InLayer, FCk_Handle_RotatorAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation, FRotator InTarget) -> FCk_Handle_RotatorAttributeModifier;

    static auto
    DoAcquire_Integer(
        FCk_Handle_CameraLayer& InLayer, FCk_Handle_IntegerAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation, int32 InTarget) -> FCk_Handle_IntegerAttributeModifier;
};

// --------------------------------------------------------------------------------------------------------------------
