#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkDialog/Emitter/CkDialogEmitter_Fragment_Data.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_DialogEmitter_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_DialogEmitter);

    // Present iff this emitter has at least one cooldown recorded — it is what gives the cooldown ticker a view.
    // The Cooldowns fragment is always present, so keying the ticker off that would visit every emitter every frame.
    CK_DEFINE_ECS_TAG(FTag_DialogEmitter_HasCooldowns);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_DialogEmitter_Params = FCk_Fragment_DialogEmitter_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Always present on an emitter (added by Add). Keyed by line ENTITY handle rather than LineID by design: a
    // re-registered bank makes new entities, so cooldowns deliberately do not survive re-registration.
    struct CKDIALOG_API FFragment_DialogEmitter_Cooldowns
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_Cooldowns);

    public:
        friend class FProcessor_DialogEmitter_HandleRequests;
        friend class FProcessor_DialogEmitter_EvaluateQueries;
        friend class FProcessor_DialogEmitter_TickCooldowns;
        friend class ::UCk_Utils_DialogEmitter_UE;

    private:
        TMap<FCk_Handle_DialogLine, FCk_DialogEmitter_CooldownEntry> _Cooldowns;

    public:
        CK_PROPERTY_GET(_Cooldowns);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Present iff the emitter has queries waiting: added by HandleRequests, drained and removed by EvaluateQueries.
    // Its presence IS the EvaluateQueries view filter + dirty marker.
    struct CKDIALOG_API FFragment_DialogEmitter_PendingQueries
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_PendingQueries);

    public:
        friend class FProcessor_DialogEmitter_HandleRequests;
        friend class FProcessor_DialogEmitter_EvaluateQueries;

    private:
        TArray<FCk_Request_DialogEmitter_Query> _Queries;

        bool _LoggedNotReadyOnce = false;

        FCk_Time _OldestPendingQueryTime;
        bool _HasOldestPendingQueryTime = false;

    public:
        CK_PROPERTY_GET(_Queries);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKDIALOG_API FFragment_DialogEmitter_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_Requests);

    public:
        friend class FProcessor_DialogEmitter_HandleRequests;
        friend class ::UCk_Utils_DialogEmitter_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_DialogEmitter_Query,
            FCk_Request_DialogEmitter_StartCooldown,
            FCk_Request_DialogEmitter_ClearCooldown,
            FCk_Request_DialogEmitter_ClearAllCooldowns>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKDIALOG_API FDialogEmitter_DebugEntry
    {
    public:
        CK_GENERATED_BODY(FDialogEmitter_DebugEntry);

    public:
        friend class FProcessor_DialogEmitter_EvaluateQueries;
        friend class ::UCk_Utils_DialogEmitter_UE;

    private:
        FCk_DialogEmitter_QueryResult _Result;
        FCk_Time _Timestamp;

    public:
        CK_PROPERTY_GET(_Result);
        CK_PROPERTY_GET(_Timestamp);
    };

    // Newest-last ring of retained query results for the graph debugger, written by the EvaluateQueries processor.
    // Cap comes from UCk_Utils_Dialog_Settings_UE::Get_DebugHistorySize.
    struct CKDIALOG_API FFragment_DialogEmitter_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_Debug);

    public:
        friend class FProcessor_DialogEmitter_EvaluateQueries;
        friend class ::UCk_Utils_DialogEmitter_UE;

    private:
        TArray<FDialogEmitter_DebugEntry> _History;

    public:
        CK_PROPERTY_GET(_History);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKDIALOG_API, OnDialogQueryCompleted,
        FCk_Delegate_DialogEmitter_OnQueryCompleted, FCk_Handle_DialogEmitter, FCk_DialogEmitter_QueryResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKDIALOG_API, OnDialogCooldownStarted,
        FCk_Delegate_DialogEmitter_OnCooldownStarted, FCk_Handle_DialogEmitter, FCk_Handle_DialogLine);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKDIALOG_API, OnDialogCooldownEnded,
        FCk_Delegate_DialogEmitter_OnCooldownEnded, FCk_Handle_DialogEmitter, FCk_Handle_DialogLine);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_DialogEmitter_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
