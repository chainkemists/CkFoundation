#pragma once

#include "CkEcs/Tag/CkTag.h"

#include <Math/Transform.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Snapshot writes the expected saved pose before quarantine. Transform owns consumption: only the processor that
    // actually publishes this exact pose may turn it into the one-frame active rebase marker below.
    struct FFragment_Transform_PendingRestoreRebase
    {
    private:
        FTransform _ExpectedWorldTransform = FTransform::Identity;

    public:
        FFragment_Transform_PendingRestoreRebase() = default;

        explicit FFragment_Transform_PendingRestoreRebase(const FTransform& InExpectedWorldTransform)
            : _ExpectedWorldTransform(InExpectedWorldTransform)
        {
        }

        const FTransform& Get_ExpectedWorldTransform() const
        { return _ExpectedWorldTransform; }
    };

    // One-frame provenance for a world-transform change applied while rebuilding saved state. SceneNode propagation
    // copies it to derived children so systems with genuinely static runtime representations can rebase those
    // representations once without weakening their ordinary "must not move" contract.
    CK_DEFINE_ECS_TAG(FTag_Transform_RestoreRebase);
}

// --------------------------------------------------------------------------------------------------------------------
