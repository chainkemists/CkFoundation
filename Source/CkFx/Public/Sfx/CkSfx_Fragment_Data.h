#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkSfx_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFX_API FCk_Handle_Sfx : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Sfx); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Sfx);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Sfx_Stop_Policy : uint8
{
    StopWhenFinished,

    // Stop sounds immediately when attached component is destroyed
    StopWhenFinishedOrDetached
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Sfx_Stop_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Sfx_AudioSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sfx_AudioSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _VolumeMultipler = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _PitchMultipler = 1.0f;

public:
    CK_PROPERTY_GET(_VolumeMultipler);
    CK_PROPERTY_GET(_PitchMultipler);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Sfx_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sfx_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Sfx"))
    FGameplayTag _Name;

    // Soft by design: path-serialized, so it is safe to author in DataAssets/Blueprints (a weak ref
    // silently saves as null there; a hard ref force-loads the asset with the owning package). The
    // fragment never roots the asset — Setup resolves these through CkResourceLoader and the rooted
    // result lives on the Sfx's Current.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<USoundBase> _SoundCue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<USoundAttenuation> _AttenuationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<USoundConcurrency> _ConcurrencySettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sfx_AudioSettings _DefaultAudioSettings;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_SoundCue);
    CK_PROPERTY_GET(_AttenuationSettings);
    CK_PROPERTY_GET(_ConcurrencySettings);
    CK_PROPERTY_GET(_DefaultAudioSettings);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sfx_Spec, _Name, _SoundCue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_MultipleSfx_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MultipleSfx_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Sfx_Spec> _SfxParams;

public:
    CK_PROPERTY_GET(_SfxParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MultipleSfx_Spec, _SfxParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Sfx_TransformSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sfx_TransformSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _Rotation = FRotator::ZeroRotator;

public:
    CK_PROPERTY(_Location);
    CK_PROPERTY(_Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Request_Sfx_PlayAttached : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sfx_PlayAttached);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sfx_PlayAttached);

public:
    FCk_Request_Sfx_PlayAttached() = default;
    explicit FCk_Request_Sfx_PlayAttached(
        USceneComponent* InAttachComponent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<USceneComponent> _AttachComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Sfx_Stop_Policy _StopPolicy = ECk_Sfx_Stop_Policy::StopWhenFinished;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sfx_TransformSettings _RelativeTransformSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideAudioSettings = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideAudioSettings"))
    FCk_Sfx_AudioSettings _OverridenAudioSettings;

public:
    CK_PROPERTY_GET(_AttachComponent);
    CK_PROPERTY(_StopPolicy);
    CK_PROPERTY(_RelativeTransformSettings);
    CK_PROPERTY(_OverrideAudioSettings);
    CK_PROPERTY(_OverridenAudioSettings);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Request_Sfx_PlayAtLocation : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sfx_PlayAtLocation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sfx_PlayAtLocation);

public:
    FCk_Request_Sfx_PlayAtLocation() = default;
    FCk_Request_Sfx_PlayAtLocation(
        UObject* InOuter,
        FCk_Sfx_TransformSettings InTransformSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UObject> _Outer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sfx_TransformSettings _TransformSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideAudioSettings = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideAudioSettings"))
    FCk_Sfx_AudioSettings _OverridenAudioSettings;

public:
    CK_PROPERTY_GET(_Outer);
    CK_PROPERTY_GET(_TransformSettings);
    CK_PROPERTY(_OverrideAudioSettings);
    CK_PROPERTY(_OverridenAudioSettings);
};

// --------------------------------------------------------------------------------------------------------------------