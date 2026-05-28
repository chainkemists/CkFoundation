#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <CoreMinimal.h>

#include "CkGameplayCamera_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraModifier_EntityScript;
class UCameraComponent;

// --------------------------------------------------------------------------------------------------------------------
// TYPE-SAFE HANDLES
// --------------------------------------------------------------------------------------------------------------------

// The "director"/stack owner — one per local viewer. Owns the active-modifier Record + composed view info.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_GameplayCamera : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GameplayCamera); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GameplayCamera);

// A single active camera "mode" — a child entity of the director, driven by a UCk_CameraModifier_EntityScript.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraModifier : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraModifier); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraModifier);

// --------------------------------------------------------------------------------------------------------------------
// ENUMS
// --------------------------------------------------------------------------------------------------------------------

// Whether a modifier's per-frame DoTick is invoked (mirrors ECk_SmTaskMode).
UENUM(BlueprintType)
enum class ECk_GameplayCamera_TickMode : uint8
{
    // DoEnter/DoExit + DoContributeToProfile only; DoTick is never called.
    EnterExitOnly,
    // DoTick is called every frame while the modifier is active.
    Tick
};

// How a newly-added modifier interacts with existing modifiers in the same ordering group.
UENUM(BlueprintType)
enum class ECk_GameplayCamera_StackingBehavior : uint8
{
    // Coexists with other modifiers in the group.
    Additive,
    // Evicts (blends out) other modifiers in the same ordering group.
    OneOnly
};

// Where the composed POV is delivered.
UENUM(BlueprintType)
enum class ECk_GameplayCamera_OutputMode : uint8
{
    // Drive a UCk_GameplayCameraComponent (default PCM pulls it via GetCameraView). Default.
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
struct CKCAMERA_API FCk_Request_GameplayCamera_AddModifier : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameplayCamera_AddModifier);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GameplayCamera_AddModifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_GameplayCamera_StackingBehavior _StackingBehavior = ECk_GameplayCamera_StackingBehavior::Additive;

public:
    CK_PROPERTY_GET(_ModifierClass);
    CK_PROPERTY(_StackingBehavior);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameplayCamera_AddModifier, _ModifierClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_GameplayCamera_RemoveModifier : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameplayCamera_RemoveModifier);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GameplayCamera_RemoveModifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;

public:
    CK_PROPERTY_GET(_ModifierClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameplayCamera_RemoveModifier, _ModifierClass);
};

// --------------------------------------------------------------------------------------------------------------------
// PARAMS
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Fragment_GameplayCamera_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_GameplayCamera_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_GameplayCamera_OutputMode _OutputMode = ECk_GameplayCamera_OutputMode::DriveCameraComponent;

    // Optional pre-existing output component. If null and OutputMode == DriveCameraComponent,
    // a UCk_GameplayCameraComponent is auto-created on the owning actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCameraComponent> _OutputComponent = nullptr;

public:
    CK_PROPERTY_GET(_OutputMode);
    CK_PROPERTY_GET(_OutputComponent);
};

// --------------------------------------------------------------------------------------------------------------------
