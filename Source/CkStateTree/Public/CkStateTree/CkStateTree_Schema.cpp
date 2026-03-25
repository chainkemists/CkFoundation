#include "CkStateTree/CkStateTree_Schema.h"

#include <StateTreeTaskBase.h>
#include <StateTreeConditionBase.h>
#include <StateTreeConsiderationBase.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_StateTree_Schema_Entity::UCk_StateTree_Schema_Entity()
{
	// Initialize context data descriptors
	// This defines what context data is available to tasks/conditions
	auto& EntityContextDesc = _ContextDataDescs.AddDefaulted_GetRef();
	EntityContextDesc.Struct = FCk_StateTree_EntityContext::StaticStruct();
	EntityContextDesc.Name = FName(TEXT("EntityContext"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Schema_Entity::
	IsStructAllowed(
		const UScriptStruct* InScriptStruct) const
	-> bool
{
	// Allow all StateTree tasks, conditions, and considerations
	return InScriptStruct != nullptr
		&& (InScriptStruct->IsChildOf(FStateTreeTaskCommonBase::StaticStruct())
			|| InScriptStruct->IsChildOf(FStateTreeConditionCommonBase::StaticStruct())
			|| InScriptStruct->IsChildOf(FStateTreeConsiderationCommonBase::StaticStruct()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Schema_Entity::
	IsClassAllowed(
		const UClass* InClass) const
	-> bool
{
	// Allow any UObject-based evaluators
	return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Schema_Entity::
	IsExternalItemAllowed(
		const UStruct& InStruct) const
	-> bool
{
	// Allow world subsystems and other common external data
	// This enables tasks to access gameplay systems via GetExternalData
	return InStruct.IsChildOf(USubsystem::StaticClass())
		|| InStruct.IsChildOf(UWorld::StaticClass());
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_StateTree_Schema_Entity::
	GetContextDataDescs() const
	-> TConstArrayView<FStateTreeExternalDataDesc>
{
	return _ContextDataDescs;
}

// --------------------------------------------------------------------------------------------------------------------
