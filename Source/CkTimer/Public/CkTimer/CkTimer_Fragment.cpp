#include "CkTimer_Fragment.h"

#include "CkCore/Time/CkTime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "Net/UnrealNetwork.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_timer_fragment
{
    // v3 save/load persistence handler for Timer (spec Phase 3.2). Save-only (Produce + HydrationApply, NO net Apply):
    // timers are unreplicated, so FCk_SaveData_Timer never enters a replicated container and the authority-side load
    // hydration dispatcher (FProcessor_Hydration_Dispatch) is its sole applier.
    //
    // COVERAGE LIMITATION: only Construct-composed timers persist. A timer is a child entity created by
    // UCk_Utils_Timer_UE::Add; when the Add ran during the owner's Construct (recipe capture), the child gets a spawn
    // recipe and is rebuilt on load, so Produce/HydrationApply run for it. Timers added at RUNTIME (post-BeginPlay —
    // e.g. an SM task's WaitForNewTimer) are RuntimeSpawned-with-no-recipe under the v3 rebuild model, are NOT rebuilt
    // on load, and therefore do not persist. That is the rebuild-model contract, not a bug in this handler.
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_SaveData_Timer>(
            {
                // Load-path applier (authority-side). NotReady until the rebuilt entity has composed its Timer
                // fragments; then re-drive the runtime state through DEFERRED requests ONLY — the chrono's
                // _CurrentValue is friend-gated, so it is never written directly. Because the timer is unreplicated,
                // these requests re-arm nothing on any client; they are drained by FProcessor_Timer_HandleRequests on
                // the authority. The NotReady gate precedes every request.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    if (NOT UCk_Utils_Timer_UE::Has(Entity))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    // Gate until Setup has run. FProcessor_Timer_Setup Completes a CountDown chrono to GoalValue when it
                    // consumes FTag_Timer_NeedsSetup. Re-driving the position BEFORE Setup would let Setup mutate the
                    // chrono AFTER we computed the (relative) Jump delta, landing the timer at the wrong elapsed. Waiting
                    // for NeedsSetup to clear makes the current-elapsed baseline stable, and guarantees the enqueued
                    // Jump is not clobbered by a still-pending Setup (Setup runs at most once). NotReady is transient —
                    // Setup runs every authority tick.
                    if (Entity.Has<ck::FTag_Timer_NeedsSetup>())
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    const auto& Payload = New.Get<FCk_SaveData_Timer>();
                    auto TimerHandle = UCk_Utils_Timer_UE::Cast(Entity);

                    // 1. Restore the RUNTIME direction if it drifted from the rebuilt (Params-derived) direction.
                    //    Request_ChangeCountDirection flips the FTag_Timer_Countdown tag immediately (it mutates the
                    //    tag synchronously, it is not a queued request despite the Request_ name).
                    if (UCk_Utils_Timer_UE::Get_CountDirection(TimerHandle) != Payload.Get_CountDirection())
                    { UCk_Utils_Timer_UE::Request_ChangeCountDirection(TimerHandle, Payload.Get_CountDirection()); }

                    // 2. Reposition the chrono to the saved elapsed via an ABSOLUTE Jump. The Jump handler owns the
                    //    direction-dependent delta math (the single source of truth): in absolute mode JumpDuration is
                    //    the TARGET elapsed and the handler Ticks (CountUp) / Consumes (CountDown) by the gap against the
                    //    CURRENT elapsed. Setup has run (gated above), so that baseline is stable — post-Setup elapsed is
                    //    GoalValue for CountDown and 0 for CountUp, and nothing else mutates a Paused chrono before the
                    //    Jump drains. Jump broadcasts OnTimerJump/OnTimerUpdate — never OnTimerDone — so positioning a
                    //    terminal timer at GoalValue does not re-fire completion.
                    UCk_Utils_Timer_UE::Request_Jump(TimerHandle,
                        FCk_Request_Timer_Jump{Payload.Get_Elapsed()}.Set_JumpMode(ECk_RelativeAbsolute::Absolute));

                    // 3. Restore run-state. NEVER Request_Complete — it is the only request that re-broadcasts
                    //    OnTimerDone. A restored terminal timer is already positioned at GoalValue by the Jump above,
                    //    and its persisted run-state is Paused (PauseOnDone/StopOnDone remove NeedsUpdate on
                    //    completion), so the Update processors never run it; even in a pathological Running+done case
                    //    FProcessor_Timer_Update / _Countdown early-out at the top when the chrono is already
                    //    done/depleted — so OnTimerDone is not re-fired on load.
                    if (UCk_Utils_Timer_UE::Get_CurrentState(TimerHandle) != Payload.Get_RunState())
                    {
                        if (Payload.Get_RunState() == ECk_Timer_State::Running)
                        { UCk_Utils_Timer_UE::Request_Resume(TimerHandle); }
                        else
                        { UCk_Utils_Timer_UE::Request_Pause(TimerHandle); }
                    }

                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Save-capture: emit the chrono position + RUNTIME direction (tag) + run-state (tag). UNSET when the
                // Timer feature is absent on the entity (nothing to persist).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_Timer_UE::Has(Entity))
                    { return {}; }

                    auto TimerHandle = UCk_Utils_Timer_UE::Cast(Entity);

                    auto Payload = FCk_SaveData_Timer{};
                    Payload.Set_Elapsed(UCk_Utils_Timer_UE::Get_CurrentTimerValue(TimerHandle).Get_TimeElapsed());
                    Payload.Set_CountDirection(UCk_Utils_Timer_UE::Get_CountDirection(TimerHandle));
                    Payload.Set_RunState(UCk_Utils_Timer_UE::Get_CurrentState(TimerHandle));
                    return FInstancedStruct::Make(Payload);
                },
            });
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkTimerSaveDataRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
