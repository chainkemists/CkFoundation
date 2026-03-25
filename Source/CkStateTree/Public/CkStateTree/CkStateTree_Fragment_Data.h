#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkStateTree_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UStateTree;

namespace ck
{
	class FProcessor_StateTree_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATETREE_API FCk_Handle_StateTree : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_StateTree); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_StateTree);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_StateTree_AutoStart : uint8
{
	Disabled,	// Must call Request_StartLogic manually
	OnSetup		// Auto-start when feature is setup (default)
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_StateTree_RunStatus : uint8
{
	Stopped,
	Running,
	Paused
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Fragment_StateTree_ParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_StateTree_ParamsData);

private:
	/** The StateTree asset to execute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (AllowPrivateAccess = true))
	TObjectPtr<UStateTree> _StateTree;

	/** Auto-start behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (AllowPrivateAccess = true))
	ECk_StateTree_AutoStart _AutoStart = ECk_StateTree_AutoStart::OnSetup;

public:
	CK_PROPERTY_GET(_StateTree);
	CK_PROPERTY(_AutoStart);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_StateTree_ParamsData, _StateTree);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Fragment_MultipleStateTree_ParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_MultipleStateTree_ParamsData);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (AllowPrivateAccess = true))
	TArray<FCk_Fragment_StateTree_ParamsData> _StateTreeParams;

public:
	CK_PROPERTY_GET(_StateTreeParams)

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleStateTree_ParamsData, _StateTreeParams);
};

// --------------------------------------------------------------------------------------------------------------------
// Request Structs
// --------------------------------------------------------------------------------------------------------------------

/** Start StateTree logic execution */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_StartLogic : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_StateTree_StartLogic);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_StartLogic);
};

// --------------------------------------------------------------------------------------------------------------------

/** Restart StateTree logic (stop + start) */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_RestartLogic : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_StateTree_RestartLogic);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_RestartLogic);
};

// --------------------------------------------------------------------------------------------------------------------

/** Stop StateTree logic execution */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_StopLogic : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_StateTree_StopLogic);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_StopLogic);
};

// --------------------------------------------------------------------------------------------------------------------

/** Pause StateTree logic execution */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_PauseLogic : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_StateTree_PauseLogic);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_PauseLogic);
};

// --------------------------------------------------------------------------------------------------------------------

/** Resume paused StateTree logic */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_ResumeLogic : public FCk_Request_Base
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_StateTree_ResumeLogic);
	CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_ResumeLogic);
};

// --------------------------------------------------------------------------------------------------------------------
