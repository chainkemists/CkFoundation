#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include <Particles/ParticleSystemComponent.h>
#include <GameplayTagContainer.h>
#include <NiagaraSystem.h>

#include "CkVfx_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFX_API FCk_Handle_Vfx : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Vfx); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Vfx);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VFX_Location_Policy : uint8
{
    UseRelativeLocation,
    UseAbsoluteLocation
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VFX_Location_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VFX_Rotation_Policy : uint8
{
    UseRelativeRotation,
    UseAbsoluteRotation
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VFX_Rotation_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VFX_Scale_Policy : uint8
{
    UseRelativeScale,
    UseAbsoluteScale
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VFX_Scale_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VFX_Attachment_Policy : uint8
{
    StayPut,
    Follow
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VFX_Attachment_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Vfx_TransformRules
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vfx_TransformRules);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VFX_Location_Policy _LocationPolicy = ECk_VFX_Location_Policy::UseRelativeLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VFX_Rotation_Policy _RotationPolicy = ECk_VFX_Rotation_Policy::UseRelativeRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VFX_Scale_Policy _ScalePolicy = ECk_VFX_Scale_Policy::UseRelativeScale;

public:
    CK_PROPERTY(_LocationPolicy);
    CK_PROPERTY(_RotationPolicy);
    CK_PROPERTY(_ScalePolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Vfx_TransformSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vfx_TransformSettings);

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
struct CKFX_API FCk_Vfx_AttachmentSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vfx_AttachmentSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VFX_Attachment_Policy _AttachmentPolicy = ECk_VFX_Attachment_Policy::StayPut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _SocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_TransformRules _TransformRules;

public:
    CK_PROPERTY(_AttachmentPolicy);
    CK_PROPERTY(_SocketName);
    CK_PROPERTY(_TransformRules);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Vfx_InstanceParameterSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vfx_InstanceParameterSettings);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FParticleSysParam> _ParticleSysParams;

public:
    CK_PROPERTY_GET(_ParticleSysParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Vfx_InstanceParameterSettings, _ParticleSysParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_Vfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Vfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Vfx"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UNiagaraSystem> _ParticleSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_AttachmentSettings _AttachmentSettings;

public:
    CK_PROPERTY(_Name);
    CK_PROPERTY(_ParticleSystem);
    CK_PROPERTY_GET(_AttachmentSettings);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Vfx_ParamsData, _Name, _ParticleSystem);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Fragment_MultipleVfx_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleVfx_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Vfx_ParamsData> _VfxParams;

public:
    CK_PROPERTY_GET(_VfxParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleVfx_ParamsData, _VfxParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Request_Vfx_Stop
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vfx_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Request_Vfx_PlayAttached
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vfx_PlayAttached);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<USceneComponent> _AttachComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_TransformSettings _RelativeTransformSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_InstanceParameterSettings _InstanceParameterSettings;

public:
    CK_PROPERTY_GET(_AttachComponent);
    CK_PROPERTY(_RelativeTransformSettings);
    CK_PROPERTY(_InstanceParameterSettings);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Vfx_PlayAttached, _AttachComponent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFX_API FCk_Request_Vfx_PlayAtLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vfx_PlayAtLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UObject> _Outer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_TransformSettings _TransformSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Vfx_InstanceParameterSettings _InstanceParameterSettings;

public:
    CK_PROPERTY_GET(_Outer);
    CK_PROPERTY_GET(_TransformSettings);
    CK_PROPERTY(_InstanceParameterSettings);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Vfx_PlayAtLocation, _Outer, _TransformSettings);
};

// --------------------------------------------------------------------------------------------------------------------
