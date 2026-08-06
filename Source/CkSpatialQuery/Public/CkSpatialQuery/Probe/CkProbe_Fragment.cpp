#include "CkProbe_Fragment.h"

#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.inl.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_probe_fragment
{
    struct FRegistrar
    {
        FRegistrar()
        {
            // Probes own an in-place reconfigure path (the ViaRequest shape): the live-read params
            // subset re-applies through Request_Reconfigure; a retune of the baked fields is rejected
            // loudly by the request handler rather than silently dropped.
            FCk_LiveTuneHandlerRegistry::Register_ViaRequest<FCk_Fragment_Probe_ParamsData>({
                .Apply = [](FCk_Handle& InProbeEntity, const FCk_Fragment_Probe_ParamsData& InFreshParams) -> void
                {
                    auto Probe = UCk_Utils_Probe_UE::CastChecked(InProbeEntity);
                    UCk_Utils_Probe_UE::Request_Reconfigure(Probe, FCk_Request_Probe_Reconfigure{InFreshParams}, {});
                },
            });
        }
    };

    const FRegistrar GCkProbeLiveTuneRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
