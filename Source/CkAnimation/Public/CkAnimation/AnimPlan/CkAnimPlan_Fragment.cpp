#include "CkAnimPlan_Fragment.h"

#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment + Get_LifetimeOwner (RegisterLazyTyped + SeedContainer)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

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

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_AnimPlans>(
            {
                .Apply = [DoApplyAnimPlans](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    return DoApplyAnimPlans(Entity,
                        New.Get<FCk_RepData_AnimPlans>().AnimPlans,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_AnimPlans>().AnimPlans
                            : TArray<FCk_AnimPlan_State>{});
                },
                // Owner-resident empty-seed-and-defer (mirrors the deleted FProcessor_AnimPlan_ReplicateOnRestore):
                // emit a SET-but-empty payload so the re-drive seeds the OWNER's container; the real per-owner
                // payload is rebuilt by FProcessor_AnimPlan_Replicate via the re-arm below. UNSET only when this
                // entity is not an AnimPlan.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_AnimPlan_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_AnimPlans{});
                },
                // Custom seed: the container lives on the LifetimeOwner; the typed add self-gates on the owner's
                // driver (NotAdded => retry). Re-arm the ongoing trigger on the AnimPlan entity itself.
                .SeedContainer = [](FCk_Handle& Entity, const FInstancedStruct& Data) -> ECk_AddedOrNot
                {
                    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Entity);
                    if (ck::Is_NOT_Valid(LifetimeOwner))
                    { return ECk_AddedOrNot::NotAdded; }

                    const auto AddedOrNot = UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_AnimPlans>(
                        LifetimeOwner, Data.Get<FCk_RepData_AnimPlans>());
                    if (AddedOrNot == ECk_AddedOrNot::NotAdded)
                    { return AddedOrNot; }

                    Entity.AddOrGet<ck::FTag_AnimPlan_MayRequireReplication>();
                    return AddedOrNot;
                },
                .Transport = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GAnimPlanRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
