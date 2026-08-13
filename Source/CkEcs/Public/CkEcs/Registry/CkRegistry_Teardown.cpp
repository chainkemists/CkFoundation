#include "CkRegistry_Teardown.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_registry_teardown
{
    // Named namespace rather than an anonymous one / bare file-local static: unity builds
    // concatenate TUs and would collide a plain GDepth with a sibling CkEcs .cpp.
    static int32 GDepth = 0;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_teardown
{
    auto Get_IsInProgress() -> bool
    {
        return ck_registry_teardown::GDepth > 0;
    }

    // --------------------------------------------------------------------------------------------------------------------

    FScopedGuard::
    FScopedGuard()
    {
        // A plain int32 is only sound because registry destruction is game-thread, matching
        // registry_table::Free's own affinity. Asserted rather than left to a comment: this project
        // has had work migrate onto a worker thread unannounced, and an off-thread teardown would
        // race the POOLS, not merely this counter — so the ensure is the useful signal, and an
        // atomic here would only hide it.
        CK_ENSURE_IF_NOT(IsInGameThread(),
            TEXT("Registry teardown started off the game thread. Fragment destruction races the "
                 "registry's own pools; the teardown guard cannot make that safe."))
        {}

        ++ck_registry_teardown::GDepth;
    }

    FScopedGuard::
    ~FScopedGuard()
    {
        --ck_registry_teardown::GDepth;
    }
}

// --------------------------------------------------------------------------------------------------------------------
