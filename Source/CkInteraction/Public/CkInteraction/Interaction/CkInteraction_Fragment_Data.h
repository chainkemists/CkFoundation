#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkInteraction_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Interaction_Setup;
    class FProcessor_Interaction_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_InteractionDuration_FloatAttribute_Name);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Interaction_CompletionPolicy : uint8
{
    Instant,
    Timed,
	ManuallyCompleted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Interaction_CompletionPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINTERACTION_API FCk_Handle_Interaction : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Interaction); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Interaction);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_Interaction_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Interaction_ParamsData);

public:
    friend class ck::FProcessor_Interaction_Setup;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    FGameplayTag _InteractionChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Interaction_CompletionPolicy _CompletionPolicy = ECk_Interaction_CompletionPolicy::Timed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_CompletionPolicy==ECk_InteractionCompletionPolicy::Timed", EditConditionHides))
    FCk_Time _InteractionDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

public:
	CK_PROPERTY_GET(_InteractionChannel)
	CK_PROPERTY_GET(_Source)
	CK_PROPERTY_GET(_Instigator)
	CK_PROPERTY_GET(_Target)
	CK_PROPERTY_GET(_CompletionPolicy)
	CK_PROPERTY_GET(_InteractionDuration)
	CK_PROPERTY_GET(_ConstructionScript)

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Interaction_ParamsData, _InteractionChannel, _Source, _Instigator, _Target, _CompletionPolicy, _InteractionDuration, _ConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_MultipleInteraction_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInteraction_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Interaction_ParamsData> _InteractionParams;

public:
    CK_PROPERTY_GET(_InteractionParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInteraction_ParamsData, _InteractionParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_Interaction_EndInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Interaction_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Interaction_EndInteraction);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SucceededFailed _SuccessFail;

public:
    CK_PROPERTY_GET(_SuccessFail)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Interaction_EndInteraction, _SuccessFail);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Interaction_OnInteractionFinished,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Interaction_OnInteractionFinished_MC,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------