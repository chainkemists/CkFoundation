#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/Profile/CkCameraProfile.h"
#include "CkCamera/Camera/CkCamera_POV.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

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
    // DIRECTOR FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Params);

    public:
        using ParamsType = FCk_Fragment_Camera_ParamsData;

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
        // Resolved read-cache of the composed profile (assembled from the tuner attributes' final values + the
        // bool/curve leaves below). Refreshed each frame by the lifecycle processor; read by UpdatePOV + observers.
        FCk_CameraProfile _ComposedProfile;

        // ---- Non-attribute leaves (bools + curves) ----
        // These do not blend, so they are plain data rather than tuner attributes (decision #9 / curves can't be attributes).
        bool _UseFixedBoomRotation = false;
        bool _ConstrainAspectRatio = false;
        bool _HasOrientationControl = true;
        bool _HasAutoReorient = false;
        bool _HasCollision = true;
        bool _UseAsyncTrace = true;
        bool _UsePostProcess = false;

        FAlphaBlend _XIntentionCurve;
        FAlphaBlend _YIntentionCurve;

        // Persistent POV pipeline state (boom rotation, smoothed pivot, collision distance, noise) — advanced by UpdatePOV.
        ck::camera::FPov_State _PovState;

        // Abstract orbit intention fed by whoever owns input. Zero = no manual orbit. The camera never reads input.
        FVector _OrientationIntention = FVector::ZeroVector;

        // Look-at target resolved from the dominant active layer (for auto-reorient). Set by the lifecycle processor,
        // consumed by UpdatePOV. Unset = the dominant layer has no look-at (a zero vector is a valid target).
        TOptional<FVector> _DominantLookAt;

        // Class of the dominant active layer (highest blend alpha). Set by the lifecycle processor; observable via Utils.
        TSubclassOf<UCk_CameraLayer_EntityScript> _DominantLayerClass;

        // The final resolved POV — written by UpdatePOV, read by UCk_CameraComponent::GetCameraView.
        FMinimalViewInfo _ViewInfo;

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
        CK_PROPERTY(_OrientationIntention);
        CK_PROPERTY_GET(_DominantLayerClass);
        CK_PROPERTY_GET(_DominantLookAt);
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
    //
    // One fragment per FCk_CameraProfile section on the Camera director; each member is a typed attribute handle for
    // O(1) lookup of the tuner attribute backing a profile leaf. Populated by UCk_Utils_Camera_UE::DoMaterializeAttributes;
    // read by UCk_Utils_Camera_UE::Get_Profile and by UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_<Tuner>.
    // FCk_FloatRange leaves are a single Float-with-MinMax attribute (Min/Max = bounds, Current unused).
    // ----------------------------------------------------------------------------------------------------------------

    // FCk_CameraProfile_Rotation → Speed (Float) + Limits (Float MinMax) + NormalSpeedRange (Float MinMax) +
    // OutOfRangeMultiplier (Float). Reused 7× (Noise.Pitch/Yaw/Roll, OrientationControl.Pitch/Yaw, AutoReorient.Pitch/Yaw).
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
