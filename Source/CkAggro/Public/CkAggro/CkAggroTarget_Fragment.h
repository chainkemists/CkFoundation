#pragma once

#include "CkAggro/CkAggroTarget_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AggroTarget_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Bridge aliases — the target's reflected param pieces, viewed as runtime fragments by the processors.

    using FFragment_AggroTarget_ThreatParams   = FCk_AggroTarget_ThreatParams;
    using FFragment_AggroTarget_SpatialParams  = FCk_AggroTarget_SpatialParams;
    using FFragment_AggroTarget_ForgetParams   = FCk_AggroTarget_ForgetParams;
    using FFragment_AggroTarget_ScoreParams    = FCk_AggroTarget_ScoreParams;
    using FFragment_AggroTarget_LifetimeParams = FCk_AggroTarget_LifetimeParams;

    // ----------------------------------------------------------------------------------------------------------------
    // The entity this target represents (the thing being aggroed).

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(UAggroTarget_TrackedEntity_Utils, AggroTarget_TrackedEntity, FCk_Handle);

    // ----------------------------------------------------------------------------------------------------------------
    // Tags.

    CK_DEFINE_ECS_TAG(FTag_AggroTarget);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_NeedsSetup);
    // COUNTED — N independent senses each vote "perceived"; N MarkUnperceived (or one ResetPerception) clears it.
    CK_DEFINE_ECS_TAG_COUNTED(FTag_AggroTarget_Perceived);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_WithinRetention);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_IsActive);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_PendingForget);
    // Marks a target dropped by cap eviction, so the Forget processor reports ECk_Aggro_ForgetReason::Evicted.
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_Evicted);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_EvaluationPending);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_CannotBecomeActive);
    CK_DEFINE_ECS_TAG(FTag_AggroTarget_CannotBeForgotten);

    // ----------------------------------------------------------------------------------------------------------------
    // Write-once identity. _AggroOwner / _Instigator / _Source are set at Create; _CreationTime is stamped by Setup.

    struct CKAGGRO_API FFragment_AggroTarget_TargetInfo
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroTarget_TargetInfo);

    public:
        friend class FProcessor_AggroTarget_Setup;

    private:
        FCk_Handle _AggroOwner;
        FCk_Handle _Instigator;
        FCk_Handle _Source;
        FCk_Time   _CreationTime;

    public:
        CK_PROPERTY_GET(_AggroOwner);
        CK_PROPERTY_GET(_Instigator);
        CK_PROPERTY_GET(_Source);
        CK_PROPERTY_GET(_CreationTime);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AggroTarget_TargetInfo, _AggroOwner, _Instigator, _Source);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Analytic-decay threat state. _LastDecayTime is the anchor the decay integrates from.

    struct CKAGGRO_API FFragment_AggroTarget_Threat
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroTarget_Threat);

    public:
        friend class FProcessor_AggroTarget_Setup;
        friend class FProcessor_AggroTarget_HandleRequests;
        friend class FProcessor_AggroTarget_Evaluate;

    private:
        float    _Threat = 0.0f;
        FCk_Time _LastThreatTime;
        FCk_Time _LastDecayTime;

    public:
        CK_PROPERTY_GET(_Threat);
        CK_PROPERTY_GET(_LastThreatTime);
        CK_PROPERTY_GET(_LastDecayTime);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Perception state (memory only — never scored directly).

    struct CKAGGRO_API FFragment_AggroTarget_Perception
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroTarget_Perception);

    public:
        friend class FProcessor_AggroTarget_Setup;
        friend class FProcessor_AggroTarget_HandleRequests;

    private:
        FCk_Time _LastPerceivedTime;
        FVector  _LastKnownLocation = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_LastPerceivedTime);
        CK_PROPERTY_GET(_LastKnownLocation);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Evaluate output. Raw score (no incumbent bias baked in); linear distance in cm.

    struct CKAGGRO_API FFragment_AggroTarget_Score
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroTarget_Score);

    public:
        friend class FProcessor_AggroTarget_Evaluate;

    private:
        float _Score    = 0.0f;
        float _Distance = 0.0f;

    public:
        CK_PROPERTY_GET(_Score);
        CK_PROPERTY_GET(_Distance);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKAGGRO_API FFragment_AggroTarget_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroTarget_Requests);

    public:
        friend class FProcessor_AggroTarget_HandleRequests;
        friend class UCk_Utils_AggroTarget_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_AggroTarget_AddThreat,
            FCk_Request_AggroTarget_SetThreat,
            FCk_Request_AggroTarget_MarkPerceived,
            FCk_Request_AggroTarget_MarkUnperceived,
            FCk_Request_AggroTarget_ResetPerception,
            FCk_Request_AggroTarget_Forget>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // ----------------------------------------------------------------------------------------------------------------
    // Owner -> AggroTarget child record. Lives on the AggroTarget side (not CkAggro) so AggroTarget::Create can connect
    // a target to any owner's record without depending on the Aggro feature. Ownership/teardown/enumeration only.

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS_TRANSIENT(FUtils_RecordOfAggroTargets, FFragment_RecordOfAggroTargets, FCk_Handle_AggroTarget);

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnAggroThreatChanged, FCk_Delegate_AggroTarget_OnThreatChanged, FCk_Handle_AggroTarget, float, float);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_AggroTarget_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
