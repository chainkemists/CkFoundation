#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkSnapshot_UnresolvedHusk_Signals.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Snapshot_OnUnresolvedHuskReaped,
    FCk_Handle, InHusk,
    FString, InArchetypePath);

// --------------------------------------------------------------------------------------------------------------------

// UHT anchor so the file-scope dynamic delegate above registers and the .generated.h is valid (the signal
// structs below are plain C++, not reflected). Carries no data; never instantiated.
USTRUCT()
struct FCk_Snapshot_UnresolvedHuskSignalsAnchor
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // WORLD-scoped: broadcast on the world's transient entity, the same rendezvous CkSnapshot's own save/load
    // signals use. It cannot be per-entity - the husk is destroyed in the same act that fires this, so a
    // consumer binding on the husk would have to already hold the handle it is being told about.
    //
    // The payload names the archetype path the husk's recipe carried, empty when nothing carried one. A
    // consumer wanting the load's OWN husks reads the load report instead: those are attributed there, and
    // this signal fires for the routes the report cannot see.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        Snapshot_OnUnresolvedHuskReaped,
        FCk_Delegate_Snapshot_OnUnresolvedHuskReaped,
        FCk_Handle,
        FString);
}

// --------------------------------------------------------------------------------------------------------------------
