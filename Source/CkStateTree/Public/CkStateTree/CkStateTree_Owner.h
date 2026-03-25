#pragma once

#include "CkStateTree/CkStateTree_ContextData.h"
#include "CkStateTree/CkStateTree_Fragment_Data.h"

#include "CkStateTree_Owner.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FStateTreeExecutionContext;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Minimal UObject that serves as the owner for StateTree execution.
 * Required because FStateTreeExecutionContext needs a UObject owner for:
 * - Creating transient UObjects during execution
 * - Logging context
 * 
 * Created with World as outer (no Actor dependency).
 * Lifetime is tied to the owning entity.
 */
UCLASS()
class CKSTATETREE_API UCk_StateTree_Owner : public UObject
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_StateTree_Owner);

public:
	/** The entity this owner is bound to */
	UPROPERTY()
	FCk_Handle_StateTree _OwnerEntity;

	/** Cached context data for efficient access during tick */
	UPROPERTY()
	FCk_StateTree_EntityContext _CachedContext;

public:
	/**
	 * Factory method to create a new StateTree owner.
	 * @param InWorld - World to use as outer (required, cannot be null)
	 * @param InOwnerEntity - Entity handle this owner is bound to
	 * @return New owner instance, or nullptr if InWorld is null
	 */
	static auto
	Create(
		UWorld* InWorld,
		FCk_Handle_StateTree InOwnerEntity) -> UCk_StateTree_Owner*;

public:
	/**
	 * Sets up the context requirements on an execution context.
	 * Must be called before StateTree execution to provide entity context.
	 * @param InContext - Execution context to configure
	 * @return true if setup succeeded
	 */
	auto
	SetContextRequirements(
		FStateTreeExecutionContext& InContext) const -> bool;

	/**
	 * Updates the cached context data.
	 * Called when the entity or world changes.
	 */
	auto
	UpdateCachedContext() -> void;

public:
	auto
	Get_OwnerEntity() const -> const FCk_Handle_StateTree& { return _OwnerEntity; }

	auto
	Get_CachedContext() const -> const FCk_StateTree_EntityContext& { return _CachedContext; }
};

// --------------------------------------------------------------------------------------------------------------------
