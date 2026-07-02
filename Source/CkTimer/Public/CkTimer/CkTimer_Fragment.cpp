#include "CkTimer_Fragment.h"

#include "CkCore/Time/CkTime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "Net/UnrealNetwork.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
// The CK_REGISTER_SNAPSHOTABLE save/load closures call entt's get<T>(Archive&), so the Archive writer/reader types
// must be COMPLETE here (not just forward-declared as in the registry header).
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Requests);

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registration. ck:: types hoisted to a file-scope alias because CK_REGISTER_SNAPSHOTABLE token-pastes the
// type name (the `::` cannot be pasted). FFragment_Timer_Current (Tier C) carries the countdown progress; the Params
// (Tier A USTRUCT) carry the config. The run/pause + count-direction TAGS already round-trip free (CK_DEFINE_ECS_TAG
// auto-registers them), and FFragment_Timer_Requests is the per-frame queue — intentionally NOT registered.

using FSnap_Timer_Current = ck::FFragment_Timer_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_Timer_Current);
CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_Timer_ParamsData); // Tier A USTRUCT (no `::` → no alias hoist needed)

// The owner-side record of timer children. Must round-trip with the timers themselves — a transient record left
// restored timers as orphaned, still-ticking entities invisible to their owner.
using FSnap_RecordOfTimers = ck::FFragment_RecordOfTimers;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfTimers);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_Timer_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    // No handle fields → InCtx unused. Delegate to the chrono's whole-value serialize (goal + progress).
    _Chrono.SerializeSnapshot(InAr);
}

// --------------------------------------------------------------------------------------------------------------------
