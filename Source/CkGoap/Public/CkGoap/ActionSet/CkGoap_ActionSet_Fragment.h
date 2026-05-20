#pragma once

#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapDiagnostic_DependencyCycle
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_ActionSet_UE;
class UCk_Utils_Goap_Action_UE;

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_ActionSet_Setup;
	class FProcessor_Goap_ActionSet_ChainUpdate;

// ====================================================================================================================
// TAGS
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_ActionSet_RequiresSetup);

	// Set whenever any action in the ActionSet completes a plan. Consumed +
	// removed by ChainUpdate. Optimization to skip walking inert ActionSets.
	CK_DEFINE_ECS_TAG(FTag_Goap_ActionSet_RequiresChainUpdate);

// ====================================================================================================================
// PARAMS — alias to the BlueprintType data shape
// ====================================================================================================================

	using FFragment_Goap_ActionSet_Params = FCk_Fragment_Goap_ActionSetParamsData;

// ====================================================================================================================
// CURRENT FRAGMENT — Runtime ActionSet state (enable toggle, diagnostics)
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_ActionSet_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_ActionSet_Current);

		friend class ::UCk_Utils_Goap_ActionSet_UE;
		friend class FProcessor_Goap_ActionSet_Setup;
		friend class FProcessor_Goap_ActionSet_ChainUpdate;

	private:
		ECk_EnableDisable _EnableToggle = ECk_EnableDisable::Enable;
		TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
	};

// ====================================================================================================================
// ACTIVE CHAIN — Ordered chain of currently-active actions. [0] is the root.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_ActionSet_ActiveChain
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_ActionSet_ActiveChain);

		friend class ::UCk_Utils_Goap_ActionSet_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_ActionSet_ChainUpdate;

	private:
		TArray<FCk_Handle_Goap_Action> _Chain;

	public:
		CK_PROPERTY_GET(_Chain);
	};

// ====================================================================================================================
// ACTION CATALOG INDEX — O(1) tag-to-action lookup. Populated at AddAction
// time; read by ChainUpdate when resolving Plan[0]'s action tag.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_ActionSet_ActionCatalogIndex
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_ActionSet_ActionCatalogIndex);

		friend class ::UCk_Utils_Goap_ActionSet_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_ActionSet_ChainUpdate;

	private:
		TMap<FGameplayTag, FCk_Handle_Goap_Action> _TagToAction;

	public:
		CK_PROPERTY_GET(_TagToAction);
	};

// ====================================================================================================================
// WORLD STATE SOURCE — ActionSet-level default WS source. Used by the unified
// ChainUpdate logic when an Action does not provide its own override.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_ActionSet_WorldStateSource
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_ActionSet_WorldStateSource);

		friend class ::UCk_Utils_Goap_ActionSet_UE;
		friend class FProcessor_Goap_ActionSet_ChainUpdate;

	private:
		FCk_Handle_Goap_WorldState _WorldStateSource;

	public:
		CK_PROPERTY_GET(_WorldStateSource);
		CK_PROPERTY_SET(_WorldStateSource);
	};

// ====================================================================================================================
// SIGNALS — ActionSet-scoped
// ====================================================================================================================

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_ActionSet_ActiveChainChanged,
		FCk_Delegate_Goap_OnActiveChainChanged,
		FCk_Handle_Goap_ActionSet,
		FCk_Goap_Payload_OnActiveChainChanged);

// ====================================================================================================================

} // namespace ck
