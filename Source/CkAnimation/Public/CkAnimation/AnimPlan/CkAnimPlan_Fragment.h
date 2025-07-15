#pragma once

#include "CkAnimPlan_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Public/CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkAnimPlan_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AnimPlan_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AnimPlan_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimPlan_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimPlan_Params);

    public:
        using ParamsType = FCk_Fragment_AnimPlan_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AnimPlan_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimPlan_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimPlan_Current);

    public:
        friend class FProcessor_AnimPlan_HandleRequests;
        friend class UCk_Utils_AnimPlan_UE;

    private:
        FGameplayTag _AnimCluster;
        FGameplayTag _AnimState;

    public:
        CK_PROPERTY_GET(_AnimCluster);
        CK_PROPERTY_GET(_AnimState);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimPlan_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimPlan_Requests);

    public:
        friend class FProcessor_AnimPlan_HandleRequests;
        friend class UCk_Utils_AnimPlan_UE;

    public:
        using UpdateClusterRequestType = FCk_Request_AnimPlan_UpdateAnimCluster;
        using UpdateStateRequestType = FCk_Request_AnimPlan_UpdateAnimState;

        using RequestType = std::variant<UpdateStateRequestType, UpdateClusterRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKANIMATION_API,
        AnimPlan_OnPlanChanged,
        FCk_Delegate_AnimPlan_OnPlanChanged_MC,
        FCk_Handle_AnimPlan,
        FCk_AnimPlan_State,
        FCk_AnimPlan_State);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAnimPlans, FCk_Handle_AnimPlan);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_AnimPlan_Replicate; }

UCLASS(Blueprintable)
class CKANIMATION_API UCk_Fragment_AnimPlan_Rep : public UCk_Ecs_ReplicatedObject_UE
{
private:
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_AnimPlan_Rep);

public:
    auto
    Broadcast_AddOrUpdate(
        const FCk_AnimPlan_State& InAnimPlanState) -> void;

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

private:
    auto
    PostLink() -> void override;

public:
    auto
    Request_TryUpdateReplicatedAnimPlans() -> void;

private:
    UFUNCTION()
    void
    OnRep_Updated();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_AnimPlan_State> _AnimPlansToReplicate;
    TArray<FCk_AnimPlan_State> _AnimPlansToReplicate_Previous;
};

// --------------------------------------------------------------------------------------------------------------------
