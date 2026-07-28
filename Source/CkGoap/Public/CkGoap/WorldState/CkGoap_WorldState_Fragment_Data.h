#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGoap_WorldState_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Planners reference one via FCk_Fragment_Goap_ParamsData::_WorldStateSource; several planners may
// share it. Lifetime is caller-owned: destruction cascades from the owner via CkRecord.

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_WorldState : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_WorldState);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_WorldState);

// --------------------------------------------------------------------------------------------------------------------
// Deliberately empty — reserved so the Add/Create signatures stay stable when knobs land.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_WorldState_ParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_WorldState_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------
// Fires only when a Set actually changes the value — same-value writes are coalesced and do not fire.

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

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_WorldState_OnValueChanged,
	FCk_Handle_Goap_WorldState, InHandle,
	FCk_Goap_WorldState_Payload_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_WorldState_SetValue : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_WorldState_SetValue);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_WorldState_SetValue);

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

// Rarely needed — Setup-time scanning of actions/goals and Set_Value both register keys already.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_WorldState_RegisterKey : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_WorldState_RegisterKey);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Goap_WorldState_RegisterKey);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

public:
	CK_PROPERTY_GET(_Key);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_WorldState_RegisterKey, _Key);
};

// --------------------------------------------------------------------------------------------------------------------
