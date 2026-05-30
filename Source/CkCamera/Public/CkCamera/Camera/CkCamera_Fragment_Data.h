#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCamera_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraModifier_EntityScript;
class UCameraComponent;

// --------------------------------------------------------------------------------------------------------------------
// TYPE-SAFE HANDLES
// --------------------------------------------------------------------------------------------------------------------

// The "director"/stack owner — one per local viewer. Owns the active-modifier Record + composed view info.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_Camera : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Camera); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Camera);

// A single active camera "mode" — a child entity of the director, driven by a UCk_CameraModifier_EntityScript.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraModifier : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraModifier); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraModifier);

// The running composed-profile entity owned by a director. Holds the FCk_CameraProfile that active modifiers
// contribute into each frame. One per director, created in UCk_Utils_Camera_UE::Add and destroyed with it.
// Modifiers receive this handle in DoContributeToProfile (instead of a by-ref struct) and mutate it via
// UCk_Utils_CameraProfile_UE — sidestepping the AS/BP by-value-struct footgun.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraProfile : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraProfile); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraProfile);

// --------------------------------------------------------------------------------------------------------------------
// ENUMS
// --------------------------------------------------------------------------------------------------------------------

// Whether a modifier's per-frame DoTick is invoked (mirrors ECk_SmTaskMode).
UENUM(BlueprintType)
enum class ECk_Camera_TickMode : uint8
{
    // DoEnter/DoExit + DoContributeToProfile only; DoTick is never called.
    EnterExitOnly,
    // DoTick is called every frame while the modifier is active.
    Tick
};

// How a newly-added modifier interacts with existing modifiers in the same ordering group.
UENUM(BlueprintType)
enum class ECk_Camera_StackingBehavior : uint8
{
    // Coexists with other modifiers in the group.
    Additive,
    // Evicts (blends out) other modifiers in the same ordering group.
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
struct CKCAMERA_API FCk_Request_Camera_AddModifier : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Camera_AddModifier);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Camera_AddModifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;

    // Ordering-group/layer this modifier belongs to (used by OneOnly stacking + dominant selection).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _OrderingGroup;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Camera_StackingBehavior _StackingBehavior = ECk_Camera_StackingBehavior::Additive;

    // Time to blend this modifier in (0 = instant).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _BlendInTime = FCk_Time{0.25};

    // Optional look-at target (for auto-reorient / lock-on). Invalid = no look-at from this modifier.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _LookAtTarget;

public:
    CK_PROPERTY_GET(_ModifierClass);
    CK_PROPERTY(_OrderingGroup);
    CK_PROPERTY(_StackingBehavior);
    CK_PROPERTY(_BlendInTime);
    CK_PROPERTY(_LookAtTarget);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Camera_AddModifier, _ModifierClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_Camera_RemoveModifier : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Camera_RemoveModifier);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Camera_RemoveModifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;

    // Time to blend this modifier out before it is destroyed (0 = instant).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _BlendOutTime = FCk_Time{0.25};

public:
    CK_PROPERTY_GET(_ModifierClass);
    CK_PROPERTY(_BlendOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Camera_RemoveModifier, _ModifierClass);
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
    TObjectPtr<UCameraComponent> _OutputComponent = nullptr;

public:
    CK_PROPERTY_GET(_OutputMode);
    CK_PROPERTY_GET(_OutputComponent);
};

// --------------------------------------------------------------------------------------------------------------------
