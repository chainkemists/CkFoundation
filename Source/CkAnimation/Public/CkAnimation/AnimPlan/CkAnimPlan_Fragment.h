#pragma once

#include "CkAnimPlan_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <Serialization/Archive.h>

#include "CkAnimPlan_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AnimPlan_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FSnapshotContext; }

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AnimPlan_MayRequireReplication);

    // Per-feature restore-replication done marker — see FTag_TagSet_RestoreReplicated for the pattern.
    CK_DEFINE_ECS_TAG(FTag_AnimPlan_RestoreReplicated);

    // --------------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        // Tags persist by NAME (stable across runs — net tag indices are not). A saved tag missing
        // from the load-time registry loads as an invalid tag instead of faulting the restore.
        inline auto SerializeSnapshot_AnimPlanTagByName(FArchive& InAr, FGameplayTag& InOutTag) -> void
        {
            auto TagName = InOutTag.GetTagName();
            InAr << TagName;

            if (InAr.IsLoading())
            {
                constexpr auto ErrorIfNotFound = false;
                InOutTag = TagName.IsNone()
                    ? FGameplayTag{}
                    : FGameplayTag::RequestGameplayTag(TagName, ErrorIfNotFound);
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_AnimPlan_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimPlan_Params);

    public:
        using ParamsType = FCk_Fragment_AnimPlan_ParamsData;
        using IsSnapshotable = void;

        // Tier-C: hand-rolled — the wrapped reflected struct exposes its fields only through
        // getters + the generated constructor, so round-trip through locals.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto Goal    = _Params.Get_AnimGoal();
            auto Cluster = _Params.Get_StartingAnimCluster();
            auto State   = _Params.Get_StartingAnimState();

            detail::SerializeSnapshot_AnimPlanTagByName(InAr, Goal);
            detail::SerializeSnapshot_AnimPlanTagByName(InAr, Cluster);
            detail::SerializeSnapshot_AnimPlanTagByName(InAr, State);

            if (InAr.IsLoading())
            {
                _Params = ParamsType{Goal}.Set_StartingAnimCluster(Cluster).Set_StartingAnimState(State);
            }
        }

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

    public:
        using IsSnapshotable = void;

        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            detail::SerializeSnapshot_AnimPlanTagByName(InAr, _AnimCluster);
            detail::SerializeSnapshot_AnimPlanTagByName(InAr, _AnimState);
        }

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
        FCk_Delegate_AnimPlan_OnPlanChanged,
        FCk_Handle_AnimPlan,
        FCk_AnimPlan_State,
        FCk_AnimPlan_State);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAnimPlans, FCk_Handle_AnimPlan);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_AnimPlan_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKANIMATION_API FCk_RepData_AnimPlans
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_AnimPlans);

    UPROPERTY()
    TArray<FCk_AnimPlan_State> AnimPlans;
};

namespace ck
{
    using FFragment_ContainerRef_AnimPlans = TFragment_ContainerEntryRef<FCk_RepData_AnimPlans>;
}

// --------------------------------------------------------------------------------------------------------------------
