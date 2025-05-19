#include "CkAnimPlan_Fragment.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include <Net/Core/PushModel/PushModel.h>
#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_AnimPlan_Rep::
    Broadcast_AddOrUpdate(
        const FCk_AnimPlan_State& InAnimPlanState)
    -> void
{
    const auto Found = _AnimPlansToReplicate.FindByPredicate([&](const FCk_AnimPlan_State& InElement)
    {
        return InElement.Get_AnimGoal() == InAnimPlanState.Get_AnimGoal();
    });

    if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
    {
        _AnimPlansToReplicate.Emplace(InAnimPlanState);
    }
    else
    {
        *Found = InAnimPlanState;
    }

    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AnimPlansToReplicate, this);
}

auto
    UCk_Fragment_AnimPlan_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AnimPlansToReplicate, Params);
}

auto
    UCk_Fragment_AnimPlan_Rep::
    PostLink()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_AnimPlan_Rep::
    Request_TryUpdateReplicatedAnimPlans()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_AnimPlan_Rep::
    OnRep_Updated()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    { return; }

    for (auto Index = _AnimPlansToReplicate_Previous.Num(); Index < _AnimPlansToReplicate.Num(); ++Index)
    {
        const auto& AnimPlanToReplicate = _AnimPlansToReplicate[Index];

        if (const auto& AnimPlanEntity = UCk_Utils_AnimPlan_UE::TryGet_AnimPlan(Get_AssociatedEntity(), AnimPlanToReplicate.Get_AnimGoal());
            ck::Is_NOT_Valid(AnimPlanEntity))
        {
            ck::animation::Verbose(TEXT("Could NOT find AnimPlan [{}]. AnimPlan replication PENDING..."),
                AnimPlanToReplicate.Get_AnimGoal());

            return;
        }
    }

    for (auto Index = 0; Index < _AnimPlansToReplicate.Num(); ++Index)
    {
        const auto& AnimPlanToReplicate = _AnimPlansToReplicate[Index];
        auto AnimPlanEntity = UCk_Utils_AnimPlan_UE::TryGet_AnimPlan(Get_AssociatedEntity(), AnimPlanToReplicate.Get_AnimGoal());

        if (NOT _AnimPlansToReplicate_Previous.IsValidIndex(Index))
        {
            ck::animation::Verbose(TEXT("Replicating AnimPlan for the FIRST time to [{}]"), AnimPlanToReplicate);

            UCk_Utils_AnimPlan_UE::Request_UpdateAnimState(AnimPlanEntity, FCk_Request_AnimPlan_UpdateAnimState{AnimPlanToReplicate.Get_AnimCluster(), AnimPlanToReplicate.Get_AnimState()});

            continue;
        }

        if (_AnimPlansToReplicate_Previous[Index] != AnimPlanToReplicate)
        {
            ck::animation::Verbose(TEXT("Replicating AnimPlan and UPDATING it to [{}]"), AnimPlanToReplicate);

            UCk_Utils_AnimPlan_UE::Request_UpdateAnimState(AnimPlanEntity, FCk_Request_AnimPlan_UpdateAnimState{AnimPlanToReplicate.Get_AnimCluster(), AnimPlanToReplicate.Get_AnimState()});

            continue;
        }

        ck::animation::Verbose(TEXT("IGNORING AnimPlan [{}] as there is no change between [{}] and [{}]"),
            AnimPlanToReplicate.Get_AnimGoal(),
            _AnimPlansToReplicate_Previous[Index],
            AnimPlanToReplicate);
    }

    _AnimPlansToReplicate_Previous = _AnimPlansToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
