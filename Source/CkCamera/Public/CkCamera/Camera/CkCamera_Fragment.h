#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/Profile/CkCameraProfile.h"
#include "CkCamera/Camera/CkCamera_POV.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"

#include <AlphaBlend.h>
#include <Camera/CameraTypes.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Camera_UE;
class UCk_CameraLayer_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // _Alpha reuses the dominant layer's blend alpha, so the final POV eases on the same curve as the layer.
    // ----------------------------------------------------------------------------------------------------------------
    struct FCk_Camera_ViewTargetResolved
    {
        bool       _IsActive = false;
        FTransform _Target   = FTransform::Identity;
        float      _Alpha    = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // DIRECTOR FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Params);

    public:
        using ParamsType = FCk_Camera_Spec;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Camera_Params, _Params);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Current);

    public:
        friend class FProcessor_CameraLayer_Lifecycle;
        friend class FProcessor_Camera_UpdatePOV;
        friend class UCk_Utils_Camera_UE;

    private:
        // Read-cache; refreshed each frame by the lifecycle processor, read by UpdatePOV + observers.
        FCk_CameraProfile _ComposedProfile;

        // ---- Non-attribute leaves (bools + curves) ----
        // These do not blend, so they are plain data rather than tuner attributes.
        bool _UseFixedBoomRotation = false;
        bool _ConstrainAspectRatio = false;
        bool _HasOrientationControl = true;
        bool _HasAutoReorient = false;
        bool _HasCollision = true;
        bool _UseAsyncTrace = true;
        bool _UsePostProcess = false;

        FAlphaBlend _XIntentionCurve;
        FAlphaBlend _YIntentionCurve;

        // Unset = the dominant layer has no look-at (a zero vector is a valid target, so it cannot encode absence).
        TOptional<FVector> _DominantLookAt;

        // Mutually exclusive with _DominantLookAt, per the dominant layer's FCk_Camera_Target mode.
        FCk_Camera_ViewTargetResolved _ViewTarget;

        TSubclassOf<UCk_CameraLayer_EntityScript> _DominantLayerClass;

    public:
        CK_PROPERTY(_ComposedProfile);
        CK_PROPERTY(_UseFixedBoomRotation);
        CK_PROPERTY(_ConstrainAspectRatio);
        CK_PROPERTY(_HasOrientationControl);
        CK_PROPERTY(_HasAutoReorient);
        CK_PROPERTY(_HasCollision);
        CK_PROPERTY(_UseAsyncTrace);
        CK_PROPERTY(_UsePostProcess);
        CK_PROPERTY(_XIntentionCurve);
        CK_PROPERTY(_YIntentionCurve);
        CK_PROPERTY_GET(_DominantLayerClass);
        CK_PROPERTY_GET(_DominantLookAt);
        CK_PROPERTY_GET(_ViewTarget);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The POV integrator state: the FPov::Run accumulator, the per-frame intention delta it
    // consumes, and the FMinimalViewInfo output (read by UCk_CameraComponent::GetCameraView).
    // Written by UpdatePOV and the Utils seed/snap paths; the compose blackboard stays in Current.
    struct CKCAMERA_API FFragment_Camera_Pov
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Pov);

    public:
        friend class FProcessor_Camera_UpdatePOV;
        friend class UCk_Utils_Camera_UE;

    private:
        ck::camera::FPov_State _PovState;

        // A per-frame DELTA pushed by whoever owns input. UpdatePOV CONSUMES it (resets to zero) after applying, so
        // the caller must re-push every frame the input is active.
        FVector _OrientationIntention = FVector::ZeroVector;

        FMinimalViewInfo _ViewInfo;

    public:
        CK_PROPERTY(_OrientationIntention);
        CK_PROPERTY_GET(_PovState);
        CK_PROPERTY_GET(_ViewInfo);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Requests);

    public:
        friend class FProcessor_Camera_HandleRequests;
        friend class UCk_Utils_Camera_UE;

    public:
        using AddLayerRequestType    = FCk_Request_Camera_AddLayer;
        using RemoveLayerRequestType = FCk_Request_Camera_RemoveLayer;
        using Requests               = TArray<std::variant<AddLayerRequestType, RemoveLayerRequestType>>;

    private:
        Requests _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // PER-SECTION TUNER-ATTRIBUTE HANDLE FRAGMENTS
    // An FCk_FloatRange profile leaf is a SINGLE Float-with-MinMax attribute (Min/Max = the bounds, Current unused).
    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FCameraAttr_Rotation
    {
        FCk_Handle_FloatAttribute _Speed;
        FCk_Handle_FloatAttribute _Limits;            // MinMax: Min/Max = range bounds
        FCk_Handle_FloatAttribute _NormalSpeedRange;  // MinMax: Min/Max = range bounds
        FCk_Handle_FloatAttribute _OutOfRangeMultiplier;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Rig
    {
        CK_GENERATED_BODY(FFragment_Camera_Rig);

    public:
        FCk_Handle_VectorAttribute  _BoomArmPivotOffset;
        FCk_Handle_FloatAttribute   _BoomArmLength;
        FCk_Handle_VectorAttribute  _FramingOffset;
        FCk_Handle_FloatAttribute   _FramingPitch;
        FCk_Handle_FloatAttribute   _FramingYaw;
        FCk_Handle_RotatorAttribute _FixedBoomRotation;
    };

    struct CKCAMERA_API FFragment_Camera_Springs
    {
        CK_GENERATED_BODY(FFragment_Camera_Springs);

    public:
        FCk_Handle_FloatAttribute _GroupBaseLocationInterpSpeed;
        FCk_Handle_FloatAttribute _LookAtLocationInterpSpeed;
    };

    struct CKCAMERA_API FFragment_Camera_Sensor
    {
        CK_GENERATED_BODY(FFragment_Camera_Sensor);

    public:
        FCk_Handle_FloatAttribute _FOV;
        FCk_Handle_FloatAttribute _AspectRatio;
    };

    struct CKCAMERA_API FFragment_Camera_Noise
    {
        CK_GENERATED_BODY(FFragment_Camera_Noise);

    public:
        FCameraAttr_Rotation _Pitch;
        FCameraAttr_Rotation _Yaw;
        FCameraAttr_Rotation _Roll;
    };

    struct CKCAMERA_API FFragment_Camera_OrientationControl
    {
        CK_GENERATED_BODY(FFragment_Camera_OrientationControl);

    public:
        FCameraAttr_Rotation _Pitch;
        FCameraAttr_Rotation _Yaw;
        // _XIntentionCurve / _YIntentionCurve live on FFragment_Camera_Current (FAlphaBlend, not attributes).
    };

    struct CKCAMERA_API FFragment_Camera_AutoReorient
    {
        CK_GENERATED_BODY(FFragment_Camera_AutoReorient);

    public:
        FCameraAttr_Rotation      _Pitch;
        FCameraAttr_Rotation      _Yaw;
        FCk_Handle_FloatAttribute _LookAtTargetLocationPitchOffset;
        FCk_Handle_FloatAttribute _LookAtTargetLocationYawOffset;
        FCk_Handle_FloatAttribute _LookAtCameraLocationPitchOffset;
        FCk_Handle_FloatAttribute _LookAtCameraLocationYawOffset;
    };

    struct CKCAMERA_API FFragment_Camera_Collision
    {
        CK_GENERATED_BODY(FFragment_Camera_Collision);

    public:
        FCk_Handle_IntegerAttribute _NumPredictionWhiskers;
        FCk_Handle_FloatAttribute   _MaxWhiskerAngle;
        FCk_Handle_FloatAttribute   _AdditionalOffsetFromHit;
        FCk_Handle_FloatAttribute   _ClippingSpeed;
        FCk_Handle_FloatAttribute   _ReturningSpeed;
    };

    struct CKCAMERA_API FFragment_Camera_DepthOfField
    {
        CK_GENERATED_BODY(FFragment_Camera_DepthOfField);

    public:
        FCk_Handle_FloatAttribute _Fstop;
        FCk_Handle_FloatAttribute _FocalDistance;
        FCk_Handle_FloatAttribute _SensorWidth;
    };
}

// --------------------------------------------------------------------------------------------------------------------
