#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/Camera/CkCamera_Component.h"
#include "CkCamera/Camera/CkCamera_GameplayTags.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Utils.h"
#include "CkCamera/Camera/CameraLayer/EntityScripts/CkCameraLayer_EntityScript.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Camera_Spec& InParams)
    -> FCk_Handle_Camera
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_OutputComponent()),
        TEXT("Camera Add on entity [{}] requires a valid output UCk_CameraComponent supplied in "
             "FCk_Camera_Spec — none was provided."), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Camera_Params>(InParams);
    InHandle.AddOrGet<ck::FFragment_Camera_Current>();
    InHandle.AddOrGet<ck::FFragment_Camera_Pov>();

    ck::FUtils_RecordOfCameraLayers::AddIfMissing(InHandle);

    auto Director = Cast(InHandle);

    DoMaterializeAttributes(Director, InParams.Get_Profile());

    // The persistent base layer acquires no modifiers — the resting state IS the tuner base values just materialized.
    if (auto DefaultLayer = UCk_Utils_CameraLayer_UE::Create(Director, UCk_CameraLayer_Default_EntityScript::StaticClass());
        ck::IsValid(DefaultLayer))
    {
        auto& Params = DefaultLayer.Get<ck::FFragment_CameraLayer_Params>();
        Params.Set_IsDefault(true);
        Params.Set_Priority(TNumericLimits<int32>::Lowest());

        auto& Blend = DefaultLayer.Get<ck::FFragment_CameraLayer_Blend>();
        Blend.Set_Alpha(1.0f);
        Blend.Set_TargetAlpha(1.0f);
    }

    InParams.Get_OutputComponent()->Set_DirectorEntity(Director);

    // The FIRST GetCameraView can land before FProcessor_Camera_UpdatePOV ever ticks; without this seed it would
    // return a default (origin) FMinimalViewInfo and the camera would snap on the first frame after Add.
    if (Director.Has<ck::FFragment_Transform>())
    {
        auto& Current = Director.Get<ck::FFragment_Camera_Current>();
        auto& Pov = Director.Get<ck::FFragment_Camera_Pov>();

        auto Input = ck::camera::FPov_Input{};
        Input._AnchorTransform  = Director.Get<ck::FFragment_Transform>().Get_Transform();
        Input._World            = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Director);
        Input._TraceIgnoreActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(Director);

        ck::camera::FPov::Run(Current.Get_ComposedProfile(), Input, Pov._PovState);

        auto ViewInfo = FMinimalViewInfo{};
        ViewInfo.Location   = Pov._PovState._CameraTransform.GetLocation();
        ViewInfo.Rotation   = Pov._PovState._CameraTransform.Rotator();
        ViewInfo.FOV        = Current.Get_ComposedProfile().Get_Sensor().Get_FOV();
        ViewInfo.DesiredFOV = ViewInfo.FOV;
        Pov._ViewInfo   = ViewInfo;
    }

    return Director;
}

auto
    UCk_Utils_Camera_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Camera_Spec& InParams)
    -> FCk_Handle_Camera
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Get_LayerCount(
        const FCk_Handle_Camera& InCamera)
    -> int32
{
    auto Count = int32{0};

    ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InCamera,
    [&](FCk_Handle_CameraLayer InLayer)
    {
        if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_IsDefault())
        { return; }

        ++Count;
    });

    return Count;
}

auto
    UCk_Utils_Camera_UE::
    Has_Layer(
        const FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraLayer_EntityScript> InLayerClass)
    -> bool
{
    auto Found = false;

    ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InCamera,
    [&](FCk_Handle_CameraLayer InLayer)
    {
        if (Found)
        { return; }

        if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_LayerClass() == InLayerClass)
        { Found = true; }
    });

    return Found;
}

auto
    UCk_Utils_Camera_UE::
    Get_DominantLayerClass(
        const FCk_Handle_Camera& InCamera)
    -> TSubclassOf<UCk_CameraLayer_EntityScript>
{
    return InCamera.Get<ck::FFragment_Camera_Current>().Get_DominantLayerClass();
}

auto
    UCk_Utils_Camera_UE::
    Get_ComposedProfile(
        const FCk_Handle_Camera& InCamera)
    -> FCk_CameraProfile
{
    return InCamera.Get<ck::FFragment_Camera_Current>().Get_ComposedProfile();
}

auto
    UCk_Utils_Camera_UE::
    Get_ViewInfo(
        const FCk_Handle_Camera& InCamera)
    -> FMinimalViewInfo
{
    return InCamera.Get<ck::FFragment_Camera_Pov>().Get_ViewInfo();
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Camera_UE, FCk_Handle_Camera, ck::FFragment_Camera_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Request_AddLayer(
        FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_AddLayer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InCamera.AddOrGet<ck::FFragment_Camera_Requests>()._Requests.Emplace(InRequest);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_RemoveLayer(
        FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_RemoveLayer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InCamera.AddOrGet<ck::FFragment_Camera_Requests>()._Requests.Emplace(InRequest);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_SetOrientationIntention(
        FCk_Handle_Camera& InCamera,
        FVector InOrientationIntention,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Pov>().Set_OrientationIntention(InOrientationIntention);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_SnapBoomRotation(
        FCk_Handle_Camera& InCamera,
        FRotator InWorldRotation,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InWorldRotation.Roll = 0.0f;

    auto& Pov = InCamera.Get<ck::FFragment_Camera_Pov>();
    // _Initialized so FPov::Run's seed-from-anchor branch cannot clobber this seed on a first frame.
    Pov._PovState._BoomArmRotation = InWorldRotation;
    Pov._PovState._Initialized     = true;

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_OrientationYawLimits(
        FCk_Handle_Camera& InCamera,
        float InMinYaw,
        float InMaxYaw,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    auto& Limits = InCamera.Get<ck::FFragment_Camera_OrientationControl>()._Yaw._Limits;
    UCk_Utils_FloatAttribute_UE::Request_Override(Limits, InMinYaw, ECk_MinMaxCurrent::Min, {});
    UCk_Utils_FloatAttribute_UE::Request_Override(Limits, InMaxYaw, ECk_MinMaxCurrent::Max, {});

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Request_Set_UseFixedBoomRotation(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_UseFixedBoomRotation(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_ConstrainAspectRatio(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_ConstrainAspectRatio(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_HasOrientationControl(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_HasOrientationControl(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_HasAutoReorient(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_HasAutoReorient(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_HasCollision(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_HasCollision(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_UseAsyncTrace(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_UseAsyncTrace(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_Set_UsePostProcess(FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Camera
{
    InCamera.Get<ck::FFragment_Camera_Current>().Set_UsePostProcess(bInEnabled);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCamera, ECk_Request_OperationResult::Succeeded);
    return InCamera;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Get_ViewRotation(
        const FCk_Handle_Camera& InCamera)
    -> FRotator
{
    return Get_ViewInfo(InCamera).Rotation;
}

// --------------------------------------------------------------------------------------------------------------------
// MATERIALIZE + ASSEMBLE
// --------------------------------------------------------------------------------------------------------------------

namespace camera_materialize_detail
{
    static constexpr ECk_Replication   NoRep   = ECk_Replication::DoesNotReplicate;
    static constexpr ECk_MinMaxCurrent Current = ECk_MinMaxCurrent::Current;
    static constexpr ECk_MinMaxCurrent Min     = ECk_MinMaxCurrent::Min;
    static constexpr ECk_MinMaxCurrent Max     = ECk_MinMaxCurrent::Max;

    // Every AddX below is adopt-or-add; rationale in CkCamera/CLAUDE.md.

    // Tuners are reconstruct-only: a camera is client-local per-viewer state whose profile plus current
    // possession own the authoritative post-load values, so hydration would run before the restored pawn is
    // locally possessed. Mark new and legacy-adopted children alike so future saves omit them.
    template <typename T_AttributeHandle>
    static auto MarkReconstructOnly(T_AttributeHandle InAttribute) -> T_AttributeHandle
    {
        InAttribute.AddOrGet<ck::FTag_Snapshot_ReconstructOnly>();
        return InAttribute;
    }

    static auto AddFloat(FCk_Handle& InOwner, const FGameplayTag& InTag, float InBase) -> FCk_Handle_FloatAttribute
    {
        if (auto Existing = UCk_Utils_FloatAttribute_UE::TryGet(InOwner, InTag);
            ck::IsValid(Existing))
        {
            UCk_Utils_FloatAttribute_UE::Request_Override(Existing, InBase, Current, {});
            return MarkReconstructOnly(Existing);
        }

        return MarkReconstructOnly(UCk_Utils_FloatAttribute_UE::Add(InOwner,
            FCk_FloatAttribute_Spec{InTag, InBase}, NoRep));
    }

    static auto AddRange(FCk_Handle& InOwner, const FGameplayTag& InTag, const FCk_FloatRange& InRange) -> FCk_Handle_FloatAttribute
    {
        if (auto Existing = UCk_Utils_FloatAttribute_UE::TryGet(InOwner, InTag);
            ck::IsValid(Existing))
        {
            UCk_Utils_FloatAttribute_UE::Request_Override(Existing, static_cast<float>(InRange.Get_Min()), Min, {});
            UCk_Utils_FloatAttribute_UE::Request_Override(Existing, static_cast<float>(InRange.Get_Max()), Max, {});
            return MarkReconstructOnly(Existing);
        }

        auto Params = FCk_FloatAttribute_Spec{InTag, 0.0f};
        Params.Set_MinMax(ECk_MinMax::MinMax);
        Params.Set_MinValue(static_cast<float>(InRange.Get_Min()));
        Params.Set_MaxValue(static_cast<float>(InRange.Get_Max()));
        return MarkReconstructOnly(UCk_Utils_FloatAttribute_UE::Add(InOwner, Params, NoRep));
    }

    static auto AddVector(FCk_Handle& InOwner, const FGameplayTag& InTag, const FVector& InBase) -> FCk_Handle_VectorAttribute
    {
        if (auto Existing = UCk_Utils_VectorAttribute_UE::TryGet(InOwner, InTag);
            ck::IsValid(Existing))
        {
            UCk_Utils_VectorAttribute_UE::Request_Override(Existing, InBase, Current, {});
            return MarkReconstructOnly(Existing);
        }

        return MarkReconstructOnly(UCk_Utils_VectorAttribute_UE::Add(InOwner,
            FCk_VectorAttribute_Spec{InTag, InBase}, NoRep));
    }

    static auto AddRotator(FCk_Handle& InOwner, const FGameplayTag& InTag, const FRotator& InBase) -> FCk_Handle_RotatorAttribute
    {
        if (auto Existing = UCk_Utils_RotatorAttribute_UE::TryGet(InOwner, InTag);
            ck::IsValid(Existing))
        {
            UCk_Utils_RotatorAttribute_UE::Request_Override(Existing, InBase, Current, {});
            return MarkReconstructOnly(Existing);
        }

        return MarkReconstructOnly(UCk_Utils_RotatorAttribute_UE::Add(InOwner,
            FCk_RotatorAttribute_Spec{InTag, InBase}, NoRep));
    }

    static auto AddInt(FCk_Handle& InOwner, const FGameplayTag& InTag, int32 InBase) -> FCk_Handle_IntegerAttribute
    {
        if (auto Existing = UCk_Utils_IntegerAttribute_UE::TryGet(InOwner, InTag);
            ck::IsValid(Existing))
        {
            UCk_Utils_IntegerAttribute_UE::Request_Override(Existing, InBase, Current, {});
            return MarkReconstructOnly(Existing);
        }

        return MarkReconstructOnly(UCk_Utils_IntegerAttribute_UE::Add(InOwner,
            FCk_IntegerAttribute_Spec{InTag, InBase}, NoRep));
    }

    static auto AddRotation(
        FCk_Handle& InOwner,
        const FGameplayTag& InSpeedTag,
        const FGameplayTag& InLimitsTag,
        const FGameplayTag& InNormalSpeedRangeTag,
        const FGameplayTag& InOutOfRangeMultiplierTag,
        const FCk_CameraProfile_Rotation& InRot) -> ck::FCameraAttr_Rotation
    {
        auto Out = ck::FCameraAttr_Rotation{};
        Out._Speed                = AddFloat(InOwner, InSpeedTag, InRot.Get_Speed());
        Out._Limits               = AddRange(InOwner, InLimitsTag, InRot.Get_Limits());
        Out._NormalSpeedRange     = AddRange(InOwner, InNormalSpeedRangeTag, InRot.Get_NormalSpeedRange());
        Out._OutOfRangeMultiplier = AddFloat(InOwner, InOutOfRangeMultiplierTag, InRot.Get_OutOfRangeMultiplier());
        return Out;
    }

    static auto ReadFloat(const FCk_Handle_FloatAttribute& InAttr) -> float
    {
        return UCk_Utils_FloatAttribute_UE::Get_FinalValue(InAttr, ECk_MinMaxCurrent::Current);
    }

    static auto ReadRange(const FCk_Handle_FloatAttribute& InAttr) -> FCk_FloatRange
    {
        return FCk_FloatRange{UCk_Utils_FloatAttribute_UE::Get_FinalValue(InAttr, Min),
                              UCk_Utils_FloatAttribute_UE::Get_FinalValue(InAttr, Max)};
    }

    static auto ReadRotation(const ck::FCameraAttr_Rotation& InAttr) -> FCk_CameraProfile_Rotation
    {
        auto Out = FCk_CameraProfile_Rotation{};
        Out.Set_Speed(ReadFloat(InAttr._Speed));
        Out.Set_Limits(ReadRange(InAttr._Limits));
        Out.Set_NormalSpeedRange(ReadRange(InAttr._NormalSpeedRange));
        Out.Set_OutOfRangeMultiplier(ReadFloat(InAttr._OutOfRangeMultiplier));
        return Out;
    }
}

auto
    UCk_Utils_Camera_UE::
    DoMaterializeAttributes(
        FCk_Handle_Camera& InCamera,
        const FCk_CameraProfile& InDefaults)
    -> void
{
    using namespace camera_materialize_detail;

    FCk_Handle& Owner = InCamera;

    // ---- Rig ----
    {
        const auto& Rig = InDefaults.Get_Rig();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_Rig>();
        Section._BoomArmPivotOffset = AddVector(Owner,  TAG_Camera_Rig_BoomArmPivotOffset, Rig.Get_BoomArmPivotOffset());
        Section._BoomArmLength      = AddFloat(Owner,   TAG_Camera_Rig_BoomArmLength,      Rig.Get_BoomArmLength());
        Section._FramingOffset      = AddVector(Owner,  TAG_Camera_Rig_FramingOffset,      Rig.Get_FramingOffset());
        Section._FramingPitch       = AddFloat(Owner,   TAG_Camera_Rig_FramingPitch,       Rig.Get_FramingPitch());
        Section._FramingYaw         = AddFloat(Owner,   TAG_Camera_Rig_FramingYaw,         Rig.Get_FramingYaw());
        Section._FixedBoomRotation  = AddRotator(Owner, TAG_Camera_Rig_FixedBoomRotation,  Rig.Get_FixedBoomRotation());
    }

    // ---- Springs ----
    {
        const auto& Springs = InDefaults.Get_Springs();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_Springs>();
        Section._GroupBaseLocationInterpSpeed = AddFloat(Owner, TAG_Camera_Springs_GroupBaseLocationInterpSpeed, Springs.Get_GroupBaseLocationInterpSpeed());
        Section._LookAtLocationInterpSpeed    = AddFloat(Owner, TAG_Camera_Springs_LookAtLocationInterpSpeed,    Springs.Get_LookAtLocationInterpSpeed());
    }

    // ---- Sensor ----
    {
        const auto& Sensor = InDefaults.Get_Sensor();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_Sensor>();
        Section._FOV         = AddFloat(Owner, TAG_Camera_Sensor_FOV,         Sensor.Get_FOV());
        Section._AspectRatio = AddFloat(Owner, TAG_Camera_Sensor_AspectRatio, Sensor.Get_AspectRatio());
    }

    // ---- Noise ----
    {
        const auto& Noise = InDefaults.Get_Noise();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_Noise>();
        Section._Pitch = AddRotation(Owner, TAG_Camera_Noise_Pitch_Speed, TAG_Camera_Noise_Pitch_Limits, TAG_Camera_Noise_Pitch_NormalSpeedRange, TAG_Camera_Noise_Pitch_OutOfRangeMultiplier, Noise.Get_Pitch());
        Section._Yaw   = AddRotation(Owner, TAG_Camera_Noise_Yaw_Speed,   TAG_Camera_Noise_Yaw_Limits,   TAG_Camera_Noise_Yaw_NormalSpeedRange,   TAG_Camera_Noise_Yaw_OutOfRangeMultiplier,   Noise.Get_Yaw());
        Section._Roll  = AddRotation(Owner, TAG_Camera_Noise_Roll_Speed,  TAG_Camera_Noise_Roll_Limits,  TAG_Camera_Noise_Roll_NormalSpeedRange,  TAG_Camera_Noise_Roll_OutOfRangeMultiplier,  Noise.Get_Roll());
    }

    // ---- Orientation Control ----
    {
        const auto& OC = InDefaults.Get_OrientationControl();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_OrientationControl>();
        Section._Pitch = AddRotation(Owner, TAG_Camera_OrientationControl_Pitch_Speed, TAG_Camera_OrientationControl_Pitch_Limits, TAG_Camera_OrientationControl_Pitch_NormalSpeedRange, TAG_Camera_OrientationControl_Pitch_OutOfRangeMultiplier, OC.Get_Pitch());
        Section._Yaw   = AddRotation(Owner, TAG_Camera_OrientationControl_Yaw_Speed,   TAG_Camera_OrientationControl_Yaw_Limits,   TAG_Camera_OrientationControl_Yaw_NormalSpeedRange,   TAG_Camera_OrientationControl_Yaw_OutOfRangeMultiplier,   OC.Get_Yaw());
    }

    // ---- Auto Reorient ----
    {
        const auto& AR = InDefaults.Get_AutoReorient();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_AutoReorient>();
        Section._Pitch = AddRotation(Owner, TAG_Camera_AutoReorient_Pitch_Speed, TAG_Camera_AutoReorient_Pitch_Limits, TAG_Camera_AutoReorient_Pitch_NormalSpeedRange, TAG_Camera_AutoReorient_Pitch_OutOfRangeMultiplier, AR.Get_Pitch());
        Section._Yaw   = AddRotation(Owner, TAG_Camera_AutoReorient_Yaw_Speed,   TAG_Camera_AutoReorient_Yaw_Limits,   TAG_Camera_AutoReorient_Yaw_NormalSpeedRange,   TAG_Camera_AutoReorient_Yaw_OutOfRangeMultiplier,   AR.Get_Yaw());
        Section._LookAtTargetLocationPitchOffset = AddFloat(Owner, TAG_Camera_AutoReorient_LookAtTargetLocationPitchOffset, AR.Get_LookAtTargetLocationPitchOffset());
        Section._LookAtTargetLocationYawOffset   = AddFloat(Owner, TAG_Camera_AutoReorient_LookAtTargetLocationYawOffset,   AR.Get_LookAtTargetLocationYawOffset());
        Section._LookAtCameraLocationPitchOffset = AddFloat(Owner, TAG_Camera_AutoReorient_LookAtCameraLocationPitchOffset, AR.Get_LookAtCameraLocationPitchOffset());
        Section._LookAtCameraLocationYawOffset   = AddFloat(Owner, TAG_Camera_AutoReorient_LookAtCameraLocationYawOffset,   AR.Get_LookAtCameraLocationYawOffset());
    }

    // ---- Collision ----
    {
        const auto& Collision = InDefaults.Get_Collision();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_Collision>();
        Section._NumPredictionWhiskers   = AddInt(Owner,   TAG_Camera_Collision_NumPredictionWhiskers,   Collision.Get_NumPredictionWhiskers());
        Section._MaxWhiskerAngle         = AddFloat(Owner, TAG_Camera_Collision_MaxWhiskerAngle,         Collision.Get_MaxWhiskerAngle());
        Section._AdditionalOffsetFromHit = AddFloat(Owner, TAG_Camera_Collision_AdditionalOffsetFromHit, Collision.Get_AdditionalOffsetFromHit());
        Section._ClippingSpeed           = AddFloat(Owner, TAG_Camera_Collision_ClippingSpeed,           Collision.Get_ClippingSpeed());
        Section._ReturningSpeed          = AddFloat(Owner, TAG_Camera_Collision_ReturningSpeed,          Collision.Get_ReturningSpeed());
    }

    // ---- Depth Of Field ----
    {
        const auto& DoF = InDefaults.Get_DepthOfField();
        auto& Section = InCamera.AddOrGet<ck::FFragment_Camera_DepthOfField>();
        Section._Fstop         = AddFloat(Owner, TAG_Camera_DepthOfField_Fstop,         DoF.Get_Fstop());
        Section._FocalDistance = AddFloat(Owner, TAG_Camera_DepthOfField_FocalDistance, DoF.Get_FocalDistance());
        Section._SensorWidth   = AddFloat(Owner, TAG_Camera_DepthOfField_SensorWidth,   DoF.Get_SensorWidth());
    }

    // ---- Non-attribute leaves (bools + curves) → Current ----
    {
        auto& CurrentFrag = InCamera.AddOrGet<ck::FFragment_Camera_Current>();
        CurrentFrag.Set_UseFixedBoomRotation(InDefaults.Get_Rig().Get_UseFixedBoomRotation());
        CurrentFrag.Set_ConstrainAspectRatio(InDefaults.Get_Sensor().Get_ConstrainAspectRatio());
        CurrentFrag.Set_HasOrientationControl(InDefaults.Get_HasOrientationControl());
        CurrentFrag.Set_HasAutoReorient(InDefaults.Get_HasAutoReorient());
        CurrentFrag.Set_HasCollision(InDefaults.Get_HasCollision());
        CurrentFrag.Set_UseAsyncTrace(InDefaults.Get_Collision().Get_UseAsyncTrace());
        CurrentFrag.Set_UsePostProcess(InDefaults.Get_UsePostProcess());
        CurrentFrag.Set_XIntentionCurve(InDefaults.Get_OrientationControl().Get_XIntentionCurve());
        CurrentFrag.Set_YIntentionCurve(InDefaults.Get_OrientationControl().Get_YIntentionCurve());

        // Seed the read-cache so the first frame (before the lifecycle processor runs) has a sane profile.
        CurrentFrag.Set_ComposedProfile(InDefaults);
    }
}

auto
    UCk_Utils_Camera_UE::
    Get_Profile(
        const FCk_Handle_Camera& InCamera)
    -> FCk_CameraProfile
{
    using namespace camera_materialize_detail;

    auto Profile = FCk_CameraProfile{};

    const auto& CurrentFrag = InCamera.Get<ck::FFragment_Camera_Current>();

    // ---- Rig ----
    {
        const auto& Section = InCamera.Get<ck::FFragment_Camera_Rig>();
        auto Rig = FCk_CameraProfile_Rig{};
        Rig.Set_BoomArmPivotOffset(UCk_Utils_VectorAttribute_UE::Get_FinalValue(Section._BoomArmPivotOffset, Current));
        Rig.Set_BoomArmLength(ReadFloat(Section._BoomArmLength));
        Rig.Set_FramingOffset(UCk_Utils_VectorAttribute_UE::Get_FinalValue(Section._FramingOffset, Current));
        Rig.Set_FramingPitch(ReadFloat(Section._FramingPitch));
        Rig.Set_FramingYaw(ReadFloat(Section._FramingYaw));
        Rig.Set_UseFixedBoomRotation(CurrentFrag.Get_UseFixedBoomRotation());
        Rig.Set_FixedBoomRotation(UCk_Utils_RotatorAttribute_UE::Get_FinalValue(Section._FixedBoomRotation, Current));
        Profile.Set_Rig(Rig);
    }

    // ---- Springs ----
    {
        const auto& Section = InCamera.Get<ck::FFragment_Camera_Springs>();
        auto Springs = FCk_CameraProfile_Springs{};
        Springs.Set_GroupBaseLocationInterpSpeed(ReadFloat(Section._GroupBaseLocationInterpSpeed));
        Springs.Set_LookAtLocationInterpSpeed(ReadFloat(Section._LookAtLocationInterpSpeed));
        Profile.Set_Springs(Springs);
    }

    // ---- Sensor ----
    {
        const auto& Section = InCamera.Get<ck::FFragment_Camera_Sensor>();
        auto Sensor = FCk_CameraProfile_Sensor{};
        Sensor.Set_FOV(ReadFloat(Section._FOV));
        Sensor.Set_ConstrainAspectRatio(CurrentFrag.Get_ConstrainAspectRatio());
        Sensor.Set_AspectRatio(ReadFloat(Section._AspectRatio));
        Profile.Set_Sensor(Sensor);
    }

    // ---- Noise ----
    {
        const auto& Section = InCamera.Get<ck::FFragment_Camera_Noise>();
        auto Noise = FCk_CameraProfile_Noise{};
        Noise.Set_Pitch(ReadRotation(Section._Pitch));
        Noise.Set_Yaw(ReadRotation(Section._Yaw));
        Noise.Set_Roll(ReadRotation(Section._Roll));
        Profile.Set_Noise(Noise);
    }

    // ---- Orientation Control ----
    {
        Profile.Set_HasOrientationControl(CurrentFrag.Get_HasOrientationControl());
        const auto& Section = InCamera.Get<ck::FFragment_Camera_OrientationControl>();
        auto OC = FCk_CameraProfile_OrientationControl{};
        OC.Set_Pitch(ReadRotation(Section._Pitch));
        OC.Set_Yaw(ReadRotation(Section._Yaw));
        OC.Set_XIntentionCurve(CurrentFrag.Get_XIntentionCurve());
        OC.Set_YIntentionCurve(CurrentFrag.Get_YIntentionCurve());
        Profile.Set_OrientationControl(OC);
    }

    // ---- Auto Reorient ----
    {
        Profile.Set_HasAutoReorient(CurrentFrag.Get_HasAutoReorient());
        const auto& Section = InCamera.Get<ck::FFragment_Camera_AutoReorient>();
        auto AR = FCk_CameraProfile_AutoReorient{};
        AR.Set_Pitch(ReadRotation(Section._Pitch));
        AR.Set_Yaw(ReadRotation(Section._Yaw));
        AR.Set_LookAtTargetLocationPitchOffset(ReadFloat(Section._LookAtTargetLocationPitchOffset));
        AR.Set_LookAtTargetLocationYawOffset(ReadFloat(Section._LookAtTargetLocationYawOffset));
        AR.Set_LookAtCameraLocationPitchOffset(ReadFloat(Section._LookAtCameraLocationPitchOffset));
        AR.Set_LookAtCameraLocationYawOffset(ReadFloat(Section._LookAtCameraLocationYawOffset));
        Profile.Set_AutoReorient(AR);
    }

    // ---- Collision ----
    {
        Profile.Set_HasCollision(CurrentFrag.Get_HasCollision());
        const auto& Section = InCamera.Get<ck::FFragment_Camera_Collision>();
        auto Collision = FCk_CameraProfile_Collision{};
        Collision.Set_UseAsyncTrace(CurrentFrag.Get_UseAsyncTrace());
        Collision.Set_NumPredictionWhiskers(UCk_Utils_IntegerAttribute_UE::Get_FinalValue(Section._NumPredictionWhiskers, Current));
        Collision.Set_MaxWhiskerAngle(ReadFloat(Section._MaxWhiskerAngle));
        Collision.Set_AdditionalOffsetFromHit(ReadFloat(Section._AdditionalOffsetFromHit));
        Collision.Set_ClippingSpeed(ReadFloat(Section._ClippingSpeed));
        Collision.Set_ReturningSpeed(ReadFloat(Section._ReturningSpeed));
        Profile.Set_Collision(Collision);
    }

    // ---- Depth Of Field ----
    {
        Profile.Set_UsePostProcess(CurrentFrag.Get_UsePostProcess());
        const auto& Section = InCamera.Get<ck::FFragment_Camera_DepthOfField>();
        auto DoF = FCk_CameraProfile_DepthOfField{};
        DoF.Set_Fstop(ReadFloat(Section._Fstop));
        DoF.Set_FocalDistance(ReadFloat(Section._FocalDistance));
        DoF.Set_SensorWidth(ReadFloat(Section._SensorWidth));
        Profile.Set_DepthOfField(DoF);
    }

    return Profile;
}

// --------------------------------------------------------------------------------------------------------------------
