#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

// Need FCk_Handle_Goap_Action for the OnActiveChainChanged payload's TArray<>.
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkGoap_ActionSet_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_ActionSet : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_ActionSet);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_ActionSet);

// ====================================================================================================================
// ACTIONSET PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_ActionSetParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_ActionSetParamsData);

private:
	// ActionSet identity within a Goap root. Unique per root entity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap.ActionSet"))
	FGameplayTag _ActionSetTag;

	// Initial enable/disable state. Disabled ActionSets skip per-Action
	// planning and chain-update. Toggle at runtime via Request_SetEnableToggle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_EnableDisable _InitialToggle = ECk_EnableDisable::Enable;

public:
	CK_PROPERTY_GET(_ActionSetTag);
	CK_PROPERTY(_InitialToggle);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_ActionSetParamsData, _ActionSetTag);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — ActionSet-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActiveChainChanged
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnActiveChainChanged);

private:
	// Snapshot of the chain BEFORE this frame's mutation. Current chain is
	// readable via UCk_Utils_Goap_ActionSet_UE::Get_ActiveChain in the handler.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_Handle_Goap_Action> _OldChain;

public:
	CK_PROPERTY_GET(_OldChain);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnActiveChainChanged, _OldChain);
};

// ====================================================================================================================
// DELEGATES — ActionSet-scoped
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnActiveChainChanged,
	FCk_Handle_Goap_ActionSet, InActionSet,
	FCk_Goap_Payload_OnActiveChainChanged, InPayload);

