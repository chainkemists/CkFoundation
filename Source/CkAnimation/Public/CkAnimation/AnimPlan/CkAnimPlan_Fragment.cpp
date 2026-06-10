#include "CkAnimPlan_Fragment.h"

#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name).
// The owner's RecordOfAnimPlans is registered too so restored owners re-link their anim-plan children.

using FSnap_AnimPlan_Params  = ck::FFragment_AnimPlan_Params;
using FSnap_AnimPlan_Current = ck::FFragment_AnimPlan_Current;
using FSnap_RecordOfAnimPlans = ck::FFragment_RecordOfAnimPlans;

CK_REGISTER_SNAPSHOTABLE(FSnap_AnimPlan_Params);
CK_REGISTER_SNAPSHOTABLE(FSnap_AnimPlan_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfAnimPlans);

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
