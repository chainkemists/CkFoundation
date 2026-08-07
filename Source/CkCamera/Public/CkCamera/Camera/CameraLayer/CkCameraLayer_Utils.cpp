#include "CkCameraLayer_Utils.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/Camera/CkCamera_Fragment.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"
#include "CkCamera/Camera/CameraLayer/EntityScripts/CkCameraLayer_EntityScript.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // FTag_Snapshot_SaveTransient

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraLayer_UE::
    Create(
        FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraLayer_EntityScript> InLayerClass)
    -> FCk_Handle_CameraLayer
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLayerClass),
        TEXT("Create CameraLayer: invalid layer class on camera [{}]"), InCamera)
    { return {}; }

    auto NewLayer = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InCamera);

    // Derived presentation — save-transient, the camera's re-drive re-pushes it (see CkSmTask_Utils::Create).
    NewLayer.Add<ck::FTag_Snapshot_SaveTransient>();

    UCk_Utils_Handle_UE::Set_DebugName(NewLayer, InLayerClass->GetFName());

    // MUST precede the EntityScript attach below: the layer's DoEnter acquires modifiers through this back-ref.
    ck::TUtils_CameraLayer_OwningCamera::AddOrReplace(NewLayer, InCamera);

    NewLayer.Add<ck::FFragment_CameraLayer_Params>(InLayerClass);
    NewLayer.Add<ck::FFragment_CameraLayer_Blend>();
    NewLayer.AddOrGet<ck::FFragment_CameraLayer_AcquiredModifiers>();

    auto TypedLayer = ck::StaticCast<FCk_Handle_CameraLayer>(NewLayer);

    ck::FUtils_RecordOfCameraLayers::AddIfMissing(InCamera);
    ck::FUtils_RecordOfCameraLayers::Request_Connect(
        InCamera, TypedLayer, ECk_Record_LabelRequirementPolicy::Optional);

    // Synchronous, and last: Construct must see the back-ref, params and record entry already in place.
    UCk_Utils_EntityScript_UE::Add(NewLayer, InLayerClass, FInstancedStruct{});

    return TypedLayer;
}

auto
    UCk_Utils_CameraLayer_UE::
    Remove_AllAcquiredModifiers(
        FCk_Handle_CameraLayer& InLayer)
    -> void
{
    if (NOT InLayer.Has<ck::FFragment_CameraLayer_AcquiredModifiers>())
    { return; }

    auto& Acquired = InLayer.Get<ck::FFragment_CameraLayer_AcquiredModifiers>();

    for (auto& Entry : Acquired._Float)
    {
        if (ck::IsValid(Entry._Modifier))
        { UCk_Utils_FloatAttributeModifier_UE::Remove(Entry._Modifier); }
    }
    for (auto& Entry : Acquired._Vector)
    {
        if (ck::IsValid(Entry._Modifier))
        { UCk_Utils_VectorAttributeModifier_UE::Remove(Entry._Modifier); }
    }
    for (auto& Entry : Acquired._Rotator)
    {
        if (ck::IsValid(Entry._Modifier))
        { UCk_Utils_RotatorAttributeModifier_UE::Remove(Entry._Modifier); }
    }
    for (auto& Entry : Acquired._Integer)
    {
        if (ck::IsValid(Entry._Modifier))
        { UCk_Utils_IntegerAttributeModifier_UE::Remove(Entry._Modifier); }
    }

    Acquired._Float.Reset();
    Acquired._Vector.Reset();
    Acquired._Rotator.Reset();
    Acquired._Integer.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CameraLayer_UE, FCk_Handle_CameraLayer, ck::FFragment_CameraLayer_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraLayer_UE::
    DoGet_LayerTag(
        const FCk_Handle_CameraLayer& InLayer)
    -> FGameplayTag
{
    if (NOT InLayer.Has<ck::FFragment_EntityScript_Current>())
    { return {}; }

    auto* Script = ::Cast<UCk_CameraLayer_EntityScript>(
        InLayer.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get());

    if (ck::Is_NOT_Valid(Script))
    { return {}; }

    return Script->Get_LayerTag();
}

auto
    UCk_Utils_CameraLayer_UE::
    DoGet_OwningCamera(
        const FCk_Handle_CameraLayer& InLayer)
    -> FCk_Handle_Camera
{
    return ck::TUtils_CameraLayer_OwningCamera::Get_StoredEntity(InLayer);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraLayer_UE::
    DoAcquire_Float(
        FCk_Handle_CameraLayer& InLayer,
        FCk_Handle_FloatAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation,
        float InTarget,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_FloatAttributeModifier
{
    auto& Acquired = InLayer.AddOrGet<ck::FFragment_CameraLayer_AcquiredModifiers>();

    for (auto& Entry : Acquired._Float)
    {
        if (Entry._Attribute != InAttribute || Entry._Component != InComponent)
        { continue; }

        CK_ENSURE_IF_NOT(Entry._Op == InOperation,
            TEXT("Acquire on layer [{}]: tuner modifier already exists with a different operation"), InLayer)
        { return Entry._Modifier; }

        Entry._Target = InTarget;
        return Entry._Modifier;
    }

    const auto UnderlyingOp = InOperation == ECk_AttributeModifier_Operation::Multiply
        ? ECk_AttributeModifier_Operation::Multiply
        : ECk_AttributeModifier_Operation::Add;
    const auto IdentityDelta = UnderlyingOp == ECk_AttributeModifier_Operation::Multiply ? 1.0f : 0.0f;

    auto Modifier = UCk_Utils_FloatAttributeModifier_UE::Add_Revocable(
        InAttribute, DoGet_LayerTag(InLayer), UnderlyingOp,
        FCk_Fragment_FloatAttributeModifier_ParamsData{IdentityDelta, InComponent});

    auto Entry = ck::FCk_CameraLayer_FloatModifier{};
    Entry._Modifier  = Modifier;
    Entry._Attribute = InAttribute;
    Entry._Target    = InTarget;
    Entry._Op        = InOperation;
    Entry._Component = InComponent;
    Acquired._Float.Add(Entry);

    return Modifier;
}

auto
    UCk_Utils_CameraLayer_UE::
    DoAcquire_Vector(
        FCk_Handle_CameraLayer& InLayer,
        FCk_Handle_VectorAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation,
        FVector InTarget)
    -> FCk_Handle_VectorAttributeModifier
{
    auto& Acquired = InLayer.AddOrGet<ck::FFragment_CameraLayer_AcquiredModifiers>();

    for (auto& Entry : Acquired._Vector)
    {
        if (Entry._Attribute != InAttribute)
        { continue; }

        CK_ENSURE_IF_NOT(Entry._Op == InOperation,
            TEXT("Acquire on layer [{}]: tuner modifier already exists with a different operation"), InLayer)
        { return Entry._Modifier; }

        Entry._Target = InTarget;
        return Entry._Modifier;
    }

    const auto UnderlyingOp = InOperation == ECk_AttributeModifier_Operation::Multiply
        ? ECk_AttributeModifier_Operation::Multiply
        : ECk_AttributeModifier_Operation::Add;
    const auto IdentityDelta = UnderlyingOp == ECk_AttributeModifier_Operation::Multiply ? FVector::OneVector : FVector::ZeroVector;

    auto Modifier = UCk_Utils_VectorAttributeModifier_UE::Add_Revocable(
        InAttribute, DoGet_LayerTag(InLayer), UnderlyingOp,
        FCk_Fragment_VectorAttributeModifier_ParamsData{IdentityDelta, ECk_MinMaxCurrent::Current});

    auto Entry = ck::FCk_CameraLayer_VectorModifier{};
    Entry._Modifier  = Modifier;
    Entry._Attribute = InAttribute;
    Entry._Target    = InTarget;
    Entry._Op        = InOperation;
    Entry._Component = ECk_MinMaxCurrent::Current;
    Acquired._Vector.Add(Entry);

    return Modifier;
}

auto
    UCk_Utils_CameraLayer_UE::
    DoAcquire_Rotator(
        FCk_Handle_CameraLayer& InLayer,
        FCk_Handle_RotatorAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation,
        FRotator InTarget)
    -> FCk_Handle_RotatorAttributeModifier
{
    auto& Acquired = InLayer.AddOrGet<ck::FFragment_CameraLayer_AcquiredModifiers>();

    for (auto& Entry : Acquired._Rotator)
    {
        if (Entry._Attribute != InAttribute)
        { continue; }

        CK_ENSURE_IF_NOT(Entry._Op == InOperation,
            TEXT("Acquire on layer [{}]: tuner modifier already exists with a different operation"), InLayer)
        { return Entry._Modifier; }

        Entry._Target = InTarget;
        return Entry._Modifier;
    }

    const auto UnderlyingOp = InOperation == ECk_AttributeModifier_Operation::Multiply
        ? ECk_AttributeModifier_Operation::Multiply
        : ECk_AttributeModifier_Operation::Add;
    const auto IdentityDelta = UnderlyingOp == ECk_AttributeModifier_Operation::Multiply ? FRotator{1.0f, 1.0f, 1.0f} : FRotator::ZeroRotator;

    auto Modifier = UCk_Utils_RotatorAttributeModifier_UE::Add_Revocable(
        InAttribute, DoGet_LayerTag(InLayer), UnderlyingOp,
        FCk_Fragment_RotatorAttributeModifier_ParamsData{IdentityDelta, ECk_MinMaxCurrent::Current});

    auto Entry = ck::FCk_CameraLayer_RotatorModifier{};
    Entry._Modifier  = Modifier;
    Entry._Attribute = InAttribute;
    Entry._Target    = InTarget;
    Entry._Op        = InOperation;
    Entry._Component = ECk_MinMaxCurrent::Current;
    Acquired._Rotator.Add(Entry);

    return Modifier;
}

auto
    UCk_Utils_CameraLayer_UE::
    DoAcquire_Integer(
        FCk_Handle_CameraLayer& InLayer,
        FCk_Handle_IntegerAttribute InAttribute,
        ECk_AttributeModifier_Operation InOperation,
        int32 InTarget)
    -> FCk_Handle_IntegerAttributeModifier
{
    auto& Acquired = InLayer.AddOrGet<ck::FFragment_CameraLayer_AcquiredModifiers>();

    for (auto& Entry : Acquired._Integer)
    {
        if (Entry._Attribute != InAttribute)
        { continue; }

        CK_ENSURE_IF_NOT(Entry._Op == InOperation,
            TEXT("Acquire on layer [{}]: tuner modifier already exists with a different operation"), InLayer)
        { return Entry._Modifier; }

        Entry._Target = InTarget;
        return Entry._Modifier;
    }

    const auto UnderlyingOp = InOperation == ECk_AttributeModifier_Operation::Multiply
        ? ECk_AttributeModifier_Operation::Multiply
        : ECk_AttributeModifier_Operation::Add;
    const auto IdentityDelta = UnderlyingOp == ECk_AttributeModifier_Operation::Multiply ? 1 : 0;

    auto Modifier = UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable(
        InAttribute, DoGet_LayerTag(InLayer), UnderlyingOp,
        FCk_Fragment_IntegerAttributeModifier_ParamsData{IdentityDelta, ECk_MinMaxCurrent::Current});

    auto Entry = ck::FCk_CameraLayer_IntegerModifier{};
    Entry._Modifier  = Modifier;
    Entry._Attribute = InAttribute;
    Entry._Target    = InTarget;
    Entry._Op        = InOperation;
    Entry._Component = ECk_MinMaxCurrent::Current;
    Acquired._Integer.Add(Entry);

    return Modifier;
}

// --------------------------------------------------------------------------------------------------------------------
// ACQUIRE WRAPPERS
// --------------------------------------------------------------------------------------------------------------------

// ---- Rig ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_BoomArmPivotOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, FVector InTarget) -> FCk_Handle_VectorAttributeModifier
{
    return DoAcquire_Vector(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._BoomArmPivotOffset, InOp, InTarget);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_BoomArmLength(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._BoomArmLength, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FramingOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, FVector InTarget) -> FCk_Handle_VectorAttributeModifier
{
    return DoAcquire_Vector(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._FramingOffset, InOp, InTarget);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FramingPitch(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._FramingPitch, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FramingYaw(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._FramingYaw, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FixedBoomRotation(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, FRotator InTarget) -> FCk_Handle_RotatorAttributeModifier
{
    return DoAcquire_Rotator(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Rig>()._FixedBoomRotation, InOp, InTarget);
}

// ---- Springs ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_GroupBaseLocationInterpSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Springs>()._GroupBaseLocationInterpSpeed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_LookAtLocationInterpSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Springs>()._LookAtLocationInterpSpeed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Sensor ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FOV(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Sensor>()._FOV, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AspectRatio(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Sensor>()._AspectRatio, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Noise ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoisePitchSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Pitch._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoisePitchLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Pitch._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoisePitchNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Pitch._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoisePitchOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Pitch._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseYawSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Yaw._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseYawLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Yaw._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseYawNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Yaw._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseYawOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Yaw._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseRollSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Roll._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseRollLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Roll._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseRollNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Roll._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NoiseRollOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Noise>()._Roll._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Orientation Control ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlPitchSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Pitch._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlPitchLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Pitch._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlPitchNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Pitch._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlPitchOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Pitch._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlYawSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Yaw._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlYawLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Yaw._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlYawNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Yaw._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_OrientationControlYawOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_OrientationControl>()._Yaw._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Auto Reorient ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientPitchSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Pitch._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientPitchLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Pitch._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientPitchNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Pitch._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientPitchOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Pitch._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientYawSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Yaw._Speed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientYawLimits(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Yaw._Limits, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientYawNormalSpeedRange(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget, ECk_MinMaxCurrent InComponent) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Yaw._NormalSpeedRange, InOp, InTarget, InComponent);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AutoReorientYawOutOfRangeMultiplier(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._Yaw._OutOfRangeMultiplier, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_LookAtTargetLocationPitchOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._LookAtTargetLocationPitchOffset, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_LookAtTargetLocationYawOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._LookAtTargetLocationYawOffset, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_LookAtCameraLocationPitchOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._LookAtCameraLocationPitchOffset, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_LookAtCameraLocationYawOffset(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_AutoReorient>()._LookAtCameraLocationYawOffset, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Collision ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_NumPredictionWhiskers(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, int32 InTarget) -> FCk_Handle_IntegerAttributeModifier
{
    return DoAcquire_Integer(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Collision>()._NumPredictionWhiskers, InOp, InTarget);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_MaxWhiskerAngle(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Collision>()._MaxWhiskerAngle, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_AdditionalOffsetFromHit(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Collision>()._AdditionalOffsetFromHit, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_ClippingSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Collision>()._ClippingSpeed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_ReturningSpeed(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_Collision>()._ReturningSpeed, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// ---- Depth Of Field ----
auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_Fstop(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_DepthOfField>()._Fstop, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_FocalDistance(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_DepthOfField>()._FocalDistance, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

auto UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_SensorWidth(FCk_Handle_CameraLayer& InLayer, ECk_AttributeModifier_Operation InOp, float InTarget) -> FCk_Handle_FloatAttributeModifier
{
    return DoAcquire_Float(InLayer, DoGet_OwningCamera(InLayer).Get<ck::FFragment_Camera_DepthOfField>()._SensorWidth, InOp, InTarget, ECk_MinMaxCurrent::Current);
}

// --------------------------------------------------------------------------------------------------------------------
