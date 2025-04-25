#include "CkAnimPlan_Utils.h"

#include "CkAnimation/CkAnimation_Log.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimPlan_UE::
    Add(
        FCk_Handle& InAnimPlanOwnerEntity,
        const FCk_Fragment_AnimPlan_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_AnimPlan
{
    auto NewAnimPlanEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_AnimPlan>(InAnimPlanOwnerEntity);

    UCk_Utils_GameplayLabel_UE::Add(NewAnimPlanEntity, InParams.Get_AnimGoal());

    NewAnimPlanEntity.Add<ck::FFragment_AnimPlan_Params>(InParams);
    auto& Current = NewAnimPlanEntity.Add<ck::FFragment_AnimPlan_Current>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::animation::VeryVerbose
        (
            TEXT("Skipping creation of AnimPlan Rep Fragment on Entity [{}] because it's set to [{}]"),
            NewAnimPlanEntity,
            InReplicates
        );
    }
    else
    {
        TryAddReplicatedFragment<UCk_Fragment_AnimPlan_Rep>(InAnimPlanOwnerEntity);
    }

    RecordOfAnimPlans_Utils::AddIfMissing(InAnimPlanOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfAnimPlans_Utils::Request_Connect(InAnimPlanOwnerEntity, NewAnimPlanEntity);

    if (ck::IsValid(InParams.Get_StartingAnimState()))
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_StartingAnimCluster()),
            TEXT("Adding new AnimPlan to Entity [{}] with a valid starting AnimState [{}] but an INVALID starting AnimCluster!"),
            InAnimPlanOwnerEntity,
            InParams.Get_StartingAnimState())
        { return NewAnimPlanEntity; }
    }

    Current._AnimCluster = InParams.Get_StartingAnimCluster();
    Current._AnimState = InParams.Get_StartingAnimState();

    return NewAnimPlanEntity;
}

auto
    UCk_Utils_AnimPlan_UE::
    AddMultiple(
        FCk_Handle& InAnimPlanOwnerEntity,
        const FCk_Fragment_MultipleAnimPlan_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_AnimPlan>
{
    return ck::algo::Transform<TArray<FCk_Handle_AnimPlan>>(InParams.Get_AnimPlanParams(),
    [&](const FCk_Fragment_AnimPlan_ParamsData& InAnimPlanParams)
    {
        return Add(InAnimPlanOwnerEntity, InAnimPlanParams, InReplicates);
    });
}

auto
    UCk_Utils_AnimPlan_UE::
    Has_Any(
        const FCk_Handle& InAnimPlanOwnerEntity)
    -> bool
{
    return RecordOfAnimPlans_Utils::Has(InAnimPlanOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AnimPlan_UE, FCk_Handle_AnimPlan, ck::FFragment_AnimPlan_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimPlan_UE::
    TryGet_AnimPlan(
        const FCk_Handle& InAnimPlanOwnerEntity,
        FGameplayTag InAnimPlanGoalName)
    -> FCk_Handle_AnimPlan
{
    return RecordOfAnimPlans_Utils::Get_ValidEntry_ByTag(InAnimPlanOwnerEntity, InAnimPlanGoalName);
}

auto
    UCk_Utils_AnimPlan_UE::
    ForEach_AnimPlan(
        FCk_Handle& InAnimPlanOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_AnimPlan>
{
    auto AnimPlan = TArray<FCk_Handle_AnimPlan>{};

    ForEach_AnimPlan(InAnimPlanOwnerEntity, [&](FCk_Handle_AnimPlan& InAnimPlan)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAnimPlan, InOptionalPayload); }
        else
        { AnimPlan.Emplace(InAnimPlan); }
    });

    return AnimPlan;
}

auto
    UCk_Utils_AnimPlan_UE::
    ForEach_AnimPlan(
        FCk_Handle& InAnimPlanOwnerEntity,
        const TFunction<void(FCk_Handle_AnimPlan&)>& InFunc)
    -> void
{
    RecordOfAnimPlans_Utils::ForEach_ValidEntry
    (
        InAnimPlanOwnerEntity,
        [&](FCk_Handle_AnimPlan InAnimPlan)
        {
            InFunc(InAnimPlan);
        }
    );
}

auto
    UCk_Utils_AnimPlan_UE::
    Get_AnimGoal(
        const FCk_Handle_AnimPlan& InAnimPlanEntity)
    -> FCk_AnimPlan_Goal
{
    return FCk_AnimPlan_Goal{InAnimPlanEntity.Get<ck::FFragment_AnimPlan_Params>().Get_Params().Get_AnimGoal()};
}

auto
    UCk_Utils_AnimPlan_UE::
    Get_AnimCluster(
        const FCk_Handle_AnimPlan& InAnimPlanEntity)
    -> FCk_AnimPlan_Cluster
{
    const auto& Params = InAnimPlanEntity.Get<ck::FFragment_AnimPlan_Params>().Get_Params();
    const auto& Current = InAnimPlanEntity.Get<ck::FFragment_AnimPlan_Current>();

    return FCk_AnimPlan_Cluster{Params.Get_AnimGoal(), Current.Get_AnimCluster()};
}

auto
    UCk_Utils_AnimPlan_UE::
    Get_AnimState(
        const FCk_Handle_AnimPlan& InAnimPlanEntity)
    -> FCk_AnimPlan_State
{
    const auto& Params = InAnimPlanEntity.Get<ck::FFragment_AnimPlan_Params>().Get_Params();
    const auto& Current = InAnimPlanEntity.Get<ck::FFragment_AnimPlan_Current>();

    return FCk_AnimPlan_State{Params.Get_AnimGoal(), Current.Get_AnimCluster(), Current.Get_AnimState()};
}

auto
    UCk_Utils_AnimPlan_UE::
    Request_UpdateAnimCluster(
        FCk_Handle_AnimPlan& InHandle,
        const FCk_Request_AnimPlan_UpdateAnimCluster& InRequest)
    -> FCk_Handle_AnimPlan
{
    InHandle.AddOrGet<ck::FFragment_AnimPlan_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_AnimPlan_UE::
    Request_UpdateAnimState(
        FCk_Handle_AnimPlan& InHandle,
        const FCk_Request_AnimPlan_UpdateAnimState& InRequest)
    -> FCk_Handle_AnimPlan
{
    InHandle.AddOrGet<ck::FFragment_AnimPlan_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_AnimPlan_UE::
    BindTo_OnGoalChanged(
        FCk_Handle_AnimPlan& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AnimPlan_OnPlanChanged& InDelegate)
    -> FCk_Handle_AnimPlan
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_AnimPlan_OnPlanChanged, InHandle, InDelegate, InBehavior, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_AnimPlan_UE::
    UnbindFrom_OnGoalChanged(
        FCk_Handle_AnimPlan& InHandle,
        const FCk_Delegate_AnimPlan_OnPlanChanged& InDelegate)
    -> FCk_Handle_AnimPlan
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_AnimPlan_OnPlanChanged, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_AnimPlan_UE::
    Request_TryReplicateAnimPlan(
        FCk_Handle_AnimPlan& InAnimPlanEntity)
    -> void
{
    InAnimPlanEntity.AddOrGet<ck::FTag_AnimPlan_MayRequireReplication>();
}

// --------------------------------------------------------------------------------------------------------------------
