#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkCameraShake_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCAMERA_API FCk_Handle_CameraShake : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CameraShake); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CameraShake);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_CameraShake_PlayOnTarget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_CameraShake_PlayOnTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Target;

public:
    CK_PROPERTY_GET(_Target);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CameraShake_PlayOnTarget, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Request_CameraShake_PlayAtLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_CameraShake_PlayAtLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::Zero();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _WorldContextObject;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_WorldContextObject);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CameraShake_PlayAtLocation, _Location, _WorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Fragment_CameraShake_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_CameraShake_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<class UCameraShakeBase> _CameraShake;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Scale = 1.f;

    // If ticked, changes the rotation of shake to point towards epicenter instead of forward
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _OrientTowardsEpicenter = false;

    // Defines radius at which Camera shake is full Strength
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _InnerRadius = 1000.f;

    // Defines radius at which Camera shake is at zero Strength
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _OuterRadius = 5000.f;

    // Exponent that describes the shake intensity falloff curve between InnerRadius and OuterRadius. 1.0 is linear
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Falloff = 1.0f;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_CameraShake);
    CK_PROPERTY(_Scale);
    CK_PROPERTY(_OrientTowardsEpicenter);
    CK_PROPERTY(_InnerRadius);
    CK_PROPERTY(_OuterRadius);
    CK_PROPERTY(_Falloff);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CameraShake_ParamsData, _Name, _CameraShake);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_Fragment_MultipleCameraShake_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleCameraShake_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_CameraShake_ParamsData> _CameraShakeParams;

public:
    CK_PROPERTY_GET(_CameraShakeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleCameraShake_ParamsData, _CameraShakeParams);
};

// --------------------------------------------------------------------------------------------------------------------