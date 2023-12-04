#pragma once

#include "CkAnimState_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkAnimState_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AnimState_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AnimState_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimState_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimState_Current);

    public:
        friend class FProcessor_AnimState_HandleRequests;
        friend class UCk_Utils_AnimState_UE;

    public:
        FFragment_AnimState_Current() = default;
        explicit FFragment_AnimState_Current(FCk_AnimState_Current InCurrent);

    private:
        FCk_AnimState_Goal _AnimGoal;
        FCk_AnimState_State _AnimState;
        FCk_AnimState_Cluster _AnimCluster;
        FCk_AnimState_Overlay _AnimOverlay;

        ECk_AnimStateComponents _ComponentsModified = ECk_AnimStateComponents::None;

    public:
        CK_PROPERTY_GET(_AnimGoal);
        CK_PROPERTY_GET(_AnimState);
        CK_PROPERTY_GET(_AnimCluster);
        CK_PROPERTY_GET(_AnimOverlay);
        CK_PROPERTY(_ComponentsModified);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimState_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimState_Requests);

    public:
        friend class FProcessor_AnimState_HandleRequests;
        friend class UCk_Utils_AnimState_UE;

    public:
        using SetGoalRequestType = FCk_Request_AnimState_SetGoal;
        using SetGoalRequests = TOptional<SetGoalRequestType>;

        using SetStateRequestType = FCk_Request_AnimState_SetState;
        using SetStateRequests = TOptional<SetStateRequestType>;

        using SetClusterRequestType = FCk_Request_AnimState_SetCluster;
        using SetClusterRequests = TOptional<SetClusterRequestType>;

        using SetOverlayRequestType = FCk_Request_AnimState_SetOverlay;
        using SetOverlayRequests = TOptional<SetOverlayRequestType>;

    private:
        SetGoalRequests _SetGoalRequest;
        SetStateRequests _SetStateRequest;
        SetClusterRequests _SetClusterRequest;
        SetOverlayRequests _SetOverlayRequest;

    public:
        CK_PROPERTY_GET(_SetGoalRequest);
        CK_PROPERTY_GET(_SetStateRequest);
        CK_PROPERTY_GET(_SetClusterRequest);
        CK_PROPERTY_GET(_SetOverlayRequest);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKANIMATION_API, AnimState_OnGoalChanged, FCk_Delegate_AnimState_OnGoalChanged_MC, FCk_Handle, FCk_Payload_AnimState_OnGoalChanged);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKANIMATION_API, AnimState_OnStateChanged, FCk_Delegate_AnimState_OnStateChanged_MC, FCk_Handle, FCk_Payload_AnimState_OnStateChanged);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKANIMATION_API, AnimState_OnClusterChanged, FCk_Delegate_AnimState_OnClusterChanged_MC, FCk_Handle, FCk_Payload_AnimState_OnClusterChanged);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKANIMATION_API, AnimState_OnOverlayChanged, FCk_Delegate_AnimState_OnOverlayChanged_MC, FCk_Handle, FCk_Payload_AnimState_OnOverlayChanged);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_AnimState_Replicate; }

UCLASS(Blueprintable)
class CKANIMATION_API UCk_Fragment_AnimState_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_AnimState_Rep);

public:
    friend class ck::FProcessor_AnimState_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_AnimGoal();

    UFUNCTION()
    void OnRep_AnimState();

    UFUNCTION()
    void OnRep_AnimCluster();

    UFUNCTION()
    void OnRep_AnimOverlay();

private:
    UPROPERTY(ReplicatedUsing = OnRep_AnimGoal)
    FCk_AnimState_Goal _AnimGoal;

    UPROPERTY(ReplicatedUsing = OnRep_AnimState)
    FCk_AnimState_State _AnimState;

    UPROPERTY(ReplicatedUsing = OnRep_AnimCluster)
    FCk_AnimState_Cluster _AnimCluster;

    UPROPERTY(ReplicatedUsing = OnRep_AnimOverlay)
    FCk_AnimState_Overlay _AnimOverlay;
};

// --------------------------------------------------------------------------------------------------------------------
