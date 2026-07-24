#pragma once

#include "CkAggro/CkAggro_Fragment_Data.h"
#include "CkAggro/CkAggroTarget_Fragment.h"

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Aggro_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Bridge aliases — each owner param piece is copied onto the owner as its own fragment so processors declare RO
    // access on exactly what they read.

    using FFragment_Aggro_DefaultTargetParams = FCk_Fragment_AggroTarget_ParamsData;
    using FFragment_Aggro_SelectionParams     = FCk_Aggro_SelectionParams;
    using FFragment_Aggro_CapParams           = FCk_Aggro_CapParams;
    using FFragment_Aggro_EvaluationParams    = FCk_Aggro_EvaluationParams;

    // ----------------------------------------------------------------------------------------------------------------
    // Tags.

    CK_DEFINE_ECS_TAG(FTag_Aggro);
    CK_DEFINE_ECS_TAG(FTag_Aggro_NeedsSetup);
    // COUNTED — N Disable calls need N Enable calls before the pipeline resumes.
    CK_DEFINE_ECS_TAG_COUNTED(FTag_Aggro_Disabled);
    CK_DEFINE_ECS_TAG(FTag_Aggro_SelectionPending);

    // ----------------------------------------------------------------------------------------------------------------
    // The current active target + the timestamps hysteresis reads.

    struct CKAGGRO_API FFragment_Aggro_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Aggro_Current);

    public:
        friend class FProcessor_Aggro_SelectActiveTarget;
        friend class FProcessor_Aggro_HandleRequests;
        friend class FProcessor_AggroTarget_Forget;
        friend class FProcessor_AggroTarget_EndPlay;

    private:
        FCk_Handle_AggroTarget _ActiveTarget;
        FCk_Time               _ActiveTargetStartTime;
        FCk_Time               _LastSwitchTime;

    public:
        CK_PROPERTY_GET(_ActiveTarget);
        CK_PROPERTY_GET(_ActiveTargetStartTime);
        CK_PROPERTY_GET(_LastSwitchTime);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // The one always-ticking clock — the evaluation pacer. _DebugEvaluationCount feeds the IdleCost autotest.

    struct CKAGGRO_API FFragment_Aggro_EvaluationClock
    {
    public:
        CK_GENERATED_BODY(FFragment_Aggro_EvaluationClock);

    public:
        friend class FProcessor_Aggro_Setup;
        friend class FProcessor_Aggro_EvaluationPacer;

    private:
        FCk_Chrono _EvaluationChrono;
        int64      _DebugEvaluationCount = 0;

    public:
        CK_PROPERTY_GET(_EvaluationChrono);
        CK_PROPERTY_GET(_DebugEvaluationCount);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // O(1) tracked-entity -> AggroTarget dedupe/lookup. NEVER used for hot math (see the record for enumeration).

    struct CKAGGRO_API FFragment_Aggro_TargetMap
    {
    public:
        CK_GENERATED_BODY(FFragment_Aggro_TargetMap);

    public:
        friend class UCk_Utils_Aggro_UE;
        friend class FProcessor_Aggro_HandleRequests;
        friend class FProcessor_AggroTarget_Forget;
        friend class FProcessor_AggroTarget_EndPlay;

    private:
        TMap<FCk_Handle, FCk_Handle_AggroTarget> _TargetsByTrackedEntity;

    public:
        CK_PROPERTY_GET(_TargetsByTrackedEntity);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKAGGRO_API FFragment_Aggro_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Aggro_Requests);

    public:
        friend class FProcessor_Aggro_HandleRequests;
        friend class UCk_Utils_Aggro_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_Aggro_AddThreat,
            FCk_Request_Aggro_RemoveTarget,
            FCk_Request_Aggro_ClearAllTargets,
            FCk_Request_Aggro_SetActiveTarget,
            FCk_Request_Aggro_ClearActiveTarget>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnAggroTargetAcquired, FCk_Delegate_Aggro_OnTargetAcquired, FCk_Handle_Aggro, FCk_Handle_AggroTarget);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnAggroActiveTargetChanged, FCk_Delegate_Aggro_OnActiveTargetChanged, FCk_Handle_Aggro, FCk_Handle_AggroTarget, FCk_Handle_AggroTarget);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnAggroTargetForgotten, FCk_Delegate_Aggro_OnTargetForgotten, FCk_Handle_Aggro, FCk_Aggro_TargetForgottenInfo);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Aggro_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
