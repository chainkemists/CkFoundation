#include "CkStateTree/CkStateTree_Owner.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <StateTreeExecutionContext.h>

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Owner::
	Create(
		UWorld* InWorld,
		FCk_Handle_StateTree InOwnerEntity)
	-> UCk_StateTree_Owner*
{
	if (NOT IsValid(InWorld))
	{
		return nullptr;
	}

	auto Owner = NewObject<UCk_StateTree_Owner>(InWorld);
	Owner->_OwnerEntity = InOwnerEntity;
	Owner->UpdateCachedContext();
	
	return Owner;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Owner::
	SetContextRequirements(
		FStateTreeExecutionContext& InContext) const
	-> bool
{
	// Set the entity context as external data so tasks can access it
	// The data view needs both the struct type and pointer to memory
	return InContext.SetContextDataByName(
		TEXT("EntityContext"),
		FStateTreeDataView(
			FCk_StateTree_EntityContext::StaticStruct(),
			const_cast<FCk_StateTree_EntityContext*>(&_CachedContext)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Owner::
	UpdateCachedContext()
	-> void
{
	_CachedContext.Entity = _OwnerEntity;
	_CachedContext.World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_OwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------
