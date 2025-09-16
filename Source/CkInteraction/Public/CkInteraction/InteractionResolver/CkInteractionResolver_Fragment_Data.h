#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkInteractionResolver_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINTERACTION_API FCk_Handle_InteractionResolver : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InteractionResolver); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InteractionResolver);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InteractionResolver_DistanceSorting : uint8
{
    Disabled,
    Enabled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InteractionResolver_DistanceSorting);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_InteractionResolver_IntentChannelMapping
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InteractionResolver_IntentChannelMapping);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "InteractionIntent"))
    FGameplayTag _Intent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    TArray<FGameplayTag> _Channels; // Ordered by priority (first = highest priority)

public:
    CK_PROPERTY_GET(_Intent);
    CK_PROPERTY_GET(_Channels);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InteractionResolver_IntentChannelMapping, _Intent, _Channels);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_InteractionResolver_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InteractionResolver_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, TitleProperty = "_Intent"))
    TArray<FCk_InteractionResolver_IntentChannelMapping> _IntentChannelMappings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_InteractionResolver_DistanceSorting _DistanceSorting = ECk_InteractionResolver_DistanceSorting::Enabled;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _MaxConcurrentInteractions = 1;

public:
    CK_PROPERTY_GET(_IntentChannelMappings);
    CK_PROPERTY(_DistanceSorting);
    CK_PROPERTY(_MaxConcurrentInteractions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InteractionResolver_ParamsData, _IntentChannelMappings);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractionResolver_StartIntent : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InteractionResolver_StartIntent);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractionResolver_StartIntent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "InteractionIntent"))
    FGameplayTag _Intent;

public:
    CK_PROPERTY_GET(_Intent);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractionResolver_StartIntent, _Intent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractionResolver_StopIntent : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InteractionResolver_StopIntent);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractionResolver_StopIntent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "InteractionIntent"))
    FGameplayTag _Intent;

public:
    CK_PROPERTY_GET(_Intent);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractionResolver_StopIntent, _Intent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractionResolver_AddInteractTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InteractionResolver_AddInteractTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractionResolver_AddInteractTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_InteractTarget _Target;

public:
    CK_PROPERTY_GET(_Target);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractionResolver_AddInteractTarget, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractionResolver_RemoveInteractTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InteractionResolver_RemoveInteractTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractionResolver_RemoveInteractTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_InteractTarget _Target;

public:
    CK_PROPERTY_GET(_Target);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractionResolver_RemoveInteractTarget, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractionResolver_RemoveAllTargetsByChannel : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InteractionResolver_RemoveAllTargetsByChannel);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractionResolver_RemoveAllTargetsByChannel);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    FGameplayTag _Channel;

public:
    CK_PROPERTY_GET(_Channel);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractionResolver_RemoveAllTargetsByChannel, _Channel);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FiveParams(
    FCk_Delegate_InteractionResolver_OnBestTargetsChanged,
    FCk_Handle_InteractionResolver, InResolver,
    FGameplayTag, InIntent,
    const TArray<FCk_Handle_InteractTarget>&, InPreviousTargets,
    const TArray<FCk_Handle_InteractTarget>&, InNewTargets,
    const TArray<FCk_Handle_InteractTarget>&, InRemovedTargets);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FCk_Delegate_InteractionResolver_OnBestTargetsChanged_MC,
    FCk_Handle_InteractionResolver, InResolver,
    FGameplayTag, InIntent,
    const TArray<FCk_Handle_InteractTarget>&, InPreviousTargets,
    const TArray<FCk_Handle_InteractTarget>&, InNewTargets,
    const TArray<FCk_Handle_InteractTarget>&, InRemovedTargets);

// --------------------------------------------------------------------------------------------------------------------