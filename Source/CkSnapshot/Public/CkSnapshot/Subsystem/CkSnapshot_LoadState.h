#pragma once

// The one fact a client needs from a server's load: has THIS load reached ready-to-resume. Request_Load is
// authority-only and the loader lives on the initiating GameInstance, so a client has no load, no report and no
// completion of its own — but on a listen-server reload it travels and rebuilds alongside the server, and its
// world must be held for exactly as long as the server's is.
//
// It rides a replicated container on an ActorRelay channel entity because that is the framework's net-correlated
// carrier: a replicated fragment reaches a client only on an entity the replication driver knows about.

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_LoadState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The wire form. The epoch is what makes the fact answerable rather than merely true: a client that travelled for
// load N must not release on a fact left standing by load N-1.
USTRUCT()
struct CKSNAPSHOT_API FCk_RepData_SnapshotLoadState
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_SnapshotLoadState);

    UPROPERTY()
    int32 LoadEpoch = 0;

    UPROPERTY()
    bool ReadyToResume = false;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The APPLIED form, written by the handler on the client and by the loader on the server, so both sides read
    // the fact the same way. Session by construction: it describes a load that is happening, not a world that was
    // saved.
    struct CKSNAPSHOT_API FFragment_Snapshot_LoadState
    {
        CK_GENERATED_BODY(FFragment_Snapshot_LoadState);

    private:
        int32 _LoadEpoch = 0;
        bool  _ReadyToResume = false;

    public:
        CK_PROPERTY(_LoadEpoch);
        CK_PROPERTY(_ReadyToResume);
    };
}

// --------------------------------------------------------------------------------------------------------------------
