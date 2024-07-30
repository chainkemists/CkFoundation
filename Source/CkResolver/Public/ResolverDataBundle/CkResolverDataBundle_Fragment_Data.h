#pragma once

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkResolverDataBundle_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRESOLVER_API FCk_Handle_ResolverSource : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ResolverSource); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ResolverSource);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRESOLVER_API FCk_Handle_ResolverTarget : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ResolverTarget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ResolverTarget);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRESOLVER_API FCk_Handle_ResolverDataBundle : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ResolverDataBundle); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ResolverDataBundle);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ResolverDataBundle_ModifierComponent : uint8
{
    BaseValue,
    BonusValue,
    TotalScalar
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ResolverDataBundle_ModifierComponent);

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_ResolverDataBundle_BaseValue);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_ResolverDataBundle_BonusValue);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_ResolverDataBundle_TotalScalarValue);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ResolverDataBundle_AllowedOperationsInPhase : uint8
{
    None,
    ModifierOnly,
    MetadataOnly,
    ModifierAndMetadata,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ResolverDataBundle_AllowedOperationsInPhase);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_ResolverDataBundle_ModifierOperation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResolverDataBundle_ModifierOperation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ResolverDataBundle_ModifierComponent _ResolverComponent = ECk_ResolverDataBundle_ModifierComponent::BaseValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
          meta = (AllowPrivateAccess = true))
    ECk_ArithmeticOperations_Basic _ModifierOperation = ECk_ArithmeticOperations_Basic::Add;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ModifierDelta = 0.0f;

public:
    CK_PROPERTY(_ResolverComponent);
    CK_PROPERTY(_ModifierOperation);
    CK_PROPERTY_GET(_ModifierDelta);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResolverDataBundle_ModifierOperation, _ModifierDelta);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_ResolverDataBundle_ModifierOperation_Conditional
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResolverDataBundle_ModifierOperation_Conditional);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagRequirements _BundleTagRequirements;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ResolverDataBundle_ModifierOperation _Operation;

public:
    CK_PROPERTY(_BundleTagRequirements);
    CK_PROPERTY(_Operation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_ResolverDataBundle_MetadataOperation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResolverDataBundle_MetadataOperation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Resolver.DataBundle.Metadata"))
    FGameplayTagContainer _TagsToAdd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Resolver.DataBundle.Metadata"))
    FGameplayTagContainer _TagsToRemove;

public:
    CK_PROPERTY(_TagsToAdd);
    CK_PROPERTY(_TagsToRemove);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_ResolverDataBundle_MetadataOperation_Conditional
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResolverDataBundle_MetadataOperation_Conditional);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagRequirements _BundleTagRequirements;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ResolverDataBundle_MetadataOperation _Operation;

public:
    CK_PROPERTY(_BundleTagRequirements);
    CK_PROPERTY(_Operation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Fragment_ResolverDataBundle_PhaseInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ResolverDataBundle_PhaseInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Resolver.DataBundle.Phase"))
    FGameplayTag _PhaseName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ResolverDataBundle_AllowedOperationsInPhase _AllowedOperations = ECk_ResolverDataBundle_AllowedOperationsInPhase::None;

public:
    CK_PROPERTY_GET(_PhaseName);
    CK_PROPERTY_GET(_AllowedOperations);
};


// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Fragment_ResolverDataBundle_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ResolverDataBundle_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Resolver.DataBundle.Name"))
    FGameplayTag _BundleName;

    // This is the Entity that ultimately owns the outgoing Resolver
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverSource _Instigator;

    // This is the Entity which is being Resolved
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverTarget _Target;

    // This is the character/bullet/sword/grenade/etc. that is the ResolverCause
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle _Causer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_ResolverDataBundle_PhaseInfo> _Phases;

public:
    CK_PROPERTY_GET(_Instigator);
    CK_PROPERTY_GET(_Target);
    CK_PROPERTY_GET(_Causer);
    CK_PROPERTY_GET(_Phases);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ResolverDataBundle_ParamsData, _BundleName, _Instigator, _Target, _Causer, _Phases);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FRequest_ResolverDataBundle_ModifierOperation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FRequest_ResolverDataBundle_ModifierOperation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ResolverDataBundle_ModifierOperation_Conditional _ModifierOperation;

public:
    CK_PROPERTY(_ModifierOperation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FRequest_ResolverDataBundle_MetadataOperation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FRequest_ResolverDataBundle_MetadataOperation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ResolverDataBundle_MetadataOperation_Conditional _MetadataOperation;

public:
    CK_PROPERTY(_MetadataOperation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Payload_ResolverDataBundle_Resolved
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_ResolverDataBundle_Resolved);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverDataBundle _DataBundle;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverSource _Instigator;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverTarget _Target;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ResolverCause;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _FinalValue = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _Metadata;

public:
    CK_PROPERTY(_DataBundle);
    CK_PROPERTY(_Instigator);
    CK_PROPERTY(_Target);
    CK_PROPERTY(_ResolverCause);
    CK_PROPERTY(_FinalValue);
    CK_PROPERTY(_Metadata);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ResolverDataBundle_OnPhaseStart,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FGameplayTag, InPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ResolverDataBundle_OnPhaseStart_MC,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FGameplayTag, InPhase);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ResolverDataBundle_OnPhaseComplete,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FGameplayTag, InPhase,
    FCk_Payload_ResolverDataBundle_Resolved, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_ResolverDataBundle_OnPhaseComplete_MC,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FGameplayTag, InPhase,
    FCk_Payload_ResolverDataBundle_Resolved, InPayload);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FCk_Payload_ResolverDataBundle_Resolved, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete_MC,
    FCk_Handle_ResolverDataBundle, InDataBundle,
    FCk_Payload_ResolverDataBundle_Resolved, InPayload);

// --------------------------------------------------------------------------------------------------------------------