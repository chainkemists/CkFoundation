#include "CkTimer_Fragment.h"

#include "CkCore/Time/CkTime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "Net/UnrealNetwork.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.inl.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_timer_fragment
{
    // Save-only persistence handler. Contract, coverage limitation, and the ordering rules every step below
    // depends on: Claude.md § Save/load persistence.
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_Timer>({
                // UNSET when the Timer feature is absent on the entity (nothing to persist).
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
                // Authority-side. DEFERRED requests only — the chrono's _CurrentValue is friend-gated. Every
                // NotReady gate precedes every request.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_Timer_UE::Has(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    // Gate until Setup has run — it mutates a CountDown chrono, so the Jump baseline is not stable
                    // until it clears. NotReady is transient: Setup runs every authority tick.
                    if (Entity.Has<ck::FTag_Timer_NeedsSetup>())
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Payload = New.Get<FCk_SaveData_Timer>();
                    auto TimerHandle = UCk_Utils_Timer_UE::Cast(Entity);

                    // 1. Restore the RUNTIME direction if it drifted from the rebuilt (Params-derived) one.
                    if (UCk_Utils_Timer_UE::Get_CountDirection(TimerHandle) != Payload.Get_CountDirection())
                    { UCk_Utils_Timer_UE::Request_ChangeCountDirection(TimerHandle, Payload.Get_CountDirection(), {}); }

                    // 2. Reposition via an ABSOLUTE Jump — the Jump handler owns the direction-dependent delta math
                    //    and never broadcasts OnTimerDone.
                    UCk_Utils_Timer_UE::Request_Jump(TimerHandle,
                        FCk_Request_Timer_Jump{Payload.Get_Elapsed()}.Set_JumpMode(ECk_RelativeAbsolute::Absolute), {});

                    // 3. Restore run-state. NEVER Request_Complete — it is the only request that re-broadcasts
                    //    OnTimerDone, and a restored terminal timer must not re-fire completion on load.
                    if (UCk_Utils_Timer_UE::Get_CurrentState(TimerHandle) != Payload.Get_RunState())
                    {
                        if (Payload.Get_RunState() == ECk_Timer_State::Running)
                        { UCk_Utils_Timer_UE::Request_Resume(TimerHandle, {}); }
                        else
                        { UCk_Utils_Timer_UE::Request_Pause(TimerHandle, {}); }
                    }

                    return ECk_Persistence_ApplyResult::Applied;
                }});

            // Timers are live-read, so replacing the params fragment IS most of the re-apply. Two derived
            // pieces are re-synced by the fixup: the count-direction tag (via its own request) and the
            // Setup pass (chrono direction alignment). The chrono's GOAL is baked at Add and stays — a
            // Duration retune needs a Timer request that does not exist yet.
            FCk_LiveTuneHandlerRegistry::Register_ViaReplace<FCk_Fragment_Timer_ParamsData>({
                .PostReplace = [](FCk_Handle& InEntity) -> void
                {
                    auto TimerHandle = UCk_Utils_Timer_UE::CastChecked(InEntity);
                    const auto& FreshParams = InEntity.Get<ck::FFragment_Timer_Params>();
                    UCk_Utils_Timer_UE::Request_ChangeCountDirection(
                        TimerHandle, FreshParams.Get_CountDirection(), {});
                },
            });
        }
    };

    const FRegistrar GCkTimerSaveDataRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
