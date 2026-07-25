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
    // Has/Cast key for a dialog emitter.
    CK_DEFINE_ECS_TAG(FTag_DialogEmitter);

    // On-demand marker: present iff this emitter has at least one cooldown recorded. It is what gives the
    // cooldown ticker a view — the Cooldowns fragment itself is always present, so keying off that would tick
    // every emitter in the world every frame just to find the few that are actually cooling.
    CK_DEFINE_ECS_TAG(FTag_DialogEmitter_HasCooldowns);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_DialogEmitter_Params = FCk_Fragment_DialogEmitter_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Sentinel expiry for a "Forever" (play-once-ever) cooldown: an expiry no world-time ever reaches, so the
    // Now < Expiry active test always holds.
    struct CKDIALOG_API FDialog_CooldownSentinels
    {
        static auto Forever() -> const FCk_Time&;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Always present on an emitter (added by Add). Line ENTITY handle -> the cooldown record (expiry, the duration it
    // was started with, and the mode); Forever cooldowns store FDialog_CooldownSentinels::Forever() as the expiry.
    // The started-with duration is retained so an observer can express progress, which expiry alone cannot give.
    // Keyed by handle (not LineID) by design: a re-registered bank makes new entities, so cooldowns do not survive
    // re-registration — the accepted consequence of handle keying.
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

    // On-demand marker fragment: present iff the emitter has queries waiting to be evaluated. Added by HandleRequests
    // when a Query request arrives; drained and removed by EvaluateQueries. Its presence IS the EvaluateQueries view
    // filter + dirty marker (this replaces the old FTag_DialogEmitter_HasPendingQuery). Carries the readiness-defer
    // bookkeeping so a query waiting on a not-yet-ready registry retries across ticks and times out loudly.
    struct CKDIALOG_API FFragment_DialogEmitter_PendingQueries
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_PendingQueries);

    public:
        friend class FProcessor_DialogEmitter_HandleRequests;
        friend class FProcessor_DialogEmitter_EvaluateQueries;

    private:
        TArray<FCk_Request_DialogEmitter_Query> _Queries;

        // One-shot "registry not ready" Display log guard (avoids per-tick spam while a query waits on the registry).
        bool _LoggedNotReadyOnce = false;

        // World time at which the FIRST still-pending query was enqueued; used to fire the readiness-timeout ensure.
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

    // One retained query result for the graph-debugger live overlay + history list.
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

    // Ring buffer of the last N query results, written by the EvaluateQueries processor. Only present once the
    // emitter has completed at least one query. Cap comes from UCk_Utils_Dialog_Settings_UE::Get_DebugHistorySize.
    struct CKDIALOG_API FFragment_DialogEmitter_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_DialogEmitter_Debug);

    public:
        friend class FProcessor_DialogEmitter_EvaluateQueries;
        friend class ::UCk_Utils_DialogEmitter_UE;

    private:
        // Newest-last ring of retained query results.
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
