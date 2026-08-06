#include "CkAutoReorient_Fragment.h"

#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.inl.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_auto_reorient_fragment
{
    struct FRegistrar
    {
        FRegistrar()
        {
            // Add stores the params and nothing else — no derived state, no NeedsSetup tag — and
            // FProcessor_AutoReorient_OrientTowardsVelocity reads them every tick, so the fragment write
            // IS the whole re-apply. Every field retunes.
            // Wrapper-form fragment (holds the params rather than aliasing them), so the fragment type is
            // named explicitly — otherwise the re-apply would write a component the entity does not carry.
            FCk_LiveTuneHandlerRegistry::Register<
                FCk_Fragment_AutoReorient_ParamsData, ck::FFragment_AutoReorient_Params>();
        }
    };

    const FRegistrar GCkAutoReorientLiveTuneRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
