#include "CkJolt_Module.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/World/CkJoltWorld.h"
#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkLoadConvergence_Registry.h"

namespace ck_jolt_module
{
    const auto k_ConvergenceRow_BodiesRegistered   = FName{TEXT("Jolt.BodiesRegistered")};
    const auto k_ConvergenceRow_StepsSinceHold     = FName{TEXT("Jolt.StepsSinceHold")};
    const auto k_ConvergenceRow_ContactQueueDrained = FName{TEXT("Jolt.ContactQueueDrained")};
    const auto k_ConvergenceAdvance_GrantSteps     = FName{TEXT("Jolt.GrantFixedSteps")};

    auto
        TryResolve_JoltWorldFor(
            const FCk_Registry& InRegistry)
        -> ck::FJoltWorld*
    {
        const auto TransientEntity = FCk_Handle{InRegistry.Get_TransientEntity(), InRegistry.Get_RegistryHandle()};
        return ck::jolt::TryResolve_JoltWorld(TransientEntity);
    }
}

void FCkJoltModule::StartupModule()
{
    // Physics facts a load waits on, registered by the module that OWNS them so neither CkEcs nor the loader
    // needs to know Jolt exists. Every one answers NotApplicable without a Jolt world: a world that has no
    // physics has not failed to converge, the question simply does not arise there.
    ck::FCk_LoadConvergenceRegistry::Register(ck_jolt_module::k_ConvergenceRow_BodiesRegistered,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            if (ck_jolt_module::TryResolve_JoltWorldFor(InRegistry) == nullptr)
            { return ck::ECk_LoadConvergence::NotApplicable; }

            return InRegistry.Has_AnyLiveEntityWith<ck::FTag_JoltBody_NeedsSetup>()
                ? ck::ECk_LoadConvergence::Pending
                : ck::ECk_LoadConvergence::Satisfied;
        });

    ck::FCk_LoadConvergenceRegistry::Register(ck_jolt_module::k_ConvergenceRow_StepsSinceHold,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            const auto* JoltWorld = ck_jolt_module::TryResolve_JoltWorldFor(InRegistry);
            if (JoltWorld == nullptr)
            { return ck::ECk_LoadConvergence::NotApplicable; }

            const auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return ck::ECk_LoadConvergence::Pending; }

            const auto StepsSinceHold = JoltWorld->Get_GrantedStepsExecutedTotal() - Convergence->_PhysicsGrantedStepsBaseline;

            return StepsSinceHold >= ck::jolt::kConvergePhysicsSteps
                ? ck::ECk_LoadConvergence::Satisfied
                : ck::ECk_LoadConvergence::Pending;
        });

    ck::FCk_LoadConvergenceRegistry::Register(ck_jolt_module::k_ConvergenceRow_ContactQueueDrained,
        [](const FCk_Registry& InRegistry) -> ck::ECk_LoadConvergence
        {
            const auto* JoltWorld = ck_jolt_module::TryResolve_JoltWorldFor(InRegistry);
            if (JoltWorld == nullptr)
            { return ck::ECk_LoadConvergence::NotApplicable; }

            // A drain must have RUN — an untouched counter says nothing about the queue — and the most recent one
            // must have carried nothing. The "has anything contacted at all" question is deliberately NOT asked
            // here: a world with a Jolt world and no bodies never contacts, and waiting for one would make every
            // such load burn the convergence budget and report a loss it never had. What keeps this from reading
            // "settled" before physics has moved is Jolt.StepsSinceHold, which it is AND-ed with.
            const auto HasDrained = JoltWorld->Get_NumContactDrains() > 0;

            return HasDrained && JoltWorld->Get_LastDrainedContactEventCount() == 0
                ? ck::ECk_LoadConvergence::Satisfied
                : ck::ECk_LoadConvergence::Pending;
        });

    // The DRIVE half. A load freezes game time, so the frame delta plans zero steps and every predicate above
    // would wait on bodies nothing is moving. Registered as an advance rather than folded into a predicate
    // because a predicate that stepped the world it is measuring would be certifying its own side effects.
    ck::FCk_LoadConvergenceRegistry::Register_Advance(ck_jolt_module::k_ConvergenceAdvance_GrantSteps,
        [](FCk_Registry& InRegistry) -> void
        {
            auto* JoltWorld = ck_jolt_module::TryResolve_JoltWorldFor(InRegistry);
            if (JoltWorld == nullptr)
            { return; }

            auto* Convergence = InRegistry.TryGetContext<ck::FCtx_LoadConvergence>();
            if (Convergence == nullptr)
            { return; }

            // Stamped by the module that owns the counter, on the phase's first frame, so Jolt.StepsSinceHold can
            // ask "how much since the hold" from a cumulative counter without either side keeping load state.
            if (Convergence->_FramesConverging == 0)
            { Convergence->_PhysicsGrantedStepsBaseline = JoltWorld->Get_GrantedStepsExecutedTotal(); }

            // SELF-LIMITING, and that is load-bearing rather than an optimisation. Every granted step dirties the
            // pose markers a later processor consumes, so a grant issued every frame keeps the scheduler
            // permanently un-quiescent — and Ecs.SchedulerQuiescent would then never be satisfied, making every
            // load burn the convergence budget on a phase that was doing exactly what it was told.
            const auto StepsSinceHold = JoltWorld->Get_GrantedStepsExecutedTotal() - Convergence->_PhysicsGrantedStepsBaseline;

            // Republished for the driver's diagnostics only — it is the one number that says how far physics got,
            // and the driver cannot ask CkJolt for it directly.
            Convergence->_PhysicsGrantedStepsSinceHold = StepsSinceHold;

            if (StepsSinceHold >= ck::jolt::kConvergePhysicsSteps)
            { return; }

            JoltWorld->Request_GrantFixedSteps(ck::jolt::kConvergePhysicsSteps - StepsSinceHold);
        });
}

void FCkJoltModule::ShutdownModule()
{
    ck::FCk_LoadConvergenceRegistry::Unregister_Advance(ck_jolt_module::k_ConvergenceAdvance_GrantSteps);
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_jolt_module::k_ConvergenceRow_ContactQueueDrained);
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_jolt_module::k_ConvergenceRow_StepsSinceHold);
    ck::FCk_LoadConvergenceRegistry::Unregister(ck_jolt_module::k_ConvergenceRow_BodiesRegistered);
}

IMPLEMENT_MODULE(FCkJoltModule, CkJolt)
