#pragma once

#include "CkGoap/Bundle/CkGoap_Bundle_Fragment_Data.h"
#include "CkGoap/Tier/CkGoap_Tier_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapDiagnostic_DependencyCycle

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Bundle_UE;
class UCk_Utils_Goap_Tier_UE;

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_Bundle_Setup;
	class FProcessor_Goap_Bundle_ChainUpdate;

// ====================================================================================================================
// TAGS
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_Bundle_RequiresSetup);

	// Set whenever any tier in the bundle completes a plan. Consumed +
	// removed by ChainUpdate. Optimization to skip walking inert bundles.
	CK_DEFINE_ECS_TAG(FTag_Goap_Bundle_RequiresChainUpdate);

// ====================================================================================================================
// PARAMS — alias to the BlueprintType data shape
// ====================================================================================================================

	using FFragment_Goap_Bundle_Params = FCk_Fragment_Goap_BundleParamsData;

// ====================================================================================================================
// CURRENT FRAGMENT — Runtime bundle state (enable toggle, diagnostics)
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Bundle_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Bundle_Current);

		friend class ::UCk_Utils_Goap_Bundle_UE;
		friend class FProcessor_Goap_Bundle_Setup;
		friend class FProcessor_Goap_Bundle_ChainUpdate;

	private:
		ECk_EnableDisable _EnableToggle = ECk_EnableDisable::Enable;
		TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
	};

// ====================================================================================================================
// ACTIVE TIERS — Ordered chain of currently-active tiers. [0] is the root.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Bundle_ActiveTiers
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Bundle_ActiveTiers);

		friend class ::UCk_Utils_Goap_Bundle_UE;
		friend class ::UCk_Utils_Goap_Tier_UE;
		friend class FProcessor_Goap_Bundle_ChainUpdate;

	private:
		TArray<FCk_Handle_Goap_Tier> _Tiers;

	public:
		CK_PROPERTY_GET(_Tiers);
	};

// ====================================================================================================================
// TIER CATALOG INDEX — O(1) tag-to-tier lookup. Populated at AddTier time;
// read by ChainUpdate when resolving Plan[0]'s _ActionTag.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Bundle_TierCatalogIndex
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Bundle_TierCatalogIndex);

		friend class ::UCk_Utils_Goap_Bundle_UE;
		friend class ::UCk_Utils_Goap_Tier_UE;
		friend class FProcessor_Goap_Bundle_ChainUpdate;

	private:
		TMap<FGameplayTag, FCk_Handle_Goap_Tier> _TagToTier;

	public:
		CK_PROPERTY_GET(_TagToTier);
	};

// ====================================================================================================================
// SIGNALS — Bundle-scoped
// ====================================================================================================================

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Bundle_ActiveTiersChanged,
		FCk_Delegate_Goap_OnActiveTiersChanged,
		FCk_Handle_Goap_Bundle,
		FCk_Goap_Payload_OnActiveTiersChanged);

// ====================================================================================================================

} // namespace ck
