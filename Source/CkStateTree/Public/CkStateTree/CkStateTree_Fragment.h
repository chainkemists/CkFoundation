#pragma once

#include "CkStateTree_Fragment_Data.h"

#include <StateTreeInstanceData.h>

#include <CkStateTree/CkStateTree_Owner.h>

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	CK_DEFINE_ECS_TAG(FTag_StateTree_RequiresSetup);
	CK_DEFINE_ECS_TAG(FTag_StateTree_NeedsTick);

	// --------------------------------------------------------------------------------------------------------------------

	struct CKSTATETREE_API FFragment_StateTree_Params
	{
	public:
		CK_GENERATED_BODY(FFragment_StateTree_Params);

	public:
		using ParamsType = FCk_Fragment_StateTree_ParamsData;

	private:
		ParamsType _Params;

	public:
		CK_PROPERTY_GET(_Params);

	public:
		CK_DEFINE_CONSTRUCTORS(FFragment_StateTree_Params, _Params);
	};

	// --------------------------------------------------------------------------------------------------------------------

	struct CKSTATETREE_API FFragment_StateTree_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_StateTree_Current);

	public:
		friend class FProcessor_StateTree_Setup;
		friend class FProcessor_StateTree_Tick;
		friend class FProcessor_StateTree_HandleRequests;
		friend class FProcessor_StateTree_EndPlay;
		friend class UCk_Utils_StateTree_UE;

	private:
		/** Owner object for StateTree execution */
		TStrongObjectPtr<UCk_StateTree_Owner> _Owner;

		/** Runtime instance data for the StateTree */
		FStateTreeInstanceData _InstanceData;

		/** Current execution status */
		ECk_StateTree_RunStatus _RunStatus = ECk_StateTree_RunStatus::Stopped;

	public:
		CK_PROPERTY_GET(_Owner);
		CK_PROPERTY_GET(_InstanceData);
		CK_PROPERTY_GET(_RunStatus);
	};

	// --------------------------------------------------------------------------------------------------------------------

	struct CKSTATETREE_API FFragment_StateTree_Requests
	{
	public:
		CK_GENERATED_BODY(FFragment_StateTree_Requests);

	public:
		friend class FProcessor_StateTree_Setup;
		friend class FProcessor_StateTree_HandleRequests;
		friend class UCk_Utils_StateTree_UE;

	public:
		using RequestType = std::variant<
			FCk_Request_StateTree_StartLogic,
			FCk_Request_StateTree_RestartLogic,
			FCk_Request_StateTree_StopLogic,
			FCk_Request_StateTree_PauseLogic,
			FCk_Request_StateTree_ResumeLogic
		>;
		using RequestList = TArray<RequestType>;

	private:
		RequestList _Requests;

	public:
		CK_PROPERTY_GET(_Requests);
	};

	// --------------------------------------------------------------------------------------------------------------------

	CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_StateTree_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
