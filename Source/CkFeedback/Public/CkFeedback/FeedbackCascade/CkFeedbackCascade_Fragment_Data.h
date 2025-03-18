#pragma once

#include "CkCameraShake_Fragment_Data.h"
#include "CkSfx_Fragment_Data.h"
#include "CkVfx_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkFeedbackCascade_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFEEDBACK_API FCk_Handle_FeedbackCascade : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FeedbackCascade); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FeedbackCascade);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFEEDBACK_API FCk_Fragment_FeedbackCascade_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FeedbackCascade_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FeedbackCascade"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasVfx = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasVfx"))
    FCk_Fragment_MultipleVfx_ParamsData _Vfx;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasSfx = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasSfx"))
    FCk_Fragment_MultipleSfx_ParamsData _Sfx;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasCameraShake = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasCameraShake"))
    FCk_Fragment_MultipleCameraShake_ParamsData _CameraShake;

    // TODO: Add support for AI Noise
    // TODO: Add support for ForceFeedback (controller rumble)

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_HasVfx);
    CK_PROPERTY_GET(_Vfx);
    CK_PROPERTY_GET(_HasSfx);
    CK_PROPERTY_GET(_Sfx);
    CK_PROPERTY_GET(_HasCameraShake);
    CK_PROPERTY_GET(_CameraShake);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFEEDBACK_API FCk_Fragment_MultipleFeedbackCascade_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleFeedbackCascade_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_FeedbackCascade_ParamsData> _FeedbackCascadeParams;

public:
    CK_PROPERTY_GET(_FeedbackCascadeParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleFeedbackCascade_ParamsData, _FeedbackCascadeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFEEDBACK_API FCk_Request_FeedbackCascade_PlayAtLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FeedbackCascade_PlayAtLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _Rotation = FRotator::ZeroRotator;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFEEDBACK_API FCk_Request_FeedbackCascade_PlayAttached
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FeedbackCascade_PlayAttached);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<USceneComponent> _AttachComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _AttachedLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _AttachedRotation = FRotator::ZeroRotator;

public:
    CK_PROPERTY_GET(_AttachComponent);
    CK_PROPERTY_GET(_AttachedLocation);
    CK_PROPERTY_GET(_AttachedRotation);
};
// --------------------------------------------------------------------------------------------------------------------