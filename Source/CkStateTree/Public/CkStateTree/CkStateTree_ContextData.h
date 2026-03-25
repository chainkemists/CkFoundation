#pragma once

#include "CkStateTree/CkStateTree_Fragment_Data.h"

#include "CkStateTree_ContextData.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Context data provided to StateTree tasks/conditions when using UCk_StateTree_Schema_Entity.
 * This struct is passed as context data and allows tasks to access the executing entity.
 */
USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_StateTree_EntityContext
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_StateTree_EntityContext);

public:
	/** The entity executing this StateTree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StateTree")
	FCk_Handle_StateTree Entity;

	/** World the entity exists in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StateTree")
	TObjectPtr<UWorld> World = nullptr;

public:
	CK_DEFINE_CONSTRUCTORS(FCk_StateTree_EntityContext, Entity);
};

// --------------------------------------------------------------------------------------------------------------------
