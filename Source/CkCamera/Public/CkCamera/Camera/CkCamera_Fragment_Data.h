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
class UCk_CameraComponent;

// --------------------------------------------------------------------------------------------------------------------
// TYPE-SAFE HANDLES
// --------------------------------------------------------------------------------------------------------------------

// The "director"/stack owner — one per local viewer. Every profile field lives as a non-replicated attribute on it.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_Camera : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Camera); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Camera);

// A single camera "layer" — a child entity of the director, driven by a UCk_CameraLayer_EntityScript.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraLayer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraLayer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraLayer);

// --------------------------------------------------------------------------------------------------------------------
// ENUMS
// --------------------------------------------------------------------------------------------------------------------

// Whether a layer's per-frame DoTick is invoked.
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

// --------------------------------------------------------------------------------------------------------------------
// CAMERA TARGET
// --------------------------------------------------------------------------------------------------------------------

// LookAt re-orients the rig toward the target's location (the rig stays anchored to the camera entity). ViewTarget
// blends the final composed POV to the target's full transform, easing on the layer's own blend alpha.
UENUM(BlueprintType)
enum class ECk_Camera_TargetMode : uint8
{
    LookAt,
    ViewTarget
};

// A camera layer's optional target. An invalid _Target means the layer contributes no target; the two _Mode
// branches are mutually exclusive by construction.
USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Camera_Target
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Camera_Target);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Camera_TargetMode _Mode = ECk_Camera_TargetMode::LookAt;

public:
    CK_PROPERTY(_Target);
    CK_PROPERTY(_Mode);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Camera_Target, _Target, _Mode);
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

    // Optional camera target for this layer; an invalid target means this layer contributes none.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Camera_Target _CameraTarget;

public:
    CK_PROPERTY_GET(_LayerClass);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_StackingBehavior);
    CK_PROPERTY(_BlendInTime);
    CK_PROPERTY(_CameraTarget);

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

// Switches a live camera between perspective and orthographic, carrying the depth range the orthographic view
// should use.
//
// The planes travel WITH the mode rather than being set separately because they are only meaningful together: an
// orthographic view with a perspective camera's near/far clips the scene at the wrong distances, and there is no
// moment at which a caller wants one without having decided the other.
USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_Camera_SetProjectionMode : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Camera_SetProjectionMode);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Camera_SetProjectionMode);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Camera_ProjectionMode _ProjectionMode = ECk_Camera_ProjectionMode::Perspective;

    // Unset leaves the camera's current planes alone, which is what makes a bare mode flip a one-liner while a
    // caller that cares about depth range can still say so in the same request.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<float> _OrthoNearClipPlane;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<float> _OrthoFarClipPlane;

public:
    CK_PROPERTY_GET(_ProjectionMode);
    CK_PROPERTY(_OrthoNearClipPlane);
    CK_PROPERTY(_OrthoFarClipPlane);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Camera_SetProjectionMode, _ProjectionMode);
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
    TWeakObjectPtr<class UCk_CameraComponent> _OutputComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_CameraProfile _Profile;

    // Publishes the resolved view rotation back to the owning pawn's LOCAL APlayerController each frame, so
    // control-rotation consumers (facing / aim / movement) follow the camera. Leave false for non-player viewers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _DriveControllerControlRotation = false;

public:
    CK_PROPERTY_GET(_OutputComponent);
    CK_PROPERTY(_Profile);
    CK_PROPERTY(_DriveControllerControlRotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Camera_ParamsData, _OutputComponent);
};

// --------------------------------------------------------------------------------------------------------------------
