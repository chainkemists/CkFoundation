#include "CkEcs_Module.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkLoadConvergence_Registry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkEcsModule"

namespace ck_ecs_module
{
    const auto k_ConvergenceRow_SchedulerQuiescent = FName{TEXT("Ecs.SchedulerQuiescent")};
    const auto k_ConvergenceRow_DestroyQueueDrained = FName{TEXT("Ecs.DestroyQueueDrained")};
}

auto FCkEcsModule::StartupModule() -> void
{
    // The scheduler's own quiescence, read PURELY from what the driver recorded. The pump that produces the
    // number is a deliberate once-per-frame action elsewhere; pumping from in here would double the phase's work,
    // reorder it against whatever else the phase is driving, and report on a world it had just changed.
    ck::FCk_LoadConvergenceRegistry::Register(ck_ecs_module::k_ConvergenceRow_SchedulerQuiescent,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            const auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return ck::ECk_LoadConvergence::Pending; }

            // Frames first: before anything has been driven, a zero pump count means "nothing has run", not
            // "nothing is left to run". And a SKIPPED tick group contributes zero passes of its own, so it reads
            // identically to a quiet one — which is why it can never be allowed to answer Satisfied.
            const auto HasBeenDriven = Convergence->_FramesConverging > 0;
            const auto NothingSkipped = Convergence->_PumpSkippedGroupsLastFrame == 0;
            const auto NothingToPump = Convergence->_PumpCountLastFrame == 0;

            return HasBeenDriven && NothingSkipped && NothingToPump
                ? ck::ECk_LoadConvergence::Satisfied
                : ck::ECk_LoadConvergence::Pending;
        });

    // The destruction pipeline spans three ticks by design, so an entity whose destroy was queued during the
    // payload drain is still structurally present for frames afterwards. Waiting on it explicitly is what stops a
    // restored world being handed back while a stray the reconcile pass condemned is still in it.
    ck::FCk_LoadConvergenceRegistry::Register(ck_ecs_module::k_ConvergenceRow_DestroyQueueDrained,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            return InRegistry.Has_AnyLiveEntityWith<ck::FTag_DestroyEntity_Initiate>()
                ? ck::ECk_LoadConvergence::Pending
                : ck::ECk_LoadConvergence::Satisfied;
        });

    return IModuleInterface::StartupModule();
}

auto FCkEcsModule::ShutdownModule() -> void
{
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_ecs_module::k_ConvergenceRow_DestroyQueueDrained);
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_ecs_module::k_ConvergenceRow_SchedulerQuiescent);

    // Flip the slot table's "alive" sentinel BEFORE Super::ShutdownModule
    // returns. After this call, Free()/Resolve()/TryResolve() are safe
    // no-ops for any UObject destructors that fire later in the DLL
    // teardown sequence — the phoenix singleton's whole purpose.
    ck::registry_table::ShutdownTable();

    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkEcsModule, CkEcs);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
