#include "CkAnimPlan_Fragment.h"

#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment + Get_LifetimeOwner (container-based replication for this feature)
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for AnimPlan

static struct FAnimPlanRepHandlerRegistrar
{
    FAnimPlanRepHandlerRegistrar()
    {
        // All-or-nothing: NotReady until every targeted AnimPlan is composed, then diff-apply.
        const auto DoApplyAnimPlans = [](FCk_Handle& Entity, const TArray<FCk_AnimPlan_State>& NewPlans, const TArray<FCk_AnimPlan_State>& OldPlans) -> ECk_Persistence_ApplyResult
        {
            for (auto Index = 0; Index < NewPlans.Num(); ++Index)
            {
                const auto& AnimPlanToReplicate = NewPlans[Index];

                if (const auto& AnimPlanEntity = UCk_Utils_AnimPlan_UE::TryGet_AnimPlan(Entity, AnimPlanToReplicate.Get_AnimGoal());
                    ck::Is_NOT_Valid(AnimPlanEntity))
                { return ECk_Persistence_ApplyResult::NotReady; }
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

            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_AnimPlans>(
                // v3 save capture (Phase 4B) — VALUE-EMITTING, keyed per-AnimPlan-entity (Produce fires on the plan
                // entity). Emit this plan's goal/cluster/state, byte-identical to the wire-builder
                // FProcessor_AnimPlan_Replicate (CkAnimPlan_Processor.cpp). UNSET when this entity is not an AnimPlan.
                // On load the hydration branch above writes it AUTHORITY-side; the re-armed Replicate pass re-publishes.
                [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_AnimPlan_Current>())
                    { return {}; }

                    const auto& Params  = Entity.Get<ck::FFragment_AnimPlan_Params>().Get_Params();
                    const auto& Current = Entity.Get<ck::FFragment_AnimPlan_Current>();

                    auto Data = FCk_RepData_AnimPlans{};
                    Data.AnimPlans.Emplace(FCk_AnimPlan_State{
                        Params.Get_AnimGoal(), Current.Get_AnimCluster(), Current.Get_AnimState()});
                    return FInstancedStruct::Make(MoveTemp(Data));
                },
                [DoApplyAnimPlans](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    return DoApplyAnimPlans(Entity,
                        New.Get<FCk_RepData_AnimPlans>().AnimPlans,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_AnimPlans>().AnimPlans
                            : TArray<FCk_AnimPlan_State>{});
                },
                // Save-load hydration (authority-side, Phase 4B): the v3 payload is CHILD-keyed (per-plan Produce),
                // so under hydration Entity IS the AnimPlan entity — write its saved state directly. The OWNER-keyed
                // DoApplyAnimPlans (net Apply) never resolves it.
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT Entity.Has<ck::FFragment_AnimPlan_Current>())
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    auto Plan = UCk_Utils_AnimPlan_UE::Cast(Entity);
                    for (const auto& Saved : New.Get<FCk_RepData_AnimPlans>().AnimPlans)
                    {
                        UCk_Utils_AnimPlan_UE::Request_UpdateAnimState(Plan,
                            FCk_Request_AnimPlan_UpdateAnimState{Saved.Get_AnimCluster(), Saved.Get_AnimState()});
                    }
                    return ECk_Persistence_ApplyResult::Applied;
                });
    }
} GAnimPlanRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
