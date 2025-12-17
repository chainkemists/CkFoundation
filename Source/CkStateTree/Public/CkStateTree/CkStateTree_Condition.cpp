// Copyright Saad Taame. All Rights Reserved.

#include "CkStateTree/CkStateTree_Condition.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkStateTree_Condition)

auto
	UCk_StateTree_Condition::
	TestCondition(
		FStateTreeExecutionContext& Context) const
	-> bool
{
	auto Result = Super::TestCondition(Context);

	if (Invert)
	{
		Result = !Result;
	}

	return Result;
}
