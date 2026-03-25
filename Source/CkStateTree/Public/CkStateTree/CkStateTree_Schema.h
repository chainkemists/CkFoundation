#pragma once

#include "CkStateTree/CkStateTree_ContextData.h"

#include <StateTreeSchema.h>
#include <StateTreeTypes.h>

#include "CkStateTree_Schema.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * StateTree schema for entity-based StateTrees.
 * This schema allows StateTrees to be executed on ECS entities without requiring an Actor.
 * 
 * Context data provided:
 * - FCk_StateTree_EntityContext: Contains the entity handle and world reference
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories,
	   Meta = (DisplayName = "CK Entity StateTree"))
class CKSTATETREE_API UCk_StateTree_Schema_Entity : public UStateTreeSchema
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_StateTree_Schema_Entity);

public:
	UCk_StateTree_Schema_Entity();

protected:
	//~ Begin UStateTreeSchema Interface
	virtual auto
	IsStructAllowed(
		const UScriptStruct* InScriptStruct) const -> bool override;

	virtual auto
	IsClassAllowed(
		const UClass* InClass) const -> bool override;

	virtual auto
	IsExternalItemAllowed(
		const UStruct& InStruct) const -> bool override;

	virtual auto
	GetContextDataDescs() const -> TConstArrayView<FStateTreeExternalDataDesc> override;
	//~ End UStateTreeSchema Interface

private:
	/** Cached context data descriptors */
	UPROPERTY()
	TArray<FStateTreeExternalDataDesc> _ContextDataDescs;
};

// --------------------------------------------------------------------------------------------------------------------
