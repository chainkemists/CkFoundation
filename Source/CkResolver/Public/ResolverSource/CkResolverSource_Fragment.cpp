#include "CkResolverSource_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.inl.h"

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKRESOLVER_API, resolver_source, ck::FFragment_ResolverSource_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_resolver_source_fragment
{
    struct FRegistrar
    {
        FRegistrar()
        {
            // Add stores the params only — no derived state, no NeedsSetup tag — and
            // FProcessor_ResolverSource_HandleRequests reads them live. Every field retunes.
            // Wrapper-form fragment — name it explicitly (see the registry header's T_Fragment note).
            FCk_LiveTuneHandlerRegistry::Register<
                FCk_Fragment_ResolverSource_ParamsData, ck::FFragment_ResolverSource_Params>();
        }
    };

    const FRegistrar GCkResolverSourceLiveTuneRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
