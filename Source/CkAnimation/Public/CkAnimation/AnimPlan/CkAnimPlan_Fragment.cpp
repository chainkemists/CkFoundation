#include "CkAnimPlan_Fragment.h"

#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for AnimPlan

static struct FAnimPlanRepHandlerRegistrar
{
    FAnimPlanRepHandlerRegistrar()
    {
        // All-or-nothing: NotReady until every targeted AnimPlan is composed, then diff-apply.
        const auto DoApplyAnimPlans = [](FCk_Handle& Entity, const TArray<FCk_AnimPlan_State>& NewPlans, const TArray<FCk_AnimPlan_State>& OldPlans) -> ECk_RepFragment_ApplyResult
        {
            for (auto Index = 0; Index < NewPlans.Num(); ++Index)
            {
                const auto& AnimPlanToReplicate = NewPlans[Index];

                if (const auto& AnimPlanEntity = UCk_Utils_AnimPlan_UE::TryGet_AnimPlan(Entity, AnimPlanToReplicate.Get_AnimGoal());
                    ck::Is_NOT_Valid(AnimPlanEntity))
                { return ECk_RepFragment_ApplyResult::NotReady; }
            }

            for (auto Index = 0; Index < NewPlans.Num(); ++Index)
            {
                const auto& AnimPlanToReplicate = NewPlans[Index];
                auto AnimPlanEntity = UCk_Utils_AnimPlan_UE::TryGet_AnimPlan(Entity, AnimPlanToReplicate.Get_AnimGoal());

                if (NOT OldPlans.IsValidIndex(Index) || OldPlans[Index] != AnimPlanToReplicate)
                {
                    UCk_Utils_AnimPlan_UE::Request_UpdateAnimState(AnimPlanEntity, FCk_Request_AnimPlan_UpdateAnimState{AnimPlanToReplicate.Get_AnimCluster(), AnimPlanToReplicate.Get_AnimState()});
                }
            }

            return ECk_RepFragment_ApplyResult::Applied;
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_AnimPlans::StaticStruct(); },
            {
                .Apply = [DoApplyAnimPlans](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    return DoApplyAnimPlans(Entity,
                        New.Get<FCk_RepData_AnimPlans>().AnimPlans,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_AnimPlans>().AnimPlans
                            : TArray<FCk_AnimPlan_State>{});
                }
            });
    }
} GAnimPlanRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
