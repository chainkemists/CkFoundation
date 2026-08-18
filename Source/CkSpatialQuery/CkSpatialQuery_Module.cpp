#include "CkSpatialQuery_Module.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkLoadConvergence_Registry.h"

#define LOCTEXT_NAMESPACE "FCkSpatialQueryModule"

namespace ck_spatialquery_module
{
    const auto k_ConvergenceRow_ProbesRegistered       = FName{TEXT("SpatialQuery.ProbesRegistered")};
    const auto k_ConvergenceRow_OverlapRequestsDrained = FName{TEXT("SpatialQuery.OverlapRequestsDrained")};

    // Probes are Jolt-backed volumes, so an absent Jolt world is the same "does not arise here" the physics rows
    // answer with — and it is the one form of that question a predicate can ask, since it is handed a registry and
    // nothing else.
    auto
        Get_HasProbeBackend(
            const FCk_Registry& InRegistry)
        -> bool
    {
        const auto TransientEntity = FCk_Handle{InRegistry.Get_TransientEntity(), InRegistry.Get_RegistryHandle()};
        return ck::jolt::TryResolve_JoltWorld(TransientEntity) != nullptr;
    }
}

void FCkSpatialQueryModule::StartupModule()
{
    // Overlap facts a load waits on, registered by the module that owns them. A restored world whose probes have
    // not registered — or whose overlap requests are still queued — is one where a trigger the player is standing
    // in reports nobody inside it, which is exactly the shape the convergence phase exists to close.
    ck::FCk_LoadConvergenceRegistry::Register(ck_spatialquery_module::k_ConvergenceRow_ProbesRegistered,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            if (NOT ck_spatialquery_module::Get_HasProbeBackend(InRegistry))
            { return ck::ECk_LoadConvergence::NotApplicable; }

            return InRegistry.Has_AnyLiveEntityWith<ck::FTag_Probe_NeedsSetup>()
                ? ck::ECk_LoadConvergence::Pending
                : ck::ECk_LoadConvergence::Satisfied;
        });

    ck::FCk_LoadConvergenceRegistry::Register(ck_spatialquery_module::k_ConvergenceRow_OverlapRequestsDrained,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            if (NOT ck_spatialquery_module::Get_HasProbeBackend(InRegistry))
            { return ck::ECk_LoadConvergence::NotApplicable; }

            // The FRAGMENT is persistent — it is the queue, not a marker — so presence proves nothing and only
            // emptiness does.
            auto AnyQueued = false;
            InRegistry.View<ck::FFragment_Probe_Requests>().ForEach(
            [&AnyQueued](FCk_Entity, const ck::FFragment_Probe_Requests& InRequests)
            {
                if (NOT InRequests.Get_Requests().IsEmpty())
                { AnyQueued = true; }
            });

            return AnyQueued
                ? ck::ECk_LoadConvergence::Pending
                : ck::ECk_LoadConvergence::Satisfied;
        });
}

void FCkSpatialQueryModule::ShutdownModule()
{
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_spatialquery_module::k_ConvergenceRow_OverlapRequestsDrained);
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_spatialquery_module::k_ConvergenceRow_ProbesRegistered);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkSpatialQueryModule, CkSpatialQuery)
