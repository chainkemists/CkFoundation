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
            // Probes bake their params into a Jolt body at Add, so a fragment write cannot re-apply them:
            // the live-read subset routes through Request_Reconfigure instead, and a retune of the baked
            // fields is rejected loudly by the request handler rather than silently dropped.
            // ScrubPolicy stays Auto (= OnCommit) deliberately: the request is cheap, but it drains a
            // frame later and its rejection path ENSUREs, so a drag across a baked field would spray
            // errors. Revisit if a numeric live-read field is ever added to these params.
            FCk_LiveTuneHandlerRegistry::Register<FCk_Fragment_Probe_ParamsData>({
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
