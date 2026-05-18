#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkGoap_WorldState_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================
//
// A FCk_Handle_Goap_WorldState identifies an entity that holds a shared
// boolean world state used by one or more GOAP planners. Planners reference
// a WorldState entity via FCk_Fragment_Goap_ParamsData::_WorldStateSource;
// reads, writes, and replan-trigger subscriptions all route through the
// referenced WorldState. Multiple planners pointing at the same WorldState
// observe each other's writes through the OnValueChanged signal.
//
// Lifetime: caller-owned. Create explicitly with utils_goap_worldstate::Create
// under any owner entity. Destruction cascades from the owner via CkRecord.

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_WorldState : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_WorldState);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_WorldState);

// ====================================================================================================================
// PARAMS
// ====================================================================================================================
//
// Placeholder for per-WorldState configuration. Empty today — added now so
// the Create signature stays stable when future knobs land (e.g. max-keys
// override, replication policy, debug-name).

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_WorldState_ParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_WorldState_ParamsData);
};

// ====================================================================================================================
// SIGNAL PAYLOAD
// ====================================================================================================================
//
// Fired by the WorldState's request processor whenever a Set request actually
// changes a key's value. Same-value writes are coalesced and do not fire.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_WorldState_Payload_OnValueChanged
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_WorldState_Payload_OnValueChanged);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _OldValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _NewValue = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_OldValue);
	CK_PROPERTY_GET(_NewValue);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_WorldState_Payload_OnValueChanged, _Key, _OldValue, _NewValue);
};

// ====================================================================================================================
// DELEGATE
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_WorldState_OnValueChanged,
	FCk_Handle_Goap_WorldState, InHandle,
	FCk_Goap_WorldState_Payload_OnValueChanged, InPayload);

// ====================================================================================================================
// REQUESTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_WorldState_SetValue
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_WorldState_SetValue);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_WorldState_SetValue, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

// Pre-registers a key with the registry so subsequent Set/Get on it route to
// a stable slot. Normally callers don't need this — Setup-time scanning of
// GOAP actions/goals registers keys automatically, and Set_Value also lazily
// registers. Useful when seeding values before any planner is attached.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_WorldState_RegisterKey
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_WorldState_RegisterKey);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

public:
	CK_PROPERTY_GET(_Key);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_WorldState_RegisterKey, _Key);
};

// ====================================================================================================================
