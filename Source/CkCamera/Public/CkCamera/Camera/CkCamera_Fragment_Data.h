#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkCamera/Camera/Profile/CkCameraProfile.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCamera_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraLayer_EntityScript;
class UCameraComponent;

// --------------------------------------------------------------------------------------------------------------------
// TYPE-SAFE HANDLES
// --------------------------------------------------------------------------------------------------------------------

// The "director"/stack owner — one per local viewer. Owns the per-section tuner attributes, the active-layer
// Record, and the composed view info. Every profile field lives as a (non-replicated) attribute on this entity.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_Camera : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Camera); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Camera);

// A single camera "layer" — a child entity of the director, driven by a UCk_CameraLayer_EntityScript. A layer
// acquires attribute modifiers on the director's tuner attributes; the framework auto-blends them in/out.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraLayer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraLayer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraLayer);

// --------------------------------------------------------------------------------------------------------------------
// ENUMS
// --------------------------------------------------------------------------------------------------------------------

// Whether a layer's per-frame DoTick is invoked (mirrors ECk_SmTaskMode).
UENUM(BlueprintType)
enum class ECk_Camera_TickMode : uint8
{
    // DoEnter/DoExit only; DoTick is never called.
    EnterExitOnly,
    // DoTick is called every frame while the layer is active.
    Tick
};

// How a newly-added layer interacts with existing layers in the same ordering group.
UENUM(BlueprintType)
enum class ECk_Camera_StackingBehavior : uint8
{
    // Coexists with other layers in the group.
    Additive,
    // Evicts (blends out) other layers in the same ordering group.
    OneOnly
};

// Where the composed POV is delivered.
UENUM(BlueprintType)
enum class ECk_Camera_OutputMode : uint8
{
    // Drive a UCk_CameraComponent (default PCM pulls it via GetCameraView). Default.
    DriveCameraComponent,
    // Drive a USceneCaptureComponent2D (non-player viewer). [M2+]
    DriveSceneCapture,
    // Compute at PCM-read time via a custom manager. [optional fallback]
    DriveViewInfo
};

// --------------------------------------------------------------------------------------------------------------------
// REQUESTS
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_Camera_AddLayer : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Camera_AddLayer);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Camera_AddLayer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraLayer_EntityScript> _LayerClass;

    // Layer priority. Higher = more dominant (wins look-at). Also the OneOnly "slot": a OneOnly layer evicts existing
    // layers of the SAME priority. Layers that should coexist (e.g. a transient FOV trim over a mode) use distinct priorities.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Camera_StackingBehavior _StackingBehavior = ECk_Camera_StackingBehavior::Additive;

    // Time to blend this layer in (0 = instant).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _BlendInTime = FCk_Time{0.25};

    // Optional look-at target (for auto-reorient / lock-on). Invalid = no look-at from this layer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _LookAtTarget;

public:
    CK_PROPERTY_GET(_LayerClass);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_StackingBehavior);
    CK_PROPERTY(_BlendInTime);
    CK_PROPERTY(_LookAtTarget);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Camera_AddLayer, _LayerClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_Camera_RemoveLayer : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Camera_RemoveLayer);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Camera_RemoveLayer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraLayer_EntityScript> _LayerClass;

    // Time to blend this layer out before it is destroyed (0 = instant).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _BlendOutTime = FCk_Time{0.25};

public:
    CK_PROPERTY_GET(_LayerClass);
    CK_PROPERTY(_BlendOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Camera_RemoveLayer, _LayerClass);
};

// --------------------------------------------------------------------------------------------------------------------
// PARAMS
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Fragment_Camera_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Camera_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Camera_OutputMode _OutputMode = ECk_Camera_OutputMode::DriveCameraComponent;

    // Optional pre-existing output component. If null and OutputMode == DriveCameraComponent,
    // a UCk_CameraComponent is auto-created on the owning actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UCameraComponent> _OutputComponent;

    // The default profile. Every leaf field is materialized into a (non-replicated) tuner attribute on the camera
    // entity; camera layers acquire modifiers on those attributes. The bool/curve leaves live on the Current fragment.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_CameraProfile _Profile;

    // When true and the director's owning actor is a pawn possessed by a LOCAL APlayerController, the UpdatePOV
    // processor publishes the resolved view rotation back to that controller each frame (SetControlRotation) so
    // control-rotation consumers (facing / aim / movement) follow the camera. For a player view; leave false for
    // non-player viewers (scene-capture, fixed cams).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _DriveControllerControlRotation = false;

public:
    CK_PROPERTY_GET(_OutputMode);
    CK_PROPERTY_GET(_OutputComponent);
    CK_PROPERTY(_Profile);
    CK_PROPERTY(_DriveControllerControlRotation);
};

// --------------------------------------------------------------------------------------------------------------------
