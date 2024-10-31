#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkInteractTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_InteractTarget_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InteractionTarget_ConcurrentInteractionsPolicy : uint8
{
    SingleInteraction,
    MultipleInteractions
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InteractionTarget_ConcurrentInteractionsPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CanInteractWithResult : uint8
{
    CanInteractWith,
    TargetDisabled,
    TargetNotValid,
    SourceNotValid,
    ChannelMismatch,
    AlreadyExists,
    MultipleInteractionsDisabledForTarget,
    MultipleInteractionsDisabledForSource,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CanInteractWithResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINTERACTION_API FCk_Handle_InteractTarget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InteractTarget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InteractTarget);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_InteractTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InteractTarget_ParamsData);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    FGameplayTag _InteractionChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Interaction_CompletionPolicy _CompletionPolicy = ECk_Interaction_CompletionPolicy::Timed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_CompletionPolicy==ECk_InteractionCompletionPolicy::Timed", EditConditionHides))
    FCk_Time _InteractionDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _InteractionConstructionScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InteractionTarget_ConcurrentInteractionsPolicy _ConcurrentInteractionsPolicy = ECk_InteractionTarget_ConcurrentInteractionsPolicy::MultipleInteractions;

public:
	CK_PROPERTY_GET(_InteractionChannel)
	CK_PROPERTY_GET(_CompletionPolicy)
	CK_PROPERTY_GET(_InteractionDuration)
	CK_PROPERTY_GET(_InteractionConstructionScript)
	CK_PROPERTY_GET(_ConcurrentInteractionsPolicy)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_MultipleInteractTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInteractTarget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_InteractTarget_ParamsData> _InteractTargetParams;

public:
    CK_PROPERTY_GET(_InteractTargetParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInteractTarget_ParamsData, _InteractTargetParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Try_InteractTarget_StartInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_InteractTarget_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Try_InteractTarget_StartInteraction);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractInstigator;

public:
	CK_PROPERTY(_InteractSource)
	CK_PROPERTY(_InteractInstigator)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Try_InteractTarget_StartInteraction, _InteractSource, _InteractInstigator);
};

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractTarget_CancelInteraction : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	friend class ck::FProcessor_InteractTarget_HandleRequests;

public:
	CK_GENERATED_BODY(FCk_Request_InteractTarget_CancelInteraction);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (AllowPrivateAccess = true))
	FCk_Handle _InteractSource;

public:
	CK_PROPERTY(_InteractSource)

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractTarget_CancelInteraction, _InteractSource);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_InteractTarget_OnNewInteraction,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_InteractTarget_OnNewInteraction_MC,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_InteractTarget_OnInteractionFinished,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_InteractTarget_OnInteractionFinished_MC,
    FCk_Handle_InteractTarget, InTarget,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------