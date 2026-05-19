#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

// Need FCk_Handle_Goap_Tier for the OnActiveTiersChanged payload's TArray<>.
#include "CkGoap/Tier/CkGoap_Tier_Fragment_Data.h"

#include "CkGoap_Bundle_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Bundle : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Bundle);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Bundle);

// ====================================================================================================================
// BUNDLE PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_BundleParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_BundleParamsData);

private:
	// Bundle identity within a Goap root. Unique per root entity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap.Bundle"))
	FGameplayTag _BundleTag;

	// Initial enable/disable state. Disabled bundles skip per-tier planning
	// and chain-update. Toggle at runtime via Request_SetEnableToggle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_EnableDisable _InitialToggle = ECk_EnableDisable::Enable;

public:
	CK_PROPERTY_GET(_BundleTag);
	CK_PROPERTY(_InitialToggle);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_BundleParamsData, _BundleTag);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — Bundle-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActiveTiersChanged
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnActiveTiersChanged);

private:
	// Snapshot of the chain BEFORE this frame's mutation. Current chain is
	// readable via UCk_Utils_Goap_Bundle_UE::Get_ActiveTiers in the handler.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_Handle_Goap_Tier> _OldChain;

public:
	CK_PROPERTY_GET(_OldChain);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnActiveTiersChanged, _OldChain);
};

// ====================================================================================================================
// DELEGATES — Bundle-scoped
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnActiveTiersChanged,
	FCk_Handle_Goap_Bundle, InBundle,
	FCk_Goap_Payload_OnActiveTiersChanged, InPayload);

