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
    const auto k_ConvergenceAdvance_Stability       = FName{TEXT("Ecs.TrackStability")};

    auto Get_DestroyCarrierCount(const FCk_Registry& InRegistry) -> int32
    {
        auto Count = 0;
        InRegistry.View<ck::FTag_DestroyEntity_Initiate>().ForEach(
        [&Count](FCk_Entity)
        {
            ++Count;
        });
        return Count;
    }
}

auto FCkEcsModule::StartupModule() -> void
{
    // The stability half of both rows below, written ONCE per frame by the module that owns the destroy tag. An
    // advance rather than a predicate because it has to REMEMBER: "unchanged since last frame" is not a question
    // answerable from a single read, and a predicate that kept state would stop being pure.
    //
    // Runs before the loader's pump, so the pump count it compares is the previous frame's — which is exactly the
    // pair that answers "would waiting another frame change anything".
    ck::FCk_LoadConvergenceRegistry::Register_Advance(ck_ecs_module::k_ConvergenceAdvance_Stability,
        [](FCk_Registry& InRegistry) -> void
        {
            auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return; }

            if (Convergence->_FramesConverging > 0)
            {
                Convergence->_PumpCountStableFrames =
                    Convergence->_PumpCountLastFrame == Convergence->_PumpCountPrevFrame
                        ? Convergence->_PumpCountStableFrames + 1
                        : 0;
            }
            Convergence->_PumpCountPrevFrame = Convergence->_PumpCountLastFrame;

            const auto Carriers = ck_ecs_module::Get_DestroyCarrierCount(InRegistry);
            if (Convergence->_FramesConverging > 0)
            {
                Convergence->_DestroyCarriersStableFrames =
                    Carriers == Convergence->_DestroyCarriersLastFrame
                        ? Convergence->_DestroyCarriersStableFrames + 1
                        : 0;
            }
            Convergence->_DestroyCarriersLastFrame = Carriers;
        });

    // The scheduler has STOPPED CHANGING, read PURELY from what the driver recorded. The pump that produces the
    // number is a deliberate once-per-frame action elsewhere; pumping from in here would double the phase's work,
    // reorder it against whatever else the phase is driving, and report on a world it had just changed.
    //
    // Silence is the easy case and still counts. But a content world never goes silent: state machines
    // re-evaluate, probe and ISM-proxy request queues refill, and a processor with a custom DoTick reports a pass
    // as work having visited no entities at all. Waiting for zero there waits forever. What the hold needs is
    // "another frame would change nothing", so a pump count that has held steady for kLoad_ConvergenceStableFrames
    // is converged too. A skipped tick group still blocks outright: it contributes zero passes of its own and so
    // reads identically to a quiet one, which is the one way this could answer Satisfied about a world it never
    // measured.
    ck::FCk_LoadConvergenceRegistry::Register(ck_ecs_module::k_ConvergenceRow_SchedulerQuiescent,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            const auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return ck::ECk_LoadConvergence::Pending; }

            // Frames first: before anything has been driven, a zero pump count means "nothing has run", not
            // "nothing is left to run".
            const auto HasBeenDriven = Convergence->_FramesConverging > 0;
            const auto NothingSkipped = Convergence->_PumpSkippedGroupsLastFrame == 0;
            const auto NothingToPump = Convergence->_PumpCountLastFrame == 0;
            const auto HoldingSteady = Convergence->_PumpCountStableFrames >= ck::kLoad_ConvergenceStableFrames;

            return HasBeenDriven && NothingSkipped && (NothingToPump || HoldingSteady)
                ? ck::ECk_LoadConvergence::Satisfied
                : ck::ECk_LoadConvergence::Pending;
        });

    // The destruction pipeline spans three ticks by design, so an entity whose destroy was queued during the
    // payload drain is still structurally present for frames afterwards. Waiting on it explicitly is what stops a
    // restored world being handed back while a stray the reconcile pass condemned is still in it.
    //
    // An EMPTY queue is the clean answer. But a live world's queue is never empty for long — NPC AI creates and
    // destroys immediate-query entities every frame — so emptiness alone would make this unreachable on exactly
    // the worlds it matters for. A carrier count that has held steady for kLoad_ConvergenceStableFrames says the
    // queue is draining as fast as it fills, which is the same statement about the world: another frame changes
    // nothing. A queue still GROWING moves the count and keeps this Pending.
    ck::FCk_LoadConvergenceRegistry::Register(ck_ecs_module::k_ConvergenceRow_DestroyQueueDrained,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            if (NOT InRegistry.Has_AnyLiveEntityWith<ck::FTag_DestroyEntity_Initiate>())
            { return ck::ECk_LoadConvergence::Satisfied; }

            const auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return ck::ECk_LoadConvergence::Pending; }

            return Convergence->_DestroyCarriersStableFrames >= ck::kLoad_ConvergenceStableFrames
                ? ck::ECk_LoadConvergence::Satisfied
                : ck::ECk_LoadConvergence::Pending;
        });

    return IModuleInterface::StartupModule();
}

auto FCkEcsModule::ShutdownModule() -> void
{
    ck::FCk_LoadConvergenceRegistry::Unregister_Advance(ck_ecs_module::k_ConvergenceAdvance_Stability);
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
