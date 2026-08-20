#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"

#include "CkGoap/Action/CkGoap_Action_Fragment.h"  // FFragment_Goap_Action_Params

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FFragment_Goap_Planner_PlanState::
		Get_PlanClasses() const -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
	{
		auto Result = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};
		Result.Reserve(_Plan.Num());
		for (const auto& ActionHandle : _Plan)
		{
			if (ck::Is_NOT_Valid(ActionHandle))
			{ continue; }
			const auto& Params = ActionHandle.template Get<FFragment_Goap_Action_Params>();
			Result.Add(Params.Get_ActionClass());
		}
		return Result;
	}

	auto
		FFragment_Goap_Planner_PlanState::
		Get_FirstPlanClass() const -> TSubclassOf<UCk_GoapAction_EntityScript>
	{
		for (const auto& ActionHandle : _Plan)
		{
			if (ck::Is_NOT_Valid(ActionHandle))
			{ continue; }

			return ActionHandle.template Get<FFragment_Goap_Action_Params>().Get_ActionClass();
		}

		return {};
	}
}
